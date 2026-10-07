    LET R=-72
 10 LET S=-140
    GOSUB 100
    LET A=W
    GOSUB 100
    LET B=W
    GOSUB 100
    LET C=W
    GOSUB 100
    LET D=W
    GOSUB 100
    LET E=W
    GOSUB 100
    LET F=W
    GOSUB 100
    LET G=W
    GOSUB 100
    LET H=W
    GOSUB 100
    LET I=W
    GOSUB 100
    LET J=W
    GOSUB 100
    LET K=W
    GOSUB 100
    LET L=W
    GOSUB 100
    LET M=W
    GOSUB 100
    LET N=W
    GOSUB 100
    LET O=W
    GOSUB 100
    LET P=W
    PRINT A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P
    LET R=R+6
    IF R<=72 THEN GOTO 10
    END

REM --- Subroutine to compute four pixels into W, advancing S
100 LET W=0
    LET Z=0
110 GOSUB 200
    LET W=W*10+V
    LET S=S+3
    LET Z=Z+1
    IF Z<4 THEN GOTO 110
    RETURN

REM --- Subroutine to compute the pixel digit V for column S, row R
200 LET X=0
    LET Y=0
    LET U=0
210 LET T=(X*X-Y*Y)/60+S
    LET Y=(2*X*Y)/60+R
    LET X=T
    LET U=U+1
    IF U>=30 THEN GOTO 220
    IF X/60*X+Y/60*Y<=240 THEN GOTO 210
220 LET V=8
    IF U<20 THEN LET V=U/4+1
    RETURN
