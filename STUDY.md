# METHOD `STUDY`

`STUDY` is a METHOD-language statement for preparing or executing a parametric study from inside a model method such as `on_load`.

## Syntax

```ascend
STUDY observed_var_list
    [VARY var FROM lower TO upper (STEPS n [LINEAR|LOG] | STEP delta | RATIO r)]
    [RUN method_name]
    [FILE "path"];
```

Examples:

```ascend
STUDY accel_required, time_run;

STUDY accel_required, time_run
    VARY time_run FROM 2 {s} TO 6 {s} STEPS 4
    FILE "study.tsv";

STUDY y, x VARY x FROM 1 TO 10 STEP 0.1;
STUDY y, x VARY x FROM 1 TO 10 RATIO 1.1;
STUDY y, x VARY x FROM 1 TO 10 STEPS 8 LOG RUN reset;
```

## Semantics

- The observed variable list is required.
- `VARY` is optional.
- `RUN method_name` is optional and is executed before each study point.
- `FILE` is optional.
- The varied variable may also appear in the observed variable list.
- The current solver and options are taken from the existing GUI state and/or earlier METHOD `SOLVER` / `OPTION` statements. They are not restated inside `STUDY`.

Stepping modes:

- `STEPS n` means `n` intervals with both endpoints included.
- `STEP delta` uses a linear increment.
- `RATIO r` uses a logarithmic/geometric progression.
- `STEPS n LOG` requests logarithmic spacing.
- `STEPS n` without `LOG` uses linear spacing.

Validation rules:

- `STEP` must be non-zero and must move from the lower bound toward the upper bound.
- `RATIO` must be positive, not equal to `1`, and must move from the lower bound toward the upper bound.
- Logarithmic studies require both bounds to be positive.

## `./a4 run`

In the `ascxx` / CLI path, `STUDY` executes immediately.

- If `VARY` is present, the requested cases are solved in sequence.
- If `RUN` is present, the named method is run before each study point.
- Output is tabular.
- Column headers include units of measurement.
- If `FILE` is present, the table is written there and a note is emitted telling the user where it went.
- If `FILE` is omitted, the table is written to standard output.

Current output format is TSV.

## GTK Browser

In the GTK browser, `STUDY` uses the existing Observer and Study dialog workflow.

- If no Observer exists yet, one is created.
- The observed variables are added to the active Observer.
- If `VARY` is omitted, the Observer is populated and no Study dialog is opened.
- If `VARY` is present, the Study dialog is opened with the variable, bounds, stepping mode, distribution, and optional `RUN` method pre-populated.
- Solving then proceeds through the existing interactive Study dialog behavior.

Current GTK limitation:

- `FILE` is not yet implemented as an export path in the GTK browser. The GUI currently reports this and keeps the results in the Observer.

## Tcl/Tk

The METHOD `STUDY` statement is currently not implemented in the Tcl/Tk GUI.
