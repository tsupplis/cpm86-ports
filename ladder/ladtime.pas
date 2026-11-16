{ IBM PC BIOS clock. No CPU-speed calibration or timer reprogramming.
  INTEGER subtraction intentionally wraps at 16 bits (Turbo Pascal 3).
  Poll at least every 27 seconds; longer stalls are bounded. }
TYPE
  ClockState = RECORD
    LowTick, HighTick, Fraction : INTEGER;
  END;
VAR
  ClockLow, ClockHigh, ClockVersion : INTEGER;

PROCEDURE ClockRead;
BEGIN
  INLINE($50/$51/$52/$B4/$00/$CD/$1A/
         $89/$16/ClockLow/$89/$0E/ClockHigh/$5A/$59/$58);
END;

PROCEDURE ClockInit;
BEGIN
  INLINE($50/$53/$51/$52/$56/$57/$1E/$06/
         $B9/$0C/$00/$CD/$E0/
         $07/$1F/$A3/ClockVersion/$5F/$5E/$5A/$59/$5B/$58);
END;

PROCEDURE ClockIdle;
BEGIN
  IF Lo(ClockVersion) = $22 THEN
    INLINE($9C/$FB/$F4/$9D) { preserve flags; interrupts wake HLT }
  ELSE
    INLINE($50/$53/$51/$52/$56/$57/$1E/$06/
           $B9/$8D/$00/$BA/$01/$00/$CD/$E0/
           $07/$1F/$5F/$5E/$5A/$59/$5B/$58);
END;

PROCEDURE ClockReset(VAR Clock : ClockState);
BEGIN
  ClockRead;
  Clock.LowTick := ClockLow;
  Clock.HighTick := ClockHigh;
  Clock.Fraction := 0;
END;

FUNCTION ClockAdvance(VAR Clock : ClockState;
                      LowTick, HighTick : INTEGER) : INTEGER;
VAR Ticks, Correction : INTEGER;
BEGIN
  Ticks := LowTick - Clock.LowTick;
  { BIOS midnight wraps at 0018:00B0, not a power of two. }
  IF HighTick < Clock.HighTick THEN Ticks := Ticks + $00B0;
  Clock.LowTick := LowTick;
  Clock.HighTick := HighTick;
  IF Ticks < 0 THEN Ticks := 0;
  IF Ticks > 500 THEN Ticks := 500;
  { 54.925 ms/tick; carry the fractional correction between calls. }
  Correction := Clock.Fraction + 3 * Ticks;
  ClockAdvance := 55 * Ticks - Correction DIV 40;
  Clock.Fraction := Correction MOD 40;
END;

FUNCTION ClockMillis(VAR Clock : ClockState) : INTEGER;
BEGIN
  ClockRead;
  ClockMillis := ClockAdvance(Clock, ClockLow, ClockHigh);
END;

PROCEDURE ClockDelay(Millis : INTEGER);
VAR Clock : ClockState;
BEGIN
  ClockReset(Clock);
  { First tick may be imminent; add one tick for a minimum delay. }
  IF Millis > 0 THEN BEGIN
    Millis := Millis + 55;
    REPEAT
      ClockIdle;
      Millis := Millis - ClockMillis(Clock);
    UNTIL Millis <= 0;
  END;
END;
