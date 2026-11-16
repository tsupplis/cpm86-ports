# PAGE

Small CP/M-86 text-file pager by Bill Bolton (1980/1982), converted from
CP/M-80 in March 1982, with IBM-compatible adjustments by Kirk Lawrence.

```
PAGE README.TXT
```

Displays 23 lines, then shows `[more]`. Press any key to continue; a key
pressed while output is running exits. No search or backward navigation.
CRLF, CR and LF line endings are accepted. Tabs expand to eight-column
stops, and long lines wrap at 79 columns. Ctrl-Z and physical EOF both
finish normally. Input files are never modified.

The prompt is cleared with backspaces and spaces. No terminal escape
sequences are emitted. On CP/M, a partial final record can contain padding;
Ctrl-Z remains the conventional way to mark a text file's exact end.

## Build and test

```
make
make test
make clean
```

Uses `cpm86_asm86`, `cpm86_gencmd` and `pcdev_cmdinfo`, retaining the original
separate code/data CMD layout. `make test` uses Python's standard library
and emu2; no PCE. Five cases exercise the production command under each of the 2.2 and 3.1
profiles. Three paging cases per profile use a private build with a console
shim: status returns no pending key and input supplies a space. File I/O
and display still use emulated BDOS. Real keyboard continuation remains
unverified; the attempted PTY harness stalled at the prompt.
These profiles do not establish real-OS compatibility.

Local changes remove reverse-video sequences, add the `[more]` prompt,
normalize line endings, reset the column after automatic wrapping, treat
physical EOF as normal, and read the BDOS version from AX.

## License

The supplied source states Copyright (C) 1980,1982, Bill Bolton.
No explicit redistribution/modification license accompanies this copy;
no public-domain claim is made. Original attribution is retained.
