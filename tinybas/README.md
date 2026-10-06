# Tinybasic

Tinybasic is an implementation of the Tiny BASIC language. It conforms to the specification by Dennis Allison, published in People's Computer Company Vol.4 No.2 and reprinted in Dr. Dobb's Journal, January 1976.

## Command line

```
tinybas [OPTIONS] INPUT-FILE
```

Run `tinybas.cmd` on CP/M-86 or `tinybas.com` on DOS. With no input file, or with `-h`, it prints the usage and the options and exits. Source files may use CR LF or LF line endings.

By default the program is interpreted. With `-olst` or `-oc` nothing is run, and a file is written next to the input file instead.

Options are case insensitive (`-nimplied` and `-NIMPLIED` are the same), so each option has its own letter. The value follows the option letter directly (`-olst`). Value names for `-n` and `-c` can be abbreviated to any prefix, for example `-ni`. There are no long (`--option`) forms.

| Option | Values | Default | Effect |
|---|---|---|---|
| `-nMODE` | `optional`, `implied`, `mandatory` | `optional` | `optional`: line numbers are labels and may be omitted. `implied`: lines without a number are numbered automatically. `mandatory`: every line needs a number, in sequence. |
| `-lLIMIT` | integer | 32767 | Highest line number allowed. |
| `-cMODE` | `enabled`, `disabled` | `enabled` | `disabled` rejects `REM` comments and blank lines. |
| `-gLIMIT` | integer | 64 | Maximum `GOSUB` nesting depth. |
| `-oFORMAT` | `lst`, `c` | interpret | `lst` writes a formatted listing and `c` writes a C translation, named after the input file with its extension replaced by `.lst` or `.c`. |
| `-h` | | | Print the usage and the list of options, then exit. |

Examples:

```
tinybas a.bas                           run a.bas
tinybas -olst a.bas                     write the formatted listing a.lst
tinybas -oc a.bas                       write a C translation a.c
tinybas -nmandatory -cdisabled a.bas    strict line numbering, no comments
```

The output file replaces the extension of the input file (`prog.bas` gives `prog.lst`; a name with no extension just gets one added), which also gives valid 8.3 names on CP/M-86. The tool refuses to overwrite the input file itself (for example `-olst prog.lst`).

Messages go to standard error with a prefix: `INF:` for information (usage and `-h` help), `ERR:` for errors (`ERR: Parse error: ...`, `ERR: Runtime error: ...`, a bad option, a second file name, or a missing file). The BASIC program's own `PRINT` output goes to standard output. CP/M-86 has no separate error stream, so there both appear on the console.
