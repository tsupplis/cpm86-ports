# CP/M-86 Ports

A small collection of classic software tools adapted or ported for CP/M-86. The repository contains language interpreters, compilers, conversion utilities, and other retro-computing experiments assembled for the CP/M environment.

Used as a soak testing and source for https://github.com/tsupplis/cpm86-crossdev ...

## Included projects

| Project | What | From | License |
| --- | --- | --- | --- |
| ansi2kr | ANSI C to K&R C converter for BDS C and MSX-C | masakioba / upstream ansi2kr sources | BSD 2-Clause |
| dc | Arbitrary precision reverse Polish desk calculator | Plan 9 / Unix Research Edition dc | MIT |
| filer | VFILER, a full-screen file manager | ZCPR2 VFILER by Rich Conn, CP/M-86 translation by H. M. Van Tassell | Public domain / Abandonware |
| grep | Search a file for a pattern | RetroBSD / DiscoBSD, originally Berkeley grep | BSD 3-Clause |
| lispc | Minimal Lisp interpreter written in C | Johan Fjeldtvedt | MIT |
| tinybas | Tiny BASIC interpreter | Classic Tiny BASIC sources | Public domain / Abandonware |
| vtl2 | VTL-2 interpreter port for CP/M, just for fun using xlt86, serves as a regression test for the translator | MITS VTL-2 source, adapted for CP/M | Public domain / Abandonware |
| xlisp | Lisp interpreter and runtime | David Betz |Public domain / Abandonware |
| yacc | YACC-compatible parser generator | RetroBSD / original yacc sources | BSD-style |

## Repository layout

- ansi2kr/ — ANSI C conversion utility
- dc/ — arbitrary precision desk calculator
- filer/ — VFILER full-screen file manager (8086 assembler)
- grep/ — regular expression search
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

--

## Companion projects

| Project | Description |
|---------|-------------|
| [cpm86-kernel](https://github.com/tsupplis/cpm86-kernel)     | CP/M-86 1.1 distribution rebuilt from patched and reconstituted sources |
| [ccpm86-y2k](https://github.com/tsupplis/ccpm86-y2k)         | CCP/M-86 3.1 distribution rebuilt from patched and reconstituted sources |
| [cpm86-crossdev](https://github.com/tsupplis/cpm86-crossdev) | Unix CP/M-86 cross development project (compilers, emulation and tools) |
| [cpm86-hacking](https://github.com/tsupplis/cpm86-hacking)   | CP/M-86 miscellaneous tools and PCE emulator helpers |
| [cpm86-cmdtools](https://github.com/tsupplis/cpm86-cmdtools) | CP/M-86 `.cmd` file manipulation tools |
| [cpm86-ports](https://github.com/tsupplis/cpm86-ports)       | CP/M-86 application ports in C and assembler |
| [cpm86-vi](https://github.com/tsupplis/cpm86-vi)             | STevie vi port for CP/M-86 and PC-DOS 1.1 |
| [pcdos11-hacking](https://github.com/tsupplis/pcdos11-hacking) | PC-DOS 1.1 distribution, tools and notes |
