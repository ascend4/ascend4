NAME          WOPLANT
ROWS
 E  H3
 E  H4
 E  H5
 E  H6
 E  H8
 E  H1
 E  H2
 E  H9
 N  OBJ
COLUMNS
    FG        H1           -1.0        H8           -1.0
    FG        H9            1.0        OBJ          -0.84
    FP        H1           -1.0        H2           -1.0
    FP        OBJ          22.0752
    FD        H1           -1.0        OBJ           0.50037
    FRA       H9            1.0
    FRP       H2            1.0        H9            1.0
    FRE       H2           -0.1        H9            1.0
    FRB       H9            1.0
    FRC       H9            1.0
    FR        H9           -1.0        OBJ          -0.0222
    TEMP
    FA        H1            1.0        H5            1.0
    FA        OBJ          -1.68
    FB        H1            1.0        H6            1.0
    FB        OBJ          -2.52
RHS
    RHS1      H5           -1.0        H6           -1.0
    RHS1      H1            1.0        H2            0.1
    RHS1      H9           -5.0
BOUNDS
 LO BOUND1    FG           -0.95
 LO BOUND1    FP           -0.95
 LO BOUND1    FD           -0.95
 LO BOUND1    FRA          -0.95
 LO BOUND1    FRP          -0.95
 LO BOUND1    FRE          -0.95
 LO BOUND1    FRB          -0.95
 LO BOUND1    FRC          -0.95
 LO BOUND1    FR           -0.95
 LO BOUND1    TEMP         -1.0
 LO BOUND1    FA           -0.95
 LO BOUND1    FB           -0.95
 UP BOUND1    FP           -0.5237
 UP BOUND1    TEMP          1.0
 FR INITIAL   FG           -0.5
 FR INITIAL   FP           -0.5237
 FR INITIAL   FD            1.0
 FR INITIAL   FRA          -0.4
 FR INITIAL   FRP          -0.4
 FR INITIAL   FRE           0.5
 FR INITIAL   FRB           2.5
 FR INITIAL   FRC           0.0
 FR INITIAL   FR            9.0
 FR INITIAL   TEMP         -0.4
 FR INITIAL   FA            0.0
 FR INITIAL   FB            3.0
ENDATA
