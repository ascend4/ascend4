NAME          T1DIET
ROWS
 G  ENERGY
 G  PROTEIN
 G  CALCIUM
 N  COST
COLUMNS
    OATMEAL   ENERGY      110.0        PROTEIN       4.0
    OATMEAL   CALCIUM       2.0        COST          3.0
    CHICKEN   ENERGY      205.0        PROTEIN      32.0
    CHICKEN   CALCIUM      12.0        COST         24.0
    EGGS      ENERGY      160.0        PROTEIN      13.0
    EGGS      CALCIUM      54.0        COST         13.0
    MILK      ENERGY      160.0        PROTEIN       8.0
    MILK      CALCIUM     285.0        COST          9.0
    PIE       ENERGY      420.0        PROTEIN       4.0
    PIE       CALCIUM      22.0        COST         20.0
    PORKBEAN  ENERGY      260.0        PROTEIN      14.0
    PORKBEAN  CALCIUM      80.0        COST         19.0
RHS
    DEMANDS   ENERGY     2000.0        PROTEIN      55.0
    DEMANDS   CALCIUM     800.0
BOUNDS
 UP SERVINGS  OATMEAL       4.0
 UP SERVINGS  CHICKEN       3.0
 UP SERVINGS  EGGS          2.0
 UP SERVINGS  MILK          8.0
 UP SERVINGS  PIE           2.0
 UP SERVINGS  PORKBEAN      2.0
ENDATA
