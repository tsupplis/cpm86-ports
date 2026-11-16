# Short fig-FORTH examples

These examples use historical fig-FORTH syntax and 16-bit integers. Each
fits in one source screen and runs a demonstration when loaded.

| Screen | Source | Demonstrates | Result |
| --- | --- | --- | --- |
| 1 | [gcd.fth](gcd.fth) | Euclid's algorithm, stack manipulation, `BEGIN WHILE REPEAT` | `6` |
| 2 | [factor.fth](factor.fth) | Factorial with a counted loop | `5040` |
| 3 | [fib.fth](fib.fth) | Fibonacci numbers using two stack values | `0 1 1 2 3 5 8 13 21 34 55` |
| 4 | [primes.fth](primes.fth) | Trial division, conditionals and `LEAVE` | `2 3 5 7 11 13 17 19 23 29` |
| 5 | [stars.fth](stars.fth) | Character output and nested word calls | A five-row triangle |

## Run on CP/M-86

1. Run `make` in the parent `figforth` directory. It builds `FORTH.CMD` and
   packs the examples into `examples/demo.blk`.
2. In a separate CP/M drive/user area, copy `FORTH.CMD` and copy `DEMO.BLK`
   **as `FORTH.BLK`**. Do not replace a block file containing your own work.
3. Run `FORTH`, then enter:

```forth
1 LOAD
2 LOAD
3 LOAD
4 LOAD
5 LOAD
BYE
```

Use `1 LIST` to inspect the first screen. Definitions remain available after
loading, so you can try `48 18 GCD .`, `5 FACTORIAL .`, `12 FIB .` or
`8 TRIANGLE`. Restarting Forth requires loading the definitions again.

The triangle prints:

```text
*
**
***
****
*****
```

Keep factorial inputs between 0 and 7, Fibonacci inputs between 0 and 23,
and GCD inputs positive and at most 32767 to avoid signed 16-bit overflow.
The primality demonstration is intentionally small and uses trial division.

## Editing

Edit the `.fth` files and rerun `make`. `mkblocks.py` requires ASCII, at most
16 lines per example and at most 64 characters per line. It reserves screen
zero and pads each source screen to 1024 bytes with spaces; the resulting
block file contains no line-ending bytes.

`make test` in the parent directory loads all five examples from this same
block file and checks their output.
