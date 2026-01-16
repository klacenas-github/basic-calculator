#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pango/pango.h>
#include <glib/gkeyfile.h>

// CSS provider for styling
GtkCssProvider *css_provider;

// Global variables for calculator state
GtkWidget *display;
GtkTextBuffer *text_buffer;
char current_input[256] = "";
char expression[1024] = ""; // Full expression being built
char history_buffer[4096] = ""; // Accumulated calculation history
double result = 0;
gboolean has_result = FALSE;

// Precision menu items
GtkWidget *precision_0, *precision_1, *precision_2, *precision_3, *precision_4;
GtkWidget *precision_6, *precision_8, *precision_10;

// Display height menu items
GtkWidget *display_auto, *display_small, *display_medium, *display_large;

// Precision variable (font sizes now calculated dynamically)
int result_precision = 6; // Default 6 decimal places

// Display height variable (in pixels, 0 = auto-scale)
int display_height = 0; // Default 0 = auto-scale based on window size

// Window size variables
int window_width = 200;   // Default width
int window_height = 300;  // Default height

// Configuration file path
#define CONFIG_FILE ".calculator_config"

// Function prototypes
void update_ui_scaling(GtkWidget *window);
void close_open_menus(GtkWidget *window);
void clear_calculator(void);
gboolean save_settings_idle(gpointer data);

// Safe string operations with bounds checking
int safe_strcat(char *dest, const char *src, size_t dest_size) {
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    if (dest_len + src_len >= dest_size) {
        return 0; // Would overflow
    }
    strcat(dest, src);
    return 1;
}

int safe_strcpy(char *dest, const char *src, size_t dest_size) {
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        return 0; // Would overflow
    }
    strcpy(dest, src);
    return 1;
}

// Function to save settings
void save_settings() {
    GKeyFile *keyfile = g_key_file_new();
    gchar *config_path;
    gchar *config_dir;
    GError *error = NULL;

    // Set values
    g_key_file_set_integer(keyfile, "Settings", "result_precision", result_precision);
    g_key_file_set_integer(keyfile, "Settings", "display_height", display_height);
    g_key_file_set_integer(keyfile, "Settings", "window_width", window_width);
    g_key_file_set_integer(keyfile, "Settings", "window_height", window_height);

    // Get config directory
    config_dir = g_build_filename(g_get_home_dir(), NULL);
    config_path = g_build_filename(config_dir, CONFIG_FILE, NULL);

    // Save to file
    if (!g_key_file_save_to_file(keyfile, config_path, &error)) {
        g_warning("Failed to save config: %s", error->message);
        g_error_free(error);
    }

    g_free(config_path);
    g_key_file_free(keyfile);
}

// Function to load settings
void load_settings() {
    GKeyFile *keyfile = g_key_file_new();
    gchar *config_path;
    gchar *config_dir;
    GError *error = NULL;

    // Get config directory
    config_dir = g_build_filename(g_get_home_dir(), NULL);
    config_path = g_build_filename(config_dir, CONFIG_FILE, NULL);

    // Load from file
    if (g_key_file_load_from_file(keyfile, config_path, G_KEY_FILE_NONE, &error)) {
        // Load values with defaults if not found
        result_precision = g_key_file_get_integer(keyfile, "Settings", "result_precision", &error);
        if (error) {
            result_precision = 6; // default
            g_error_free(error);
            error = NULL;
        }

        display_height = g_key_file_get_integer(keyfile, "Settings", "display_height", &error);
        if (error) {
            display_height = 0; // default (auto-scale)
            g_error_free(error);
            error = NULL;
        }

        window_width = g_key_file_get_integer(keyfile, "Settings", "window_width", &error);
        if (error) {
            window_width = 200; // default
            g_error_free(error);
            error = NULL;
        }

        window_height = g_key_file_get_integer(keyfile, "Settings", "window_height", &error);
        if (error) {
            window_height = 300; // default
            g_error_free(error);
            error = NULL;
        }
    } else {
        // File doesn't exist, use defaults
        g_error_free(error);
    }

    g_free(config_path);
    g_key_file_free(keyfile);
}


// Simple function to update UI scaling based on window size
void update_ui_scaling(GtkWidget *window) {
    int width, height;
    gtk_window_get_size(GTK_WINDOW(window), &width, &height);

    // Ensure minimum sizes
    if (width < 100) width = 100;
    if (height < 150) height = 150;

    // Check if window is maximized and limit the effective size
    GdkWindowState state = gdk_window_get_state(gtk_widget_get_window(window));
    if (state & GDK_WINDOW_STATE_MAXIMIZED) {
        // When maximized, use reasonable maximum sizes instead of full screen
        width = MIN(width, 1200);  // Cap at reasonable desktop size
        height = MIN(height, 800);
    }

    // Calculate font sizes based on window dimensions
    int base_size = (width < height) ? width : height;

    // Display font: smaller for 5-line history display
    int display_font_size = MAX(5, base_size / 20);  // Much smaller font for more content
    display_font_size = MIN(display_font_size, 20);  // Smaller maximum for compact display

    // Button font: smaller for compact buttons
    int button_font_size = MAX(6, display_font_size * 2 / 3);
    button_font_size = MIN(button_font_size, 16);

    // Menu font: readable size
    int menu_font_size = MAX(8, button_font_size * 4 / 5);
    menu_font_size = MIN(menu_font_size, 18);

    // Update display font via CSS

    // Update display height for 5-line history
    GtkWidget *scrolled_window = gtk_widget_get_parent(display);
    if (GTK_IS_SCROLLED_WINDOW(scrolled_window)) {
        int final_display_height;

        if (display_height == 0) {
            // Auto-scale mode: calculate based on window size
            // Calculate line height using Pango metrics for accurate 5-line sizing
            PangoContext *context = gtk_widget_get_pango_context(display);
            if (context) {
                PangoFontDescription *font_desc = pango_font_description_new();
                pango_font_description_set_weight(font_desc, PANGO_WEIGHT_BOLD);
                pango_font_description_set_size(font_desc, display_font_size * PANGO_SCALE);

                PangoFontMetrics *metrics = pango_context_get_metrics(context, font_desc, NULL);
                int line_height = (pango_font_metrics_get_ascent(metrics) +
                                  pango_font_metrics_get_descent(metrics)) / PANGO_SCALE;

                pango_font_metrics_unref(metrics);
                pango_font_description_free(font_desc);

                // Calculate height for 5 lines plus padding
                final_display_height = line_height * 5 + 12; // 5 lines + padding
                final_display_height = MAX(final_display_height, 80); // Minimum height
                final_display_height = MIN(final_display_height, height * 2 / 3); // Maximum 2/3 of window
            } else {
                final_display_height = 120; // Fallback
            }
        } else {
            // Fixed height mode: use configured height
            final_display_height = display_height;
        }

        gtk_widget_set_size_request(scrolled_window, -1, final_display_height);
    }

    // Update button sizes and fonts
    int button_width = MAX(30, (width - 40) / 4);  // 4 columns with padding
    button_width = MIN(button_width, 200);  // Cap button width
    int button_height = MAX(15, height / 15); // More aggressive scaling for visible changes
    button_height = MIN(button_height, height / 6); // Scale up to 1/6 window height

    // Get all widgets in the window
    GList *children = gtk_container_get_children(GTK_CONTAINER(window));
    if (children) {
        GtkWidget *vbox = children->data;
        if (GTK_IS_CONTAINER(vbox)) {
            GList *vbox_children = gtk_container_get_children(GTK_CONTAINER(vbox));
            if (vbox_children) {
                // Update menu bar font via CSS

                // Update grid (buttons)
                if (vbox_children->next) {
                    GtkWidget *grid = vbox_children->next->data;
                    if (GTK_IS_GRID(grid)) {
                        // Update grid spacing
                        int spacing = MAX(2, MIN(10, base_size / 80));
                        gtk_grid_set_row_spacing(GTK_GRID(grid), spacing);
                        gtk_grid_set_column_spacing(GTK_GRID(grid), spacing);

                        // Update all buttons in the grid
                        GList *grid_children = gtk_container_get_children(GTK_CONTAINER(grid));
                        for (GList *iter = grid_children; iter; iter = iter->next) {
                            GtkWidget *child = GTK_WIDGET(iter->data);
                            if (GTK_IS_BUTTON(child)) {
                                gtk_widget_set_name(child, "calc-button");
                                gtk_widget_set_size_request(child, button_width, -1); // Width fixed, height flexible
                                gtk_widget_set_vexpand(child, TRUE); // Expand vertically to fill space
                            }
                        }
                        g_list_free(grid_children);
                    }
                }
            }
            g_list_free(vbox_children);
        }
        g_list_free(children);
    }

    // Update CSS with new font sizes
    char css[512];
    snprintf(css, sizeof(css), "#display { font-weight: bold; font-size: %dpx; }\n#menu-bar { font-size: %dpx; }\n#calc-button { font-size: %dpx; }", display_font_size, menu_font_size, button_font_size);
    gtk_css_provider_load_from_data(css_provider, css, -1, NULL);

    // Force redraw
    gtk_widget_queue_draw(window);
}



// Window resize/move handler
gboolean on_window_resize(GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
    (void)widget; (void)event; (void)data;
    static int last_x = -1, last_y = -1, last_width = 0, last_height = 0;
    int threshold = 5; // Small threshold to avoid too many updates

    // Check if window moved (position changed) - close menus
    if (last_x != -1 && (abs(event->x - last_x) > threshold || abs(event->y - last_y) > threshold)) {
        close_open_menus(widget);
    }

    // Check if window resized - update scaling and save size
    if (abs(event->width - last_width) > threshold || abs(event->height - last_height) > threshold) {
        last_width = event->width;
        last_height = event->height;

        // Update stored window size
        window_width = event->width;
        window_height = event->height;

        // Save settings with new window size
        g_idle_add(save_settings_idle, NULL);

        update_ui_scaling(widget);
    }

    // Update position tracking
    last_x = event->x;
    last_y = event->y;

    return FALSE;
}

// Helper function to close any open menus
void close_open_menus(GtkWidget *window) {
    // Find and hide any visible popup menus
    GList *toplevels = gtk_window_list_toplevels();
    for (GList *iter = toplevels; iter; iter = iter->next) {
        GtkWidget *toplevel = GTK_WIDGET(iter->data);
        if (GTK_IS_MENU(toplevel) && gtk_widget_get_visible(toplevel)) {
            gtk_widget_hide(toplevel);
        }
    }
    g_list_free(toplevels);

    // Also deactivate the menu bar to be safe
    GList *children = gtk_container_get_children(GTK_CONTAINER(window));
    if (children) {
        GtkWidget *vbox = children->data;
        if (GTK_IS_CONTAINER(vbox)) {
            GList *vbox_children = gtk_container_get_children(GTK_CONTAINER(vbox));
            if (vbox_children) {
                GtkWidget *menu_bar = vbox_children->data;
                if (GTK_IS_MENU_BAR(menu_bar)) {
                    gtk_menu_shell_deactivate(GTK_MENU_SHELL(menu_bar));
                }
            }
            g_list_free(vbox_children);
        }
        g_list_free(children);
    }
}

// Handler to close menus when clicking on buttons
gboolean on_window_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    // Only handle button press events (not release)
    if (event->type != GDK_BUTTON_PRESS) {
        return FALSE;
    }

    // Only close menus if the click is not on menu-related widgets
    // Check if the event target is a menu item, menu bar, or menu - if so, don't close
    GtkWidget *target = gtk_get_event_widget((GdkEvent *)event);
    if (target && (GTK_IS_MENU_ITEM(target) || GTK_IS_MENU_BAR(target) || GTK_IS_MENU(target))) {
        return FALSE; // Let the menu system handle the click
    }

    return FALSE; // Allow other handlers to process the event
}

// Handler to close menus when window loses focus
gboolean on_window_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer data) {
    (void)widget; (void)event; (void)data;
    close_open_menus(widget);
    return FALSE;
}

// Initialize UI scaling when window is first shown
gboolean on_window_show(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    update_ui_scaling(widget);
    clear_calculator();
    return FALSE;
}

// Function to update menu labels to show current selection with light blue color
void update_precision_menu_labels() {
    // Update all menu item labels - selected one gets light blue color using Pango markup
    gtk_menu_item_set_label(GTK_MENU_ITEM(precision_0),
        result_precision == 0 ? "<span foreground=\"#4A90E2\">0 decimal places</span>" : "0 decimal places");
    gtk_menu_item_set_label(GTK_MENU_ITEM(precision_1),
        result_precision == 1 ? "<span foreground=\"#4A90E2\">1 decimal place</span>" : "1 decimal place");
    gtk_menu_item_set_label(GTK_MENU_ITEM(precision_2),
        result_precision == 2 ? "<span foreground=\"#4A90E2\">2 decimal places</span>" : "2 decimal places");
    gtk_menu_item_set_label(GTK_MENU_ITEM(precision_3),
        result_precision == 3 ? "<span foreground=\"#4A90E2\">3 decimal places</span>" : "3 decimal places");
    gtk_menu_item_set_label(GTK_MENU_ITEM(precision_4),
        result_precision == 4 ? "<span foreground=\"#4A90E2\">4 decimal places</span>" : "4 decimal places");
    gtk_menu_item_set_label(GTK_MENU_ITEM(precision_6),
        result_precision == 6 ? "<span foreground=\"#4A90E2\">6 decimal places</span>" : "6 decimal places");
    gtk_menu_item_set_label(GTK_MENU_ITEM(precision_8),
        result_precision == 8 ? "<span foreground=\"#4A90E2\">8 decimal places</span>" : "8 decimal places");
    gtk_menu_item_set_label(GTK_MENU_ITEM(precision_10),
        result_precision == 10 ? "<span foreground=\"#4A90E2\">10 decimal places</span>" : "10 decimal places");
}

// Function to update display height menu labels to show current selection
void update_display_height_menu_labels() {
    // Update all menu item labels - selected one gets light blue color using Pango markup
    gtk_menu_item_set_label(GTK_MENU_ITEM(display_auto),
        display_height == 0 ? "<span foreground=\"#4A90E2\">Auto-scale</span>" : "Auto-scale");
    gtk_menu_item_set_label(GTK_MENU_ITEM(display_small),
        display_height == 80 ? "<span foreground=\"#4A90E2\">Small (80px)</span>" : "Small (80px)");
    gtk_menu_item_set_label(GTK_MENU_ITEM(display_medium),
        display_height == 120 ? "<span foreground=\"#4A90E2\">Medium (120px)</span>" : "Medium (120px)");
    gtk_menu_item_set_label(GTK_MENU_ITEM(display_large),
        display_height == 160 ? "<span foreground=\"#4A90E2\">Large (160px)</span>" : "Large (160px)");
}

// Idle callback to save settings without blocking menu operations
gboolean save_settings_idle(gpointer data) {
    (void)data;
    save_settings();
    return FALSE; // Don't repeat
}

// Menu callback functions for precision changes
void on_precision_changed(GtkMenuItem *menuitem, gpointer user_data) {
    (void)menuitem; (void)user_data;
    int new_precision = GPOINTER_TO_INT(user_data);

    // Only update if precision actually changed
    if (result_precision != new_precision) {
        result_precision = new_precision;

        // Update menu labels immediately to show new selection
        update_precision_menu_labels();

        // Defer settings save to avoid blocking during menu operation
        g_idle_add(save_settings_idle, NULL);
    }
}

// Menu callback functions for display height changes
void on_display_height_changed(GtkMenuItem *menuitem, gpointer user_data) {
    int new_height = GPOINTER_TO_INT(user_data);

    // Only update if height actually changed
    if (display_height != new_height) {
        display_height = new_height;

        // Update menu labels immediately to show new selection
        update_display_height_menu_labels();

        // Update UI scaling immediately to reflect the change
        GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(menuitem));
        if (GTK_IS_WINDOW(window)) {
            update_ui_scaling(window);
        }

        // Defer settings save to avoid blocking during menu operation
        g_idle_add(save_settings_idle, NULL);
    }
}

// Function to auto-scroll display to bottom
gboolean scroll_display_to_bottom(gpointer data) {
    (void)data;
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(text_buffer, &end);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(display), &end, 0.0, FALSE, 0.0, 0.0);
    return FALSE; // Don't repeat
}

// Function to update display (shows current expression being built)
void update_display() {
    char display_text[2048] = "";

    // Build complete display: history + current expression
    if (strlen(history_buffer) > 0) {
        strcpy(display_text, history_buffer);
    }

    // Add current expression + current input
    char current_expr[1024] = "";
    if (strlen(expression) > 0) {
        strcpy(current_expr, expression);
        if (strlen(current_input) > 0) {
            if (strlen(current_expr) > 0) {
                strcat(current_expr, " ");
            }
            strcat(current_expr, current_input);
        }
    } else if (strlen(current_input) > 0) {
        strcpy(current_expr, current_input);
    }

    if (strlen(current_expr) > 0) {
        if (strlen(display_text) > 0) {
            strcat(display_text, current_expr);
        } else {
            strcpy(display_text, current_expr);
        }
    }

    // Set the complete display text
    gtk_text_buffer_set_text(text_buffer, display_text, -1);

    // Auto-scroll to show the latest content
    g_idle_add(scroll_display_to_bottom, NULL);
}

// Function to append to calculation history
void append_to_history(const char *text) {
    char history_text[1024];

    snprintf(history_text, sizeof(history_text), "%s\n", text);

    // Append to history buffer
    if (strlen(history_buffer) + strlen(history_text) < sizeof(history_buffer) - 1) {
        strcat(history_buffer, history_text);
    }

    // Update display with new history
    update_display();
}

// Function to clear calculator (keeps calculation history)
void clear_calculator() {
    strcpy(current_input, "");
    strcpy(expression, "");
    has_result = FALSE;
    update_display();
}

// Function to handle number button clicks
void on_number_clicked(GtkWidget *widget, gpointer data) {
    char *number = (char *)data;

    // Start new input if we had a result
    if (has_result) {
        strcpy(expression, "");
        has_result = FALSE;
    }

    // Check if we can add this number (total length < 20 AND decimal places <= 16)
    char *decimal_pos = strchr(current_input, '.');
    int decimal_places = 0;

    if (decimal_pos != NULL) {
        decimal_places = strlen(decimal_pos + 1);
    }

    // Allow input if: total length < 20, AND (no decimal OR decimal places < 16)
    if (strlen(current_input) < 20 && (decimal_pos == NULL || decimal_places < 16)) {
        strcat(current_input, number);
        update_display();
    }
}

// Function to handle operation button clicks
// Check if adding this operator would create consecutive operators
int would_create_consecutive_ops(const char *expr, const char *current, char op) {
    // If we have current input (number being entered), allow operators
    // This allows completing the current number and starting a new operation
    if (strlen(current) > 0) {
        return 0;
    }

    // Allow operators at the start of expression (but not +)
    if (strlen(expr) == 0) {
        return op == '+';  // Block unary + at start, allow - and others
    }

    // Check the last character in the expression (ignoring spaces)
    const char *last_char = NULL;
    if (strlen(expr) > 0) {
        for (int i = strlen(expr) - 1; i >= 0; i--) {
            if (expr[i] != ' ') {
                last_char = &expr[i];
                break;
            }
        }
    }

    // If last character is an operator, don't allow another operator
    // Exception: allow '-' after binary operators (+, *, /) for unary minus
    // Exception: allow '(' and ')' anywhere (parentheses have special rules)
    if (last_char && (*last_char == '+' || *last_char == '-' || *last_char == '*' || *last_char == '/')) {
        // Parentheses are always allowed
        if (op == '(' || op == ')') {
            return 0;
        }
        // Only allow '-' after +, *, / (but not after another -)
        if (op == '-' && *last_char != '-') {
            return 0;  // Allow unary minus after binary operators
        }
        return 1;  // Block all other operator combinations
    }

    return 0;
}

void on_operation_clicked(GtkWidget *widget, gpointer data) {
    char *op = (char *)data;

    // Start new expression with result if we had a previous calculation
    if (has_result) {
        // Special case: opening parenthesis after result should start new calculation
        if (*op == '(') {
            // Clear everything and start fresh with parenthesis
            strcpy(expression, "(");
            strcpy(current_input, "");
            has_result = FALSE;
            update_display();
            return;
        } else {
            // Normal operators: use result as starting point
            if (result == (int)result) {
                sprintf(expression, "%d", (int)result);
            } else {
                sprintf(expression, "%.*f", result_precision, result);
            }
            strcpy(current_input, "");
            has_result = FALSE;
        }
    }

    // Prevent consecutive operators (except unary minus)
    // Check after has_result processing so result can be used
    if (would_create_consecutive_ops(expression, current_input, *op)) {
        return; // Ignore this operator input
    }

    // Special handling for parentheses - can be added anywhere
    if (*op == '(' || *op == ')') {
        // If we have current input, append it to expression first
        if (strlen(current_input) > 0) {
            if (strlen(expression) > 0) {
                strcat(expression, " ");
            }
            strcat(expression, current_input);
            strcpy(current_input, "");
        }

        // Add space before parenthesis if there's content (except for opening at start)
        if (strlen(expression) > 0 && *op == ')') {
            strcat(expression, " ");
        }

        strncat(expression, op, 1);
    } else if (strlen(current_input) > 0) {
        // Normal operators (+, -, *, /) - require a number before them
        if (strlen(expression) > 0) {
            strcat(expression, " ");
        }
        strcat(expression, current_input);
        strcat(expression, " ");
        strncat(expression, op, 1);
        strcpy(current_input, "");
    } else if (strlen(expression) > 0) {
        // Operator after expression (e.g., after closing parenthesis)
        // Allow operators to be added to continue the expression
        strcat(expression, " ");
        strncat(expression, op, 1);
        // current_input stays empty, ready for next number
    } else if (strlen(expression) == 0 && strlen(current_input) == 0) {
        // Starting expression with operator (mainly for unary minus)
        strncat(expression, op, 1);
    }
    update_display();
}

// Stack-based expression evaluator with parentheses support
typedef struct {
    double *data;
    int top;
    int capacity;
} DoubleStack;

typedef struct {
    char *data;
    int top;
    int capacity;
} CharStack;

void init_double_stack(DoubleStack *stack, int capacity) {
    stack->data = malloc(sizeof(double) * capacity);
    stack->top = -1;
    stack->capacity = capacity;
}

void init_char_stack(CharStack *stack, int capacity) {
    stack->data = malloc(sizeof(char) * capacity);
    stack->top = -1;
    stack->capacity = capacity;
}

int push_double(DoubleStack *stack, double value) {
    if (stack->top >= stack->capacity - 1) {
        return 0; // Stack full
    }
    stack->data[++stack->top] = value;
    return 1; // Success
}

int push_char(CharStack *stack, char value) {
    if (stack->top >= stack->capacity - 1) {
        return 0; // Stack full
    }
    stack->data[++stack->top] = value;
    return 1; // Success
}

double pop_double(DoubleStack *stack) {
    return stack->top >= 0 ? stack->data[stack->top--] : 0;
}

char pop_char(CharStack *stack) {
    return stack->top >= 0 ? stack->data[stack->top--] : '\0';
}

char peek_char(CharStack *stack) {
    return stack->top >= 0 ? stack->data[stack->top] : '\0';
}

int get_precedence(char op) {
    switch (op) {
        case '+':
        case '-': return 1;
        case '*':
        case '/': return 2;
        default: return 0;
    }
}

double evaluate_expression(const char *expr) {
    DoubleStack values;
    CharStack ops;
    init_double_stack(&values, 100);
    init_char_stack(&ops, 100);

    int i = 0;
    while (expr[i] != '\0') {
        if (expr[i] == ' ') {
            i++;
            continue;
        }

        if ((expr[i] >= '0' && expr[i] <= '9') || expr[i] == '.') {
            // Parse number
            double num = 0;
            int decimal_place = 0;
            double decimal_multiplier = 1;

            while ((expr[i] >= '0' && expr[i] <= '9') || expr[i] == '.') {
                if (expr[i] == '.') {
                    decimal_place = 1;
                } else {
                    if (decimal_place) {
                        decimal_multiplier *= 0.1;
                        num += (expr[i] - '0') * decimal_multiplier;
                    } else {
                        num = num * 10 + (expr[i] - '0');
                    }
                }
                i++;
            }
            if (!push_double(&values, num)) {
                free(values.data);
                free(ops.data);
                return NAN; // Stack overflow
            }
            continue;
        }

        if (expr[i] == '(') {
            if (!push_char(&ops, expr[i])) {
                free(values.data);
                free(ops.data);
                return NAN; // Stack overflow
            }
        } else if (expr[i] == ')') {
            while (ops.top >= 0 && peek_char(&ops) != '(') {
                char op = pop_char(&ops);
                if (values.top < 1) {
                    // Not enough operands
                    free(values.data);
                    free(ops.data);
                    return NAN;
                }
                double b = pop_double(&values);
                double a = pop_double(&values);
                double result_val;
                switch (op) {
                    case '+': result_val = a + b; break;
                    case '-': result_val = a - b; break;
                    case '*': result_val = a * b; break;
                    case '/': result_val = b != 0 ? a / b : 0; break;
                    default: result_val = 0; break;
                }
                if (!push_double(&values, result_val)) {
                    free(values.data);
                    free(ops.data);
                    return NAN; // Stack overflow
                }
            }
            if (ops.top < 0 || peek_char(&ops) != '(') {
                // Mismatched parenthesis
                free(values.data);
                free(ops.data);
                return NAN;
            }
            pop_char(&ops); // Remove '('
        } else if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/') {
            // Check if this is a unary operator
            int is_unary = 0;
            if ((expr[i] == '+' || expr[i] == '-') && i > 0) {
                // Look backwards to find the previous non-space character
                int j = i - 1;
                while (j >= 0 && expr[j] == ' ') {
                    j--;
                }
                if (j < 0 || expr[j] == '(' || expr[j] == '+' || expr[j] == '-' ||
                    expr[j] == '*' || expr[j] == '/') {
                    is_unary = 1;
                    if (expr[i] == '-') {
                        // Unary minus: push 0 and treat as subtraction
                        if (!push_double(&values, 0)) {
                            free(values.data);
                            free(ops.data);
                            return NAN; // Stack overflow
                        }
                    }
                    // For unary +, skip it entirely (no-op)
                    if (expr[i] == '+') {
                        i++; // Skip this character
                        continue;
                    }
                }
            } else if ((expr[i] == '+' || expr[i] == '-') && i == 0) {
                // At start of expression
                is_unary = 1;
                if (expr[i] == '-') {
                    if (!push_double(&values, 0)) {
                        free(values.data);
                        free(ops.data);
                        return NAN; // Stack overflow
                    }
                }
                // For unary +, skip it entirely (no-op)
                if (expr[i] == '+') {
                    i++; // Skip this character
                    continue;
                }
            }

            if (!is_unary) {
                while (ops.top >= 0 && get_precedence(peek_char(&ops)) >= get_precedence(expr[i])) {
                    if (values.top < 1) {
                        // Not enough operands
                        free(values.data);
                        free(ops.data);
                        return NAN;
                    }
                    char op = pop_char(&ops);
                    double b = pop_double(&values);
                    double a = pop_double(&values);
                    double result_val;
                    switch (op) {
                        case '+': result_val = a + b; break;
                        case '-': result_val = a - b; break;
                        case '*': result_val = a * b; break;
                        case '/': result_val = b != 0 ? a / b : 0; break;
                        default: result_val = 0; break;
                    }
                    if (!push_double(&values, result_val)) {
                        free(values.data);
                        free(ops.data);
                        return NAN; // Stack overflow
                    }
                }
            }
            if (!push_char(&ops, expr[i])) {
                free(values.data);
                free(ops.data);
                return NAN; // Stack overflow
            }
        }
        i++;
    }

    // Process remaining operations
    while (ops.top >= 0) {
        if (values.top < 1) {
            // Not enough operands
            free(values.data);
            free(ops.data);
            return NAN;
        }
        char op = pop_char(&ops);
        double b = pop_double(&values);
        double a = pop_double(&values);
        double result_val;
        switch (op) {
            case '+': result_val = a + b; break;
            case '-': result_val = a - b; break;
            case '*': result_val = a * b; break;
            case '/': result_val = b != 0 ? a / b : 0; break;
            default: result_val = 0; break;
        }
        if (!push_double(&values, result_val)) {
            free(values.data);
            free(ops.data);
            return NAN; // Stack overflow
        }
    }

    // Check for leftover opening parentheses (mismatched)
    for (int j = 0; j <= ops.top; j++) {
        if (ops.data[j] == '(') {
            free(values.data);
            free(ops.data);
            return NAN;
        }
    }

    double result = values.top >= 0 ? values.data[values.top] : 0;

    // Free allocated memory
    free(values.data);
    free(ops.data);

    return result;
}

// Function to handle equals button click
void on_equals_clicked(GtkWidget *widget, gpointer data) {
    // If we have a result displayed and no new input, start fresh new calculation
    if (has_result && strlen(current_input) == 0 && strlen(expression) == 0) {
        has_result = FALSE;
        strcpy(history_buffer, "");  // Clear history for fresh start
        update_display();
        return;
    }

    // Check if we have something to evaluate
    if (strlen(current_input) > 0 || strlen(expression) > 0) {
        // Complete the expression with any remaining input
        if (strlen(current_input) > 0) {
            if (strlen(expression) > 0) {
                if (!safe_strcat(expression, " ", sizeof(expression))) return;
            }
            if (!safe_strcat(expression, current_input, sizeof(expression))) return;
        }

        // Only evaluate if we have a complete expression
        if (strlen(expression) > 0) {
            // Evaluate the expression
            double calc_result = evaluate_expression(expression);

            // Check for evaluation errors (NaN or other issues)
            if (isnan(calc_result) || isinf(calc_result)) {
                // Show syntax error for invalid expressions
                char error_str[512];
                sprintf(error_str, "%s = syntax error", expression);
                has_result = FALSE;
                strcpy(expression, "");
                strcpy(current_input, "");
                append_to_history(error_str);
                return;
            }

            // Show the full expression with result
            char result_str[512];
            if (calc_result == (int)calc_result) {
                sprintf(result_str, "%s = %d", expression, (int)calc_result);
            } else {
                // Calculate appropriate precision for display
                int display_precision = result_precision;
                double abs_result = fabs(calc_result);

                // For numbers less than 1, use significant digit precision
                if (abs_result < 1.0 && abs_result > 0.0) {
                    // Find how many decimal places needed for significant digits
                    double log_val = log10(abs_result);
                    int first_sig_digit_pos = -floor(log_val); // Position after decimal
                    display_precision = first_sig_digit_pos + (result_precision - 1);

                    // Cap at reasonable maximum to prevent excessive output
                    if (display_precision > 12) {
                        display_precision = 12;
                    }
                }

                sprintf(result_str, "%s = %.*f", expression, display_precision, calc_result);
            }

            // Store result for next calculation
            result = calc_result;
            has_result = TRUE;

            // Clear expression and current input for next calculation (before adding to history)
            strcpy(expression, "");
            strcpy(current_input, "");

            // Add to history
            append_to_history(result_str);
        }
    }
}

// Function to handle clear button click
void on_clear_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    clear_calculator();
}

// Function to handle decimal point
void on_decimal_clicked(GtkWidget *widget, gpointer data) {
    if (has_result) {
        strcpy(expression, "");
        has_result = FALSE;
    }

    if (strchr(current_input, '.') == NULL) {
        if (strlen(current_input) == 0) {
            strcat(current_input, "0");
        }
        strcat(current_input, ".");
        update_display();
    }
}

// Function to handle backspace (delete one character)
void on_backspace_clicked(GtkWidget *widget, gpointer data) {
    size_t len = strlen(current_input);
    if (len > 0) {
        current_input[len - 1] = '\0';
        update_display();
    } else if (strlen(expression) > 0) {
        // Remove last part of expression
        char *last_space = strrchr(expression, ' ');
        if (last_space != NULL) {
            *last_space = '\0';
        } else {
            expression[0] = '\0';
        }
        update_display();
    }
}

// Function to handle delete key (clear everything including history)
void on_delete_clicked(GtkWidget *widget, gpointer data) {
    strcpy(current_input, "");
    strcpy(expression, "");
    strcpy(history_buffer, "");
    has_result = FALSE;
    gtk_text_buffer_set_text(text_buffer, "", -1);
}

// Function to handle keyboard input
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    guint key = event->keyval;

    // Handle number keys (0-9) and numpad keys
    if (key >= '0' && key <= '9') {
        char num_str[2] = {(char)key, '\0'};
        on_number_clicked(NULL, (gpointer)num_str);
        return TRUE;
    }

    // Handle numpad number keys
    switch (key) {
        case GDK_KEY_KP_0:
            on_number_clicked(NULL, (gpointer)"0");
            return TRUE;
        case GDK_KEY_KP_1:
            on_number_clicked(NULL, (gpointer)"1");
            return TRUE;
        case GDK_KEY_KP_2:
            on_number_clicked(NULL, (gpointer)"2");
            return TRUE;
        case GDK_KEY_KP_3:
            on_number_clicked(NULL, (gpointer)"3");
            return TRUE;
        case GDK_KEY_KP_4:
            on_number_clicked(NULL, (gpointer)"4");
            return TRUE;
        case GDK_KEY_KP_5:
            on_number_clicked(NULL, (gpointer)"5");
            return TRUE;
        case GDK_KEY_KP_6:
            on_number_clicked(NULL, (gpointer)"6");
            return TRUE;
        case GDK_KEY_KP_7:
            on_number_clicked(NULL, (gpointer)"7");
            return TRUE;
        case GDK_KEY_KP_8:
            on_number_clicked(NULL, (gpointer)"8");
            return TRUE;
        case GDK_KEY_KP_9:
            on_number_clicked(NULL, (gpointer)"9");
            return TRUE;
    }

    // Handle operation keys
    switch (key) {
        case '+':
        case GDK_KEY_KP_Add:
            on_operation_clicked(NULL, (gpointer)"+");
            return TRUE;
        case '-':
        case GDK_KEY_KP_Subtract:
            on_operation_clicked(NULL, (gpointer)"-");
            return TRUE;
        case '*':
        case GDK_KEY_KP_Multiply:
            on_operation_clicked(NULL, (gpointer)"*");
            return TRUE;
        case '/':
        case GDK_KEY_KP_Divide:
            on_operation_clicked(NULL, (gpointer)"/");
            return TRUE;
        case '(':
            on_operation_clicked(NULL, (gpointer)"(");
            return TRUE;
        case ')':
            on_operation_clicked(NULL, (gpointer)")");
            return TRUE;
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case '=':
            on_equals_clicked(NULL, NULL);
            return TRUE;
        case '.':
        case GDK_KEY_KP_Decimal:
            on_decimal_clicked(NULL, NULL);
            return TRUE;
        case 'c':
        case 'C':
        case GDK_KEY_Escape:
            on_clear_clicked(NULL, NULL);
            return TRUE;
        case GDK_KEY_BackSpace:
            on_backspace_clicked(NULL, NULL);
            return TRUE;
        case GDK_KEY_Delete:
            on_delete_clicked(NULL, NULL);
            return TRUE;
        default:
            return FALSE;
    }
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *button;

    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create CSS provider for styling
    css_provider = gtk_css_provider_new();
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // Load saved settings
    load_settings();

    // Create main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Basic Calculator");
    gtk_window_set_default_size(GTK_WINDOW(window), window_width, window_height);

    // Set window type to normal for standard behavior
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_NORMAL);

    // Center the window on screen
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    // Ensure window is resizable and not maximized
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

    // Allow ultra-compact window sizes
    GdkGeometry geometry;
    geometry.min_width = 70;
    geometry.min_height = 30;
    gtk_window_set_geometry_hints(GTK_WINDOW(window), window, &geometry, GDK_HINT_MIN_SIZE);


    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);
    g_signal_connect(window, "configure-event", G_CALLBACK(on_window_resize), NULL);
    g_signal_connect(window, "show", G_CALLBACK(on_window_show), NULL);
    g_signal_connect(window, "button-press-event", G_CALLBACK(on_window_button_press), NULL);

    // Create menu bar
    GtkWidget *menu_bar = gtk_menu_bar_new();
    gtk_widget_set_name(menu_bar, "menu-bar");

    // View menu
    GtkWidget *view_menu = gtk_menu_new();
    GtkWidget *view_menu_item = gtk_menu_item_new_with_label("View");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_menu_item), view_menu);


    // Precision submenu
    GtkWidget *precision_menu = gtk_menu_new();
    GtkWidget *precision_item = gtk_menu_item_new_with_label("Result Precision");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(precision_item), precision_menu);

    // Display height submenu
    GtkWidget *display_menu = gtk_menu_new();
    GtkWidget *display_item = gtk_menu_item_new_with_label("Display Height");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(display_item), display_menu);

    // Precision options (regular menu items with markup support)
    precision_0 = gtk_menu_item_new_with_label("0 decimal places");
    precision_1 = gtk_menu_item_new_with_label("1 decimal place");
    precision_2 = gtk_menu_item_new_with_label("2 decimal places");
    precision_3 = gtk_menu_item_new_with_label("3 decimal places");
    precision_4 = gtk_menu_item_new_with_label("4 decimal places");
    precision_6 = gtk_menu_item_new_with_label("6 decimal places");
    precision_8 = gtk_menu_item_new_with_label("8 decimal places");
    precision_10 = gtk_menu_item_new_with_label("10 decimal places");

    // Display height options
    display_auto = gtk_menu_item_new_with_label("Auto-scale");
    display_small = gtk_menu_item_new_with_label("Small (80px)");
    display_medium = gtk_menu_item_new_with_label("Medium (120px)");
    display_large = gtk_menu_item_new_with_label("Large (160px)");

    // Enable markup for all menu items initially
    gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(precision_0))), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(precision_1))), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(precision_2))), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(precision_3))), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(precision_4))), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(precision_6))), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(precision_8))), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(precision_10))), TRUE);

    // Enable markup for display height menu items
    gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(display_auto))), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(display_small))), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(display_medium))), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(display_large))), TRUE);

    // Update labels to show current selection with blue color
    update_precision_menu_labels();
    update_display_height_menu_labels();

    g_signal_connect(precision_0, "activate", G_CALLBACK(on_precision_changed), GINT_TO_POINTER(0));
    g_signal_connect(precision_1, "activate", G_CALLBACK(on_precision_changed), GINT_TO_POINTER(1));
    g_signal_connect(precision_2, "activate", G_CALLBACK(on_precision_changed), GINT_TO_POINTER(2));
    g_signal_connect(precision_3, "activate", G_CALLBACK(on_precision_changed), GINT_TO_POINTER(3));
    g_signal_connect(precision_4, "activate", G_CALLBACK(on_precision_changed), GINT_TO_POINTER(4));
    g_signal_connect(precision_6, "activate", G_CALLBACK(on_precision_changed), GINT_TO_POINTER(6));
    g_signal_connect(precision_8, "activate", G_CALLBACK(on_precision_changed), GINT_TO_POINTER(8));
    g_signal_connect(precision_10, "activate", G_CALLBACK(on_precision_changed), GINT_TO_POINTER(10));

    g_signal_connect(display_auto, "activate", G_CALLBACK(on_display_height_changed), GINT_TO_POINTER(0));
    g_signal_connect(display_small, "activate", G_CALLBACK(on_display_height_changed), GINT_TO_POINTER(80));
    g_signal_connect(display_medium, "activate", G_CALLBACK(on_display_height_changed), GINT_TO_POINTER(120));
    g_signal_connect(display_large, "activate", G_CALLBACK(on_display_height_changed), GINT_TO_POINTER(160));

    gtk_menu_shell_append(GTK_MENU_SHELL(precision_menu), precision_0);
    gtk_menu_shell_append(GTK_MENU_SHELL(precision_menu), precision_1);
    gtk_menu_shell_append(GTK_MENU_SHELL(precision_menu), precision_2);
    gtk_menu_shell_append(GTK_MENU_SHELL(precision_menu), precision_3);
    gtk_menu_shell_append(GTK_MENU_SHELL(precision_menu), precision_4);
    gtk_menu_shell_append(GTK_MENU_SHELL(precision_menu), precision_6);
    gtk_menu_shell_append(GTK_MENU_SHELL(precision_menu), precision_8);
    gtk_menu_shell_append(GTK_MENU_SHELL(precision_menu), precision_10);

    gtk_menu_shell_append(GTK_MENU_SHELL(display_menu), display_auto);
    gtk_menu_shell_append(GTK_MENU_SHELL(display_menu), display_small);
    gtk_menu_shell_append(GTK_MENU_SHELL(display_menu), display_medium);
    gtk_menu_shell_append(GTK_MENU_SHELL(display_menu), display_large);

    // Add precision menu to view menu (fonts are now automatic)
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), precision_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), display_item);

    // Add view menu to menu bar
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), view_menu_item);

    // Create display (text view for scrolling)
    display = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(display), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(display), GTK_WRAP_WORD);
    gtk_text_view_set_justification(GTK_TEXT_VIEW(display), GTK_JUSTIFY_RIGHT);
    gtk_widget_set_margin_bottom(display, 10);
    gtk_widget_set_margin_end(display, 20);  // Add right margin for scrollbar

    // Get the text buffer
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(display));

    // Set display widget name for CSS targeting
    gtk_widget_set_name(display, "display");

    // Add scrolling
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), display);

    // Create grid for buttons
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), FALSE);  // Don't stretch rows
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    // Create buttons
    // Row 0: Display
    gtk_grid_attach(GTK_GRID(grid), scrolled_window, 0, 0, 4, 1);

    // Row 1: Clear and operations
    button = gtk_button_new_with_label("C");
    g_signal_connect(button, "clicked", G_CALLBACK(on_clear_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 1, 1, 1);

    button = gtk_button_new_with_label("/");
    g_signal_connect(button, "clicked", G_CALLBACK(on_operation_clicked), (gpointer)"/");
    gtk_grid_attach(GTK_GRID(grid), button, 1, 1, 1, 1);

    button = gtk_button_new_with_label("*");
    g_signal_connect(button, "clicked", G_CALLBACK(on_operation_clicked), (gpointer)"*");
    gtk_grid_attach(GTK_GRID(grid), button, 2, 1, 1, 1);

    button = gtk_button_new_with_label("-");
    g_signal_connect(button, "clicked", G_CALLBACK(on_operation_clicked), (gpointer)"-");
    gtk_grid_attach(GTK_GRID(grid), button, 3, 1, 1, 1);

    // Row 2: Numbers 7, 8, 9, +
    button = gtk_button_new_with_label("7");
    g_signal_connect(button, "clicked", G_CALLBACK(on_number_clicked), (gpointer)"7");
    gtk_grid_attach(GTK_GRID(grid), button, 0, 2, 1, 1);

    button = gtk_button_new_with_label("8");
    g_signal_connect(button, "clicked", G_CALLBACK(on_number_clicked), (gpointer)"8");
    gtk_grid_attach(GTK_GRID(grid), button, 1, 2, 1, 1);

    button = gtk_button_new_with_label("9");
    g_signal_connect(button, "clicked", G_CALLBACK(on_number_clicked), (gpointer)"9");
    gtk_grid_attach(GTK_GRID(grid), button, 2, 2, 1, 1);

    button = gtk_button_new_with_label("+");
    g_signal_connect(button, "clicked", G_CALLBACK(on_operation_clicked), (gpointer)"+");
    gtk_grid_attach(GTK_GRID(grid), button, 3, 2, 1, 1);

    // Row 3: Numbers 4, 5, 6
    button = gtk_button_new_with_label("4");
    g_signal_connect(button, "clicked", G_CALLBACK(on_number_clicked), (gpointer)"4");
    gtk_grid_attach(GTK_GRID(grid), button, 0, 3, 1, 1);

    button = gtk_button_new_with_label("5");
    g_signal_connect(button, "clicked", G_CALLBACK(on_number_clicked), (gpointer)"5");
    gtk_grid_attach(GTK_GRID(grid), button, 1, 3, 1, 1);

    button = gtk_button_new_with_label("6");
    g_signal_connect(button, "clicked", G_CALLBACK(on_number_clicked), (gpointer)"6");
    gtk_grid_attach(GTK_GRID(grid), button, 2, 3, 1, 1);

    button = gtk_button_new_with_label(")");
    g_signal_connect(button, "clicked", G_CALLBACK(on_operation_clicked), (gpointer)")");
    gtk_grid_attach(GTK_GRID(grid), button, 3, 3, 1, 1);

    // Row 4: Numbers 1, 2, 3, =
    button = gtk_button_new_with_label("1");
    g_signal_connect(button, "clicked", G_CALLBACK(on_number_clicked), (gpointer)"1");
    gtk_grid_attach(GTK_GRID(grid), button, 0, 4, 1, 1);

    button = gtk_button_new_with_label("2");
    g_signal_connect(button, "clicked", G_CALLBACK(on_number_clicked), (gpointer)"2");
    gtk_grid_attach(GTK_GRID(grid), button, 1, 4, 1, 1);

    button = gtk_button_new_with_label("3");
    g_signal_connect(button, "clicked", G_CALLBACK(on_number_clicked), (gpointer)"3");
    gtk_grid_attach(GTK_GRID(grid), button, 2, 4, 1, 1);

    button = gtk_button_new_with_label("=");
    g_signal_connect(button, "clicked", G_CALLBACK(on_equals_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 3, 4, 1, 2); // Span 2 rows, back to column 3

    // Row 5: Numbers 0, ., (, )
    button = gtk_button_new_with_label("0");
    g_signal_connect(button, "clicked", G_CALLBACK(on_number_clicked), (gpointer)"0");
    gtk_grid_attach(GTK_GRID(grid), button, 0, 5, 2, 1);

    button = gtk_button_new_with_label(".");
    g_signal_connect(button, "clicked", G_CALLBACK(on_decimal_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 2, 5, 1, 1);

    button = gtk_button_new_with_label("(");
    g_signal_connect(button, "clicked", G_CALLBACK(on_operation_clicked), (gpointer)"(");
    gtk_grid_attach(GTK_GRID(grid), button, 3, 5, 1, 1);

    // Create vertical box to hold menu and grid
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 0);

    // Add vbox to window
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Show all widgets
    gtk_widget_show_all(window);

    // Start GTK main loop87
    gtk_main();

    return 0;
}