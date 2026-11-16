{
  Some terminal routines not handled by Turbo Pascal.
  
  CursOff: hide the cursor on the CP/M-86 or CCP/M-86 console
  CursOn: show the cursor on the CP/M-86 or CCP/M-86 console
  Beep: ring the terminal bell
}

PROCEDURE CursOff;
BEGIN
  IF Lo(ClockVersion) = $22 THEN Write(#$1B'n')
  ELSE Write(#$1B'f');
END;

PROCEDURE CursOn;
BEGIN
  IF Lo(ClockVersion) = $22 THEN Write(#$1B'm')
  ELSE Write(#$1B'e');
END;

{ CP/M-86 colour extension, as used by util.lib: save position,
  set foreground, restore position. Restore neutral text after the glyph. }
PROCEDURE DrawGlyph(Ch : CHAR; Color : INTEGER);
BEGIN
  Write(#$1B'j'#$1B'b', Chr(Color), #$1B'k');
  Write(Ch);
  Write(#$1B'j'#$1B'b', Chr(NormalColor), #$1B'k');
END;

{ Use the same colours for the initial map and restored actor cells. }
PROCEDURE DrawCell(Ch : CHAR);
BEGIN
  IF Ch IN ['H', '=', '-', '|'] THEN DrawGlyph(Ch, StructureColor)
  ELSE Write(Ch);
END;

PROCEDURE Beep;
BEGIN
  IF sound THEN
    Write(#7);
END;

