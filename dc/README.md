dc
==

`dc` — arbitrary precision desk calculator.

This is the Plan 9 / Unix Research Edition `dc` ported to CP/M-86 (and PC-DOS)
using the Aztec C86 4.2 cross compiler.

## Synopsis

```
dc [ file ]
```

## Description

`dc` is an arbitrary precision desk calculator. Ordinarily it operates on
decimal integers, but one may specify an input base, output base, and a number
of fractional digits to be maintained.

The overall structure of `dc` is a stacking (reverse Polish) calculator. If an
argument is given, input is taken from that file until its end, then from the
standard input.

The following constructions are recognized:

| Command | Meaning |
| --- | --- |
| *number* | The value of the number is pushed on the stack. A number is an unbroken string of the digits `0-9A-F` or `0-9a-f`. A hexadecimal number beginning with a lower case letter must be preceded by a zero to distinguish it from the command associated with the letter. It may be preceded by an underscore `_` to input a negative number. Numbers may contain decimal points. |
| `+` `-` `*` `/` `%` `^` | Add, subtract, multiply, divide, remainder, or exponentiate the top two values on the stack. The two entries are popped off the stack; the result is pushed on the stack in their place. Any fractional part of an exponent is ignored. |
| `s`*x* / `S`*x* | Pop the top of the stack and store into a register named *x*, where *x* may be any character. Under operation `S` register *x* is treated as a stack and the value is pushed on it. |
| `l`*x* / `L`*x* | Push the value in register *x* onto the stack. The register *x* is not altered. All registers start with zero value. Under operation `L` register *x* is treated as a stack and its top value is popped onto the main stack. |
| `d` | Duplicate the top value on the stack. |
| `p` | Print the top value on the stack. The top value remains unchanged. |
| `P` | Interpret the top of the stack as a text string, remove it, and print it. |
| `f` | Print the values on the stack. |
| `q` / `Q` | Exit the program. If executing a string, the recursion level is popped by two. Under operation `Q` the top value on the stack is popped and the string execution level is popped by that value. |
| `x` | Treat the top element of the stack as a character string and execute it as a string of `dc` commands. |
| `X` | Replace the number on the top of the stack with its scale factor. |
| `[ ... ]` | Put the bracketed text string on the top of the stack. |
| `<`*x* `>`*x* `=`*x* | Pop and compare the top two elements of the stack. Register *x* is executed if they obey the stated relation. |
| `v` | Replace the top element on the stack by its square root. Any existing fractional part of the argument is taken into account, but otherwise the scale factor is ignored. |
| `!` | Interpret the rest of the line as a shell command. **Not available in this port** — see [Porting notes](#porting-notes). |
| `c` | Clear the stack. |
| `i` | The top value on the stack is popped and used as the number base for further input. |
| `I` | Push the input base on the top of the stack. |
| `o` | The top value on the stack is popped and used as the number base for further output. In bases larger than 10, each "digit" prints as a group of decimal digits. |
| `O` | Push the output base on the top of the stack. |
| `k` | Pop the top of the stack, and use that value as a non-negative scale factor: the appropriate number of places are printed on output, and maintained during multiplication, division, and exponentiation. The interaction of scale factor, input base, and output base will be reasonable if all are changed together. |
| `z` | Push the stack level onto the stack. |
| `Z` | Replace the number on the top of the stack with its length. |
| `?` | A line of input is taken from the input source (usually the terminal) and executed. |
| `;` `:` | Used by `bc` for array operations. |

## Scale factor

The scale factor set by `k` determines how many digits are kept to the right of
the decimal point. If *s* is the current scale factor, *sa* is the scale of the
first operand, *sb* is the scale of the second, and *b* is the (integer) second
operand, results are truncated to the following scales:

| Operator | Resulting scale |
| --- | --- |
| `+` `-` | max(*sa*, *sb*) |
| `*` | min(*sa* + *sb*, max(*s*, *sa*, *sb*)) |
| `/` | *s* |
| `%` | so that dividend = divisor * quotient + remainder; remainder has sign of dividend |
| `^` | min(*sa* * \|*b*\|, max(*s*, *sa*)) |
| `v` | max(*s*, *sa*) |

## Examples

Print the first ten values of *n*!

```
[la1+dsa*pla10>y]sy
0sa1
lyx
```

## Diagnostics

| Message | Meaning |
| --- | --- |
| *x* `is unimplemented` | where *x* is an octal number: an internal error. |
| `stack empty` | not enough elements on the stack to do what was asked. |
| `out of space` | the free list is exhausted (too many digits). |
| `out of headers` | too many numbers being kept around. |
| `divide by 0` | trying to divide by zero. |
| `nesting depth` | too many levels of nested execution. |

## Building

```sh
make
```

This produces `dc.cmd` for CP/M-86. A PC-DOS `dc.com` can be built with
`make dc.com`. See https://github.com/tsupplis/cpm86-crossdev for the
cross-development environment.

## Porting notes

- The POSIX-only headers `<sys/wait.h>` and `<unistd.h>` and the Plan 9
  `#pragma varargck` declaration were removed.
- The `!` shell escape used `fork`/`execl`/`wait`. CP/M-86 has no command
  processor to shell out to, so the rest of the line is consumed and ignored.
- `memmove` is not in the Aztec C86 CP/M-86 library; the one call site copies
  between two distinct allocations, so `memcpy` is used instead.

## Bugs

When the input base exceeds 16, there is no notation for digits greater than
`F`.

## License

MIT — see [LICENSE.md](LICENSE.md). Copyright 2021 Plan 9 Foundation.
