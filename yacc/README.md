Yacc for CP/M-86
===============

`make` builds a self-contained `yacc.cmd`. Only the CMD is needed at runtime;
`yaccpar.tpl` remains the editable build-time parser skeleton.

After linking, `cmdmod` installs the template in group slot 2, type EXTRA.
The payload is `YTPL`, a little-endian 16-bit byte count, and the unmodified
template bytes. Its minimum and maximum allocations are fixed to the payload
size rounded to a paragraph. `template.c` reads the loaded group's base-page descriptor
and exposes a bounded byte reader. It does not reopen the executable, use
ES as persistent state, or copy the template into the C data segment.
The build rejects empty templates and payloads that would exceed one segment.

This is a CMD-only build. Its build tools are `aztec43_acc`, `cmdmod`, and
Python 3 (standard library only). Changing the template rebuilds the CMD.

`make test` also requires `emu2`. It runs Yacc in directories without an
external template, compiles and runs a generated calculator, checks CP/M
text endings and group padding, rejects missing/malformed embedded templates,
and verifies a template change through the actual build rule. The calculator
covers semantic actions, precedence, associativity, an empty production,
parentheses, and invalid input.
