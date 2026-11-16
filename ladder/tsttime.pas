PROGRAM TstTime;
{$I LADTIME}
VAR C : ClockState; I, Total, Started : INTEGER;
PROCEDURE Check(Value, Expected : INTEGER);
BEGIN
  IF Value <> Expected THEN BEGIN
    WriteLn('FAIL: ', Value, ' expected ', Expected);
    Halt;
  END;
END;
BEGIN
  C.LowTick := 0; C.HighTick := 0; C.Fraction := 0;
  Check(ClockAdvance(C, 0, 0), 0);
  Total := 0;
  FOR I := 1 TO 40 DO Total := Total + ClockAdvance(C, I, 0);
  Check(Total, 2197);
  C.LowTick := $7FFF; C.HighTick := 0; C.Fraction := 0;
  Check(ClockAdvance(C, $8000, 0), 55);
  C.LowTick := $FFFF; C.HighTick := 0; C.Fraction := 0;
  Check(ClockAdvance(C, 0, 1), 55);
  C.LowTick := $00AF; C.HighTick := $18; C.Fraction := 0;
  Check(ClockAdvance(C, 0, 0), 55);
  C.LowTick := 0; C.HighTick := 0; C.Fraction := 0;
  Check(ClockAdvance(C, 1000, 0), 27463);
  WriteLn('Arithmetic PASS');
  ClockInit;
  ClockRead; Started := ClockLow;
  ClockDelay(1000);
  ClockRead; Total := ClockLow - Started;
  WriteLn('Delay ticks: ', Total);
  IF (Total < 20) OR (Total > 22) THEN BEGIN
    WriteLn('FAIL: delay'); Halt;
  END;
  ClockReset(C); Total := 0;
  REPEAT
    ClockIdle;
    Total := Total + ClockMillis(C);
  UNTIL Total >= 3000;
  WriteLn('Elapsed ms: ', Total);
  WriteLn('Timing PASS');
END.
