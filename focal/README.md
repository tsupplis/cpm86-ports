# FOCAL-81

FOCAL (FOrmula CALculator) is an ancient, block-structured interpretive language originally designed for the DEC PDP-8 and PDP-11.

This implementation (FOCAL-81) was written by **Dave Conroy** (1981) and later enhanced by **Akira KIDA** (1995). This version is ported to **CP/M-86** and **PC-DOS 1.x / MS-DOS** using Aztec C86.

## Building

To build both `focal.cmd` (CP/M-86) and `focal.com` (DOS 1):

```sh
make
```

To clean build artifacts:

```sh
make clean
```

## Running and Command Line Usage

### Command-Line Arguments

```text
focal [file.foc] [[R] | [R | [r] | [r | /R | /r]
```

- **`focal`**: Starts in interactive mode at the `*` prompt.
- **`focal file.foc`**: Loads `file.foc` into program memory and remains at the interactive prompt.
- **`focal file.foc [R]`** (or `[R`, `[r]`, `[r`, `/R`, `/r`): Loads `file.foc` into memory and immediately executes it (`goto`).

Examples:

```sh
emu2 focal.cmd queens.foc [R]
emu2 focal.cmd sieve.foc [R
emu2 focal.com queens.foc /r
```

### Interactive Usage

Under CP/M-86 or DOS:

```text
A>FOCAL
*
```

When started, FOCAL displays the `*` prompt. Commands can be entered interactively for immediate execution or prefixed with a line number to store them in program memory.

To load and run sample scripts from within FOCAL:

```text
*library call queens.foc
*goto
```

or:

```text
*library call sieve.foc
*goto
```

## Language Reference

### Line Numbering and Structure

Programs are organized into groups and lines:

- **`LGRP`**: A group number (integer, e.g., `10`, `20`).
- **`LNUM`**: A line number specifying group and line within the group (e.g., `10.10`, `10.20`, `20.40`).

Lines are stored in numerical order. Entering a line number followed by text adds or replaces that line in memory. Entering a line number alone with no text removes that line.

### Command Syntax

All command keywords may be abbreviated to their first letter (e.g., `t` for `type`, `d` for `do`, `w` for `write`). Multiple commands on a single line are separated by semicolons (`;`).

| Command | Syntax | Description |
| --- | --- | --- |
| **`ask`** | `ask (["prompt"][,][v][,])...` | Prompts for interactive numeric value assignment to variable(s). Subscripted variables (`v(i)`) are supported. |
| **`comment`** | `comment ...` | Ignores the remainder of the line. |
| **`do`** | `do all`<br>`do LGRP`<br>`do LNUM` | Subroutine call:<br>• `do all`: Executes all groups in order as a subroutine.<br>• `do LGRP`: Executes all lines in the specified group and returns.<br>• `do LNUM`: Executes only the specified line and returns. |
| **`erase`** | `erase`<br>`erase all`<br>`erase LGRP`<br>`erase LNUM` | Clears variables or program text:<br>• `erase`: Clears all variables from memory.<br>• `erase all`: Erases the entire program.<br>• `erase LGRP`: Erases all lines in the group.<br>• `erase LNUM`: Erases the specified line. |
| **`for`** | `for v=start,limit; CMD`<br>`for v=start,limit,step; CMD` | Loop iteration. Executes `CMD` while varying `v` from `start` to `limit` (default step is 1.0). Subscripted loop variables are supported. |
| **`goto`** | `goto LNUM`<br>`goto` | Transfers execution:<br>• `goto LNUM`: Jumps to the specified line.<br>• `goto`: Jumps to the beginning of the program. |
| **`if`** | `if (expr) L_NEG ; CMD`<br>`if (expr) L_NEG, L_ZERO ; CMD`<br>`if (expr) L_NEG, L_ZERO, L_POS` | Conditional branching based on expression evaluation:<br>• 1 target: Jumps to `L_NEG` if `< 0`; otherwise executes `CMD`.<br>• 2 targets: Jumps to `L_NEG` if `< 0`, `L_ZERO` if `== 0`; otherwise executes `CMD`.<br>• 3 targets: Jumps to `L_NEG` if `< 0`, `L_ZERO` if `== 0`, `L_POS` if `> 0`. |
| **`library`** | `library call <FILE>`<br>`library save <FILE>`<br>`library delete <FILE>`<br>`library list [<DIR>]` | File and library management:<br>• `call`: Loads program lines from `<FILE>`.<br>• `save`: Saves the current program to `<FILE>`.<br>• `delete`: Deletes `<FILE>`.<br>• `list`: Lists directory contents. |
| **`quit`** | `quit` | Exits to the operating system when called at top level, or stops execution. |
| **`return`** | `return` | Returns early from a subroutine (`do`). |
| **`set`** | `set v = expr` | Evaluates `expr` and assigns the result to variable `v`. Subscripted variables (`v(i)`) are supported. |
| **`type`** | `type item[, item]...` | Outputs text and expressions. Items can be:<br>• An arithmetic expression.<br>• `"%IW.FW"`: Sets integer width (`IW`) and fraction precision (`FW`).<br>• `"%"`: Restores exponential notation.<br>• `"string"`: Prints literal text.<br>• `!`: Prints a newline.<br>• `#`: Prints a carriage return. |
| **`write`** | `write`<br>`write LGRP`<br>`write LNUM` | Lists program source code:<br>• `write`: Lists the entire program.<br>• `write LGRP`: Lists all lines in `LGRP`.<br>• `write LNUM`: Lists line `LNUM`. |

### Built-in Functions

FOCAL includes standard mathematical functions:

| Function | Description |
| --- | --- |
| `fsin(x)` | Sine of $x$ (radians) |
| `fcos(x)` | Cosine of $x$ (radians) |
| `fatn(x)` | Arctangent of $x$ |
| `fexp(x)` | Exponential $e^x$ |
| `flog(x)` | Natural logarithm $\ln(x)$ |
| `fsqt(x)` | Square root $\sqrt{x}$ ($x \ge 0$) |
| `fabs(x)` | Absolute value $\|x\|$ |
| `fsgn(x)` | Sign of $x$ (`-1.0` if $x < 0$, `1.0` if $x \ge 0$) |
| `fitr(x)` | Integer part / floor of $x$ |
| `fran()` | Pseudo-random number in $[0.0, 1.0)$ |

### Expression Grammar

Enclosures `()`, `[]`, and `<>` are interchangeable for grouping expressions.

```text
EXPR     ::= [ SIGN ] PRIMARY { ADDOP PRIMARY }
ADDOP    ::= '+' | '-'
PRIMARY  ::= TERM { MULOP TERM }
MULOP    ::= '*' | '/' | '^'
TERM     ::= '(' EXPR ')' | '[' EXPR ']' | '<' EXPR '>'
           | NUMBER
           | VARIABLE [ '(' EXPR ')' ]
           | BUILTIN '(' EXPR ')'
NUMBER   ::= MANTISSA [ 'e' SIGNED ]
MANTISSA ::= INTEGER | INTEGER '.' | '.' INTEGER | INTEGER '.' INTEGER
SIGNED   ::= [ SIGN ] INTEGER
SIGN     ::= '+' | '-'
INTEGER  ::= DIGIT { DIGIT }
VARIABLE ::= ALPHA { ALPHA | DIGIT }
```

## Sample Programs

- **`queens.foc`**: Solves the Eight Queens chess problem.
- **`sieve.foc`**: Sieve of Eratosthenes prime number benchmark.

## Credits and Copyright

- **Dave Conroy** (1981) — Original author (released freely without explicit copyright restrictions).
- **Akira KIDA** (1995) — ANSI C cleanup, symbol hash table, nested `for` stack fixes, and placed all modifications into the **Public Domain**.
