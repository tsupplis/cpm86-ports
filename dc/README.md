# dc

An arbitrary precision calculator using reverse Polish notation: enter
operands first, then the operator.

## Usage

```
dc [-h] [file]
```

Without a file, read commands from the console. After a file ends, input
continues from the console; end a batch script with `q`. `-h`, `-H`, and
`-?` show help. Calculator commands are case sensitive.

| Command | Meaning |
| --- | --- |
| `number` | Push a value; `_` makes it negative, `.` introduces a fraction. |
| `+ - * / % ^` | Add, subtract, multiply, divide, remainder, power. |
| `v` | Square root. |
| `p`, `P`, `f` | Print the top value, pop and print a string, or print the stack. |
| `d`, `c`, `z` | Duplicate the top, clear the stack, or push its depth. |
| `sx`, `lx` | Store to or load from register x. |
| `Sx`, `Lx` | Push to or pop from register x as a stack. |
| `[text]`, `x` | Push a string; execute the top string as commands. |
| `<x`, `>x`, `=x` | Compare the top two values and conditionally execute register x. |
| `i`, `I`, `o`, `O` | Set/get the input base or output base. |
| `k`, `K` | Set/get the number of fractional digits retained. |
| `X`, `Z` | Replace the top value with its scale or digit count. |
| `?` | Read and execute one line from the input source. |
| `q`, `Q` | Quit two execution levels, or a number of levels popped from the stack. |

## Examples

At the console, enter one line at a time:

```
3 4 + p
6 7 * p
2 k 1 3 / p
q
```

These print 7, 42, and one third truncated to two fractional digits.
For batch use, save the commands in `calc.dc` and run `dc calc.dc`.

## Notes

The default input/output base is 10 and scale is 0. Digits `A` through `F`
are available for bases up to 16. `!` shell commands are unavailable.
This calculator has no `-o` or `-p` options; `p` and `o` are calculator commands.

## Build

`make` builds CP/M-86 `.cmd` and PC-DOS `.com` binaries with Aztec C86 4.2.
`make test` runs emulator checks; `make clean` removes build output.

Plan 9 / Unix Research Edition source; [MIT](LICENSE.md).
