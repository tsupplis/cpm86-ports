# Sokoban

CP/M-86 port of a Unix curses clone of sokoban.

**Freeware by Russell Marks (ber@astbe).**

I found it as part of the ZCN repository: https://github.com/jamesots/zcn

The C source has been modified for VT52 terminal compatibility, and some
unused variables have been removed.

`sokoban.cmd` is the compiled CP/M-86 binary.

To run it, the files `sokoban.cmd` and `soklevls.dat` are needed on the
CP/M-86 disk. The help file, `sokoban.hlp`, is not used in this version.

## Commands

| Key   | Action              | Key    | Action         | Key | Action              |
| ----- | ------------------- | ------ | -------------- | --- | ------------------- |
| h     | move/push left      | H      | run/push left  | ^H  | run left to object  |
| l     | move/push right     | L      | run/push right | ^L  | run right to object |
| j     | move/push down      | J      | run/push down  | ^J  | run down to object  |
| k     | move/push up        | K      | run/push up    | ^K  | run up to object    |
| u     | undo last move/push | U      | undo all       | 5   | undo all            |
| c / s | temporary save      | ^U / r | reset to temporary save | | |

| ^R | Refresh screen | q | quit |

## The game

Characters on screen are:

| Symbol | Meaning                    | Symbol | Meaning                    |
| ------ | -------------------------- | ------ | -------------------------- |
| `@@` | player                     | `++` | player on saving position  |
| `..` | saving position for packet | `$$` | packets                    |
| `**` | saved packet               | `##` | wall                       |

Your goal is to move all packets to the saving position by pushing them.

As you could see you can make a temporary save. This is useful if you think
that all the moves/pushes you have made are correct, but you don't know how
to go on. In this case you can temporary save (using the c command). If you
then get stuck you need not undo all (using U), you can reset to your
temporary save.

If you have restored a saved game, a temporary save is automatically made
at the start.

## Compiling

Built with the Aztec C compiler (`aztec42_cc`) targeting CP/M-86:

```sh
make
```

This produces `sokoban.cmd` linked against the CP/M-86 C library (`-lc86`).
Terminal I/O uses VT52 escape sequences via a minimal `conio.h`/`conio.c`
that provides `getch()`, `clrscr()`, `gotoxy()` and `textcolor()`.
