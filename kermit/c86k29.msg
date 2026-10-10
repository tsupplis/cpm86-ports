 2-Dec-84 18:51:28-EST,1798;000000000015
Return-Path: <CONTEXT@WASHINGTON.ARPA>
Received: from WASHINGTON.ARPA by CU20B.ARPA with TCP; Sun 2 Dec 84 18:51:10-EST
Date: Sun 2 Dec 84 15:49:29-PST
From: Ronald Blanford <CONTEXT@WASHINGTON.ARPA>
Subject: 86kermit v. 2.9
To: cc.fdc@CU20B.ARPA


Frank,

I've been making changes off and on to 86kermit, but the next month or two
looks slack so I'll give you the current version for testing and possible
release.  The files are in my account as usual with the source in
86KER*.* and the hex and binary in APCKERMIT.*.

The specific features that have been implemented in version 2.9 are:

	o  LOCAL DIRECTORY command now computes file sizes correctly
	   for all files.  The size given is the actual allocation on
	   disk, and not the logical size (which might differ for
	   non-sequential files).

	o  LOCAL TYPE command has been implemented to display (text) files
	   on the screen.  A wildcard filespec is accepted and files displayed
	   alphabetically.  The display is paged in Unix fashion with --more--
	   displayed on the last line.  Typein options at that point can be
	   obtained by hitting a '?'.

	o  Wildcard SENDs now send files in alphabetical order by name, and
	   accept an optional initial filename in the command line to allow
	   transmission of partial groups in the manner of TOPS-20 Kermit.

	o  Problems with use under Concurrent CP/M on the APC have been fixed.
	   In particular, a KERMIT.INI file is no longer required, the
	   SET DEFAULT-DISK command works correctly, and a process dispatch
	   is performed each time a call to the serial port status routine
	   returns negative, vastly improving the response of other jobs.
	   There is still no provision for mutual exclusion on the serial
	   port.

-- Ron
-------
