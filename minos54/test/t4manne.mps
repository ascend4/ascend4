NAME          MANNE10
ROWS
 G  MON001
 G  MON002
 G  MON003
 G  MON004
 G  MON005
 G  MON006
 G  MON007
 G  MON008
 G  MON009
 G  MON010
 L  CAP002
 L  CAP003
 L  CAP004
 L  CAP005
 L  CAP006
 L  CAP007
 L  CAP008
 L  CAP009
 L  CAP010
 L  TERMINV
COLUMNS
    KAP001    MON001      .1           CAP001     1.0
    KAP001    CAP002    -1.0
    KAP002    MON002      .1           CAP002     1.0
    KAP002    CAP003    -1.0
    KAP003    MON003      .1           CAP003     1.0
    KAP003    CAP004    -1.0
    KAP004    MON004      .1           CAP004     1.0
    KAP004    CAP005    -1.0
    KAP005    MON005      .1           CAP005     1.0
    KAP005    CAP006    -1.0
    KAP006    MON006      .1           CAP006     1.0
    KAP006    CAP007    -1.0
    KAP007    MON007      .1           CAP007     1.0
    KAP007    CAP008    -1.0
    KAP008    MON008      .1           CAP008     1.0
    KAP008    CAP009    -1.0
    KAP009    MON009      .1           CAP009     1.0
    KAP009    CAP010    -1.0
    KAP010    MON010      .1           CAP010     1.0
    KAP010    TERMINV     .03
    CON001    MON001    -1.0
    CON002    MON002    -1.0
    CON003    MON003    -1.0
    CON004    MON004    -1.0
    CON005    MON005    -1.0
    CON006    MON006    -1.0
    CON007    MON007    -1.0
    CON008    MON008    -1.0
    CON009    MON009    -1.0
    CON010    MON010    -1.0
    INV001    MON001    -1.0           CAP002    -1.0
    INV002    MON002    -1.0           CAP003    -1.0
    INV003    MON003    -1.0           CAP004    -1.0
    INV004    MON004    -1.0           CAP005    -1.0
    INV005    MON005    -1.0           CAP006    -1.0
    INV006    MON006    -1.0           CAP007    -1.0
    INV007    MON007    -1.0           CAP008    -1.0
    INV008    MON008    -1.0           CAP009    -1.0
    INV009    MON009    -1.0           CAP010    -1.0
    INV010    MON010    -1.0           CAP011    -1.0
    INV010    TERMINV   -1.0
RHS
*
*   The RHS is zero
*
    LAGRANGE  MON002      -0.9         MON003      -0.8
    LAGRANGE  MON010     -10.0
RANGES
    RANGE1    MON010      10.0         TERMINV     20.0
BOUNDS
 FX BOUND1    KAP001      3.05
 LO BOUND1    KAP002      3.05
 LO BOUND1    KAP003      3.05
 LO BOUND1    KAP004      3.05
 LO BOUND1    KAP005      3.05
 LO BOUND1    KAP006      3.05
 LO BOUND1    KAP007      3.05
 LO BOUND1    KAP008      3.05
 LO BOUND1    KAP009      3.05
 LO BOUND1    KAP010      3.05
 LO BOUND1    CON001       .95
 LO BOUND1    CON002       .95
 LO BOUND1    CON003       .95
 LO BOUND1    CON004       .95
 LO BOUND1    CON005       .95
 LO BOUND1    CON006       .95
 LO BOUND1    CON007       .95
 LO BOUND1    CON008       .95
 LO BOUND1    CON009       .95
 LO BOUND1    CON010       .95
 LO BOUND1    INV001       .05
 LO BOUND1    INV002       .05
 LO BOUND1    INV003       .05
 LO BOUND1    INV004       .05
 LO BOUND1    INV005       .05
 LO BOUND1    INV006       .05
 LO BOUND1    INV007       .05
 LO BOUND1    INV008       .05
 LO BOUND1    INV009       .05
 LO BOUND1    INV010       .05
 UP BOUND1    INV008       .112
 UP BOUND1    INV009       .114
 UP BOUND1    INV010       .116
 FX INITIAL   KAP002      3.1
 FX INITIAL   KAP003      3.2
 FX INITIAL   KAP004      3.3
 FX INITIAL   KAP005      3.4
 FX INITIAL   KAP006      3.5
 FX INITIAL   KAP007      3.6
 FX INITIAL   KAP008      3.7
 FX INITIAL   KAP009      3.8
 FX INITIAL   KAP010      3.9
ENDATA
