fv
==

`fv` (FileView) — view and amend a file in hexadecimal and ASCII, one
128-byte record at a time.

FileView 2.8d by Stephen Hunt (1989-1998), for CP/M-86, Concurrent CP/M and
Concurrent DOS.

## Synopsis

```
fv [d:[user:]]filename[.typ][;password]
fv [d:][path\]filename[.typ][;password]
```

If no file is named on the command line, or the file cannot be opened,
`fv` prompts for one. After a file is closed it prompts again.

- `d:` is a drive letter, `A` to `P`.
- `user` is a user number, `0` to `15`, e.g. `B5:FILE.DAT`. User numbers
  work only on CP/M media.
- `path\` is a directory path. Paths work only on DOS media under
  Concurrent DOS.
- `;password` is the file password, if the file has one.

## Commands

| Key | Action |
| --- | --- |
| `N`, `+`, `^X` | Next record |
| `P`, `-`, `^E` | Previous record |
| `G` | Go to a record number |
| `I` | Invert the record display |
| `T` | Toggle record numbers between hex and decimal |
| `F` | Fill the record with an ASCII or hex string |
| `A` | Amend the record in hex or ASCII (`TAB` switches, `BAC` undoes) |
| `S` | Search for an ASCII or hex string, forward |
| `H` | Print the record |
| `C` | Close the file and ask for another |
| `Q` | Quit |
| `^J` | Help for the current command |

The screen is driven with VT52 escape sequences.

## Building

```sh
make          # fv.cmd (CP/M-86 small model)
make clean
```

See https://github.com/tsupplis/cpm86-crossdev for the cross-development
environment.

## Porting notes

- The data segment now starts with `org 100h`, and `fv.cmd` is built with
  the small model. The old command used the `8080` model, which overlaid the
  data segment on the code at offset 0 and hung on start. The code reads the
  base page at DS:0, so the data has to start above it.

## License

Public domain. The author handed FileView over to the public domain on
27 April 1998 (license number 289999, see [fvlicn.doc](fvlicn.doc)).
