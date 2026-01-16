# Basic Calculator v1.2.0

A professional graphical calculator for Linux written in C using GTK+ 3.0. Features advanced expression-based mathematics with comprehensive error handling, dynamic precision, and robust UI.

## 🚀 Key Features

### Core Functionality
- **Expression-based calculations** - Build complex mathematical expressions before evaluating
- **Full parentheses support** - Proper operator precedence with nested parentheses
- **Advanced arithmetic** - Addition, subtraction, multiplication, division with correct precedence
- **Unary operators** - Support for negative numbers and unary plus
- **Dynamic precision** - Smart decimal display for small numbers based on significant digits
- **Comprehensive error handling** - Syntax validation, division by zero protection, stack overflow prevention

### User Interface
- **Persistent calculation history** - All calculations remain visible with auto-scrollable display
- **Independent scaling** - Display and buttons scale separately for optimal layout
- **Window size memory** - Remembers your preferred window dimensions across sessions
- **Compact professional design** - Optimized button heights and clean spacing
- **Responsive layout** - Adapts perfectly from minimal to fullscreen sizes
- **Right-margin protection** - Scroll bar doesn't interfere with text content

### Input Methods
- **Complete keyboard support** - All operations work from keyboard including parentheses
- **Mouse input** - Intuitive button interface with hover effects
- **Advanced expression building** - Type complex formulas like `(2 + 3) * (-4 + 1)`
- **Operator validation** - Prevents invalid operator sequences in real-time

### Advanced Features
- **Intelligent operator precedence** - `*` and `/` before `+` and `-`, parentheses override all
- **Continuing calculations** - Use previous results in new expressions seamlessly
- **Fresh calculation mode** - Press Enter after results to start completely fresh
- **Settings persistence** - All preferences auto-saved and restored
- **Memory safety** - Comprehensive bounds checking and leak prevention
- **Professional appearance** - Clean, modern interface with proper typography

## 📋 Requirements

- **GTK+ 3.0** development libraries
- **GCC compiler** (or compatible C compiler)
- **GNU Make**
- **Linux system** with X11 or Wayland display server

## 🛠️ Installation & Building

### Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential libgtk-3-dev pkg-config
```

**Fedora/CentOS/RHEL:**
```bash
sudo dnf install gcc gtk3-devel pkgconfig make
```

**Arch Linux:**
```bash
sudo pacman -S gcc gtk3 pkgconf make
```

**CachyOS (current system):**
```bash
sudo pacman -S gcc gtk3 pkgconf make
```

### Build the Calculator

```bash
make
```

### Run the Calculator

```bash
./calculator
```

### Optional: System-wide Installation

```bash
sudo make install
```

This installs to `/usr/local/bin/calculator` for system-wide access.

### Uninstall

```bash
sudo make uninstall
```

## 🎯 How It Works

The calculator uses **expression-based mathematics** - you build complete mathematical expressions before calculating them.

### Expression Building
- **Type expressions**: Build formulas like `(2 + 3) * (4 - 1)`
- **Live display**: See your expression as you type it
- **Operator precedence**: `*` and `/` before `+` and `-`, parentheses override all
- **Complex math**: Supports nested parentheses and chained operations

### Calculation History
- **Persistent history**: All calculations remain visible
- **Scrollable display**: History grows with automatic scrolling
- **Clear vs Delete**: Clear button preserves history, Delete clears everything
- **Continuing math**: Use previous results in new expressions

### Examples

**Basic expressions:**
```
2 + 3 = 5
10 * 2 = 20
```

**With parentheses:**
```
(2 + 3) * 4 = 20
(10 - 5) / 2 = 2.5
```

**Complex expressions:**
```
(2 + (3 * 4)) - 1 = 12
5 + (10 / 2) * 3 = 20
```

**Chained operations:**
```
2 + 3 = 5
* 4 = 20
/ 2 = 10
```

## ⚙️ Menu Options

Use the **View** menu to customize:

- **Result Precision**: Choose decimal places (0, 1, 2, 3, 4, 6, 8, or 10)
- **Display Height**: Choose fixed height or auto-scale (Small, Medium, Large, Auto-scale)

**Smart Scaling**: Display and buttons scale independently. Window size is automatically remembered between sessions.

## 📁 Configuration

Settings are automatically saved to `~/.calculator_config`:

```ini
[Settings]
result_precision=6
display_height=0
window_width=200
window_height=300
```

- **result_precision**: Decimal places for results (0-10)
- **display_height**: Display area height (0=auto-scale, or fixed pixels)
- **window_width/height**: Remembered window dimensions

Delete the config file to restore defaults.

## 🎮 Controls

### Mouse Controls
- **Numbers (0-9)**: Input digits
- **Operations (+, -, *, /)**: Add operators to expression
- **Parentheses (()**: Add opening/closing parentheses
- **Decimal (.)**: Add decimal point
- **Equals (=)**: Calculate the complete expression
- **Clear (C)**: Reset current expression (keeps history)
- **Delete**: Clear everything including history

### Keyboard Controls
- **Numbers (0-9)**: Input digits
- **Numpad (0-9)**: Alternative number input
- **Operations (+, -, *, /)**: Add operators
- **Parentheses ( (, ) )**: Add parentheses
- **Decimal (.)**: Add decimal point
- **Enter/=**: Calculate expression
- **Backspace**: Remove last character/input
- **Delete**: Clear all (history + current)
- **C/Escape**: Clear current expression
- **All operations work from keyboard!**

### Expression Building
1. **Start typing**: `2 + 3`
2. **Add complexity**: `( 2 + 3 ) * 4`
3. **Calculate**: Press `=` to evaluate
4. **Continue**: Result becomes part of new expressions
5. **History**: All calculations remain visible

## 📝 Changelog

### Version 1.2.0 (January 16, 2026)
- **Dynamic precision display**: Smart decimal places for small numbers based on significant digits
- **Comprehensive error handling**: Syntax validation, stack overflow protection, memory leak prevention
- **Advanced operator validation**: Real-time prevention of invalid operator sequences
- **Unary operator support**: Proper handling of negative numbers and unary plus
- **Fresh calculation mode**: Press Enter after results to start completely fresh
- **Enhanced parentheses handling**: Parentheses work after all operators
- **Memory safety improvements**: Bounds checking, proper cleanup, leak prevention
- **Professional error messages**: Clear "syntax error" feedback for malformed expressions
- **Robust expression parsing**: Handles edge cases and malformed input gracefully

### Version 1.1.0 (January 16, 2026)
- **Operator validation**: Prevents consecutive operators and invalid sequences
- **Enhanced keyboard support**: Improved key handling and input validation
- **UI improvements**: Better button spacing and responsive scaling
- **History management**: Enhanced display buffer handling

### Version 1.0.0 (January 16, 2026)
- **Expression-based mathematics**: Build complete formulas before calculating
- **Full parentheses support**: Proper operator precedence with nesting
- **Persistent calculation history**: Scrollable display of all calculations
- **Independent UI scaling**: Display and buttons scale separately
- **Window size memory**: Remembers dimensions between sessions
- **Complete keyboard support**: All operations including parentheses
- **Clean UI**: Compact buttons, user-friendly layout, responsive design
- **Error handling**: Input validation and division by zero protection
- **Settings persistence**: Precision, display height, window size auto-saved

### Version 0.x (Previous)
- Basic immediate calculation
- Simple arithmetic operations
- Basic scrolling display
