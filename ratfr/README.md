# RATFR

I'v added the Supersoft version of RATFOR for CP/M as a cp/m library.
The output is not compatible with Microsoft fortran.

RATFOR v 1.05 from linux.  Runs on CP/M

Below is the README I found in the source package.

Notes:  as of now, the '-O' switch is broken. 
The code is compiled with z88dk and the F77 flag is off.
This makes the output a bit more compatible with Microsoft
FORTRAN on CP/M.

The autoconfig has been removed.  I provide a simple Makefile
for use with z88dk.

        This is a C version of ratfor, derived from a UofA ratfor
	in ratfor. It was originally released to the net sometime
	ago, and It is re-released for the benefit of those sites
	who only get mod->comp.sources.

	It now includes minor changes to produce F77 code as well.

	This code *is* PD. You (public) have all the rights to the code.
	[But this also means you (singular) do not have any *extra*
	rights to the code, hence it is impossible for you to restrict
	the use and distribution of this code in any way.]

	I would, as usual, appreciate hearing about bug fixes and
	improvements.

	oz

	Usenet: [decvax|ihnp4]!utzoo!yetti!oz ||
		    ...seismo!mnetor!yetti!oz
	Bitnet: oz@[yusol|yuyetti].BITNET
	Phonet: [416] 736-5257 x 3976

-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

Autoconf-packaged, converted to ANSI C and packaged for Linux by
Brian Gaeke, brg@dgate.org - Sun Mar  4 15:36:08 PST 2001
with minor updates as of Fri Nov 20 00:12:02 PST 2015

Change History

1.05 - 2020-04-03
This version tries to do a better job outputting data statements in the
order traditionally required by Fortran 77 when handling sequences of
string directives.
This version should emit a warning if eos is not defined when the program uses
the string directive.
Added some tests and documentation of known limitations.

1.04 - 2020-02-19
This version incorporates my attempt to make the string directive work.
You can use the -s command line option to change the type of array
elements generated for strings.
This version should work even when chars are unsigned.
Don't let gnbtok() walk off the beginning of infile[] (patch by Ole Streicher).
Added tests and performed some code cleanup.

1.03 - 2015-12-03
I incorporated the changes from Debian ratfor 1.0-15 and a fix for Debian
bug#716285.

1.02 - 2015-09-16
I fixed several C compiler warnings, rpmlint warnings, and autoconf/automake
warnings.

1.01 - 2001-03-04
I converted the source to ANSI C, added prototypes, adapted it for the GNU
autoconf mechanism, and made the test cases (all two of them!) actually work.

