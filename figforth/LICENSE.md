# fig-FORTH licensing and attribution

## Original fig-FORTH 8086/8088 Release 1.0

Thomas Newman implemented the original CP/M-86 version, dated February 18,
1981, published by the Forth Interest Group in March 1981.

The cover of the original listing states:

> This public domain publication is provided through the courtesy
> of the FORTH Interest Group, PO Box 8231, San Jose, CA 95155.
> Further distribution must contain this notice.

The source listing also states:

> All publications of the Forth Interest Group are public domain. They may be
> further distributed by the inclusion of this credit notice:
>
> This publication has been made available by the
> FORTH Interest Group
> P.O. Box 8231
> San Jose, CA 95155

Source: [FIG 8086/8088 Release 1.0 listing](https://www.forth.org/fig-forth/fig-forth_8086-8088_ver_10.pdf),
cover and first source page. Preserve this credit when redistributing the port.

## Machine-readable source used for this port

`forth.a86` is adapted from Charlie Krajewski's IBM-PC fig-FORTH 1.0 source,
which credits Thomas Newman's implementation. It is a later adaptation,
not an exact transcription of the 1981 CP/M-86 listing.

The unchanged downloaded archive is retained as `upstream/figforth.zip`.
It came from the [FIG source archive](https://www.forth.org/library/eforth_SOC/eforth_SOC_source/figforth/FIGFORTH.ZIP).
Archive SHA-256: `7430a9541d535d20d9aa78e0318281f53a9fdc456037de6ee2f8525c1ca9ad18`.

Its source repeats the public-domain statement and gives this credit address:

> This publication has been made available by:
>
> FORTH Interest Group
> P.O. Box 1105
> San Carlos, Ca. 94070

The original source credits Charlie Krajewski, Thomas Newman, John Cassidy,
Kim Harris, George Flammer and Robert D. Villwock. Those notices are retained
in `forth.a86`. The archive's DOS executables are historical reference files,
not binaries shipped as the CP/M-86 program.

## CP/M-86 adapter and supporting files

New CP/M-86 integration, build and test code is covered by the repository's
[Unlicense/public-domain dedication](../LICENSE.md). That licence applies to
the new contributions;
it does not replace the upstream FIG notices or change their stated terms.
