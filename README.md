# CP/M-86 Ports

A small collection of classic software tools adapted or ported for CP/M-86. The repository contains language interpreters, compilers, conversion utilities, and other retro-computing experiments assembled for the CP/M environment.

Used as a soak testing and source for https://github.com/tsupplis/cpm86-crossdev ...

## Included projects

| Project | What | From | License |
| --- | --- | --- | --- |
| ansi2kr | ANSI C to K&R C converter for BDS C and MSX-C | masakioba / upstream ansi2kr sources | BSD 2-Clause |
| lispc | Minimal Lisp interpreter written in C | Johan Fjeldtvedt | MIT |
| tinybas | Tiny BASIC interpreter | Classic Tiny BASIC sources | Public domain / original terms |
| vtl2 | VTL-2 interpreter port for CP/M, just for fun using xlt86, serves as a regression test for the translator | MITS VTL-2 source, adapted for CP/M | Original upstream / source-specific terms |
| xlisp | Lisp interpreter and runtime | David Betz | XLISP-style open-source terms |
| yacc | YACC-compatible parser generator | RetroBSD / original yacc sources | BSD-style |

## Repository layout

- ansi2kr/ — ANSI C conversion utility
- lispc/ — tiny Lisp interpreter and runtime
- tinybas/ — Tiny BASIC implementation
- vtl2/ — VTL-2 source and generated files
- xlisp/ — XLISP port
- yacc/ — parser generator sources

## Build notes

The root Makefile builds the subprojects in order:

```sh
make
make clean
```

Each subdirectory typically contains its own Makefile and CP/M or DOS build artifacts.

## Cross-development environment

For a CP/M-86 cross-development setup, see:

https://github.com/tsupplis/cpm86-crossdev

## Notes

This repository is mainly a collection of historical software ports and adaptations, not a single monolithic application. Licensing remains mixed because each project keeps its own original upstream terms where applicable.

## Summary

The goal of this collection is to keep small classic programming tools usable in a CP/M-86 environment, while preserving the original source material and the spirit of the original software.
