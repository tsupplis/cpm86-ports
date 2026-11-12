# ROGUE(6)
4

## CP/M-86 terminal colors

`make` builds the CP/M-86 VT52 console version. For a VT100/ANSI color
terminal, use `make TERMINAL=VT100`; switch back with `make TERMINAL=VT52`.
Switching recompiles the renderer and relinks the root and all three overlays.
Keep `rogue.cmd`, `init.ovr`, `save.ovr`, and `score.ovr` from the same build.

| Dungeon symbol | Color |
| --- | --- |
| Floor dots `.` | Dark blue |
| Creatures `A`–`Z` | Bright red |
| Hero `@` | Bright cyan |
| Objects `? ! * : / ] ) = ,` | Bright green |
| Walls and doors `- \| +` | Bright yellow |

Messages, menus, status text and other terrain keep the normal foreground.
Colors follow the displayed symbol, so disguised monsters retain their
disguise. Reverse video remains independent of the foreground color.

VT52 uses the CP/M-86 extension `ESC j ESC b <raw PC color byte> ESC k`.
VT100/ANSI uses SGR foreground colors and bold for bright colors; original
monochrome VT100 hardware cannot display colors. Refresh and exit restore
normal foreground and reverse-video state.

`make test` builds the game and overlays, then runs the real Aztec renderer
under emu2 in both dialects. It verifies identical colored frames, neutral
text, color-only updates, cursor replay, reverse video, full redraws,
menu restoration, clears and exit cleanup.

## NAME

rogue - Exploring The Dungeons of Doom 

## SYNOPSIS

**/usr/games/rogue** [ **-r** ] [ _save\_file_ ] [ **-s** ] [ **-d** ] 

## DESCRIPTION



_Rogue_ is a computer fantasy game with a new twist.  It is crt oriented and the object of the game is to survive the attacks of various monsters and get a lot of gold, rather than the puzzle solving orientation of most computer fantasy games. 

To get started you really only need to know two commands.  The command **?** will give you a list of the available commands and the command **/** will identify the things you see on the screen. 

To win the game (as opposed to merely playing to beat other people's high scores) you must locate the Amulet of Yendor which is somewhere below the 20th level of the dungeon and get it out.  Nobody has achieved this yet and if somebody does, they will probably go down in history as a hero among heroes. 

When the game ends, either by your death, when you quit, or if you (by some miracle) manage to win, _rogue_ will give you a list of the top-ten scorers.  The scoring is based entirely upon how much gold you get.  There is a 10% penalty for getting yourself killed. 

If _save\_file_ is specified, rogue will be restored from the specified saved game file. If the **-r** option is used, the save game file is presumed to be the default. 

The **-s** option will print out the list of scores. 

The **-d** option will kill you and try to add you to the score file. 

For more detailed directions, read the document _A Guide to the Dungeons of Doom._ 

## AUTHORS

Michael C. Toy, Kenneth C. R. C. Arnold, Glenn Wichman 

## FILES

\w'/usr/games/lib/rogue.scores   'u/usr/games/lib/rogue.scores	Score file 
**~**/rogue.save	Default save file 

## SEE ALSO

Michael C. Toy and Kenneth C. R. C. Arnold, _A guide to the Dungeons of Doom_ 

## BUGS



Probably infinite (although countably infinite). However, that Ice Monsters sometimes transfix you permanently is _not_ a bug. It's a feature.
