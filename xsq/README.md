# SQ(1)


## NAME

	Squeeze/Unsqueeze/Type-Squeeze programs by Richard Greenlaw. 
```
xsq    -- squeeze files
xusq   -- unsqueeze files
xtype  -- type squeezed files
```


## SYNOPSIS


```
1.  xsq
     Usage:  xsq [-] path1 path2 ...
             - output to stdout

2.  xusq
     Usage:  xusq [-nu] file1 file2 ...
             -n  Remove carriage returns
             -u  Preserved upper-case pathnames

3.  xtype
     Usage:  xtype file1 file2 ...

```


## DESCRIPTION

	xsq, xusq, and xtype are the UNIX complements of the CP/M programs sq, usq, and typesq, resp.  These two sets of programs are completely compatable. 	xsq/sq is used to compress files for data transfer.  No data is lost in the compression, and no character conversion (say, from CP/M to UNIX text file structure) is done. 	xusq/usq is used to uncompress files which were compressed by xsq/sq.  These programs are intended to be used after the data transfer is complete.  The UNIX xusq has the additional option of removing carriage returns from lines (to perform CP/M to UNIX text file conversion). 	xtype/typesq is used to type out a file that is compressed without having to first run xusq/usq. 

## FILES


```
 xsq.c
 xusq.c
 xtype.c
```


## SEE ALSO


```
UNIXCPM (1)
CPMUNIX (1)
COMHEX (1)
CRCK (1)
UC (1)
```


## AUTHOR


```
        Richard Greenlaw
        251 Colony Ct.
        Gahanna, OH  43230
```


## BUGS

	No known bugs exist in these programs. 