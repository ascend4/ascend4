# METHOD `STUDY`

`STUDY` is a METHOD-language statement for preparing or executing a parametric study from inside a model method such as `on_load`.

## Syntax

```ascend
STUDY observed_var_list
    [VARY var FROM lower TO upper (STEPS n [LINEAR|LOG] | STEP delta | RATIO r)]
    [RUN method_name]
    [NOW]
    [FILE "path"];
```

Examples:

```ascend
STUDY accel_required, time_run;

STUDY accel_required, time_run
    VARY time_run FROM 2 {s} TO 6 {s} STEPS 4
    NOW
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
- `NOW` is optional.
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

In the `ascxx` / CLI path, `STUDY` is interpreted in two different ways depending on whether `VARY` is present.

- If `VARY` is present, the requested cases are solved in sequence.
- If `RUN` is present, the named method is run before each study point.
- `NOW` is accepted but has no additional effect in the CLI path; varying studies already execute immediately.
- Output is tabular.
- Column headers include units of measurement.
- If `FILE` is present, the table is written there and a note is emitted telling the user where it went.
- If `FILE` is omitted, the table is written to standard output.
- If `VARY` is omitted, the observed variables are stored as the model's default post-solve print list.
  `a4 run` then prints them after the main solve using `name = value` formatting, but only when no explicit `-p/--print` arguments were supplied.
  Explicit `-p` arguments take precedence over this model-defined list.
- If any `STUDY ... VARY ...` request is encountered during the run, the deferred no-`VARY` print action is suppressed for that run.

Current output format is TSV.

## GTK Browser

In the GTK browser, `STUDY` uses the existing Observer and Study dialog workflow.

- If no Observer exists yet, one is created.
- The observed variables are added to the active Observer.
- If `VARY` is omitted, the Observer is populated and no Study dialog is opened.
- If `VARY` is present, the Study dialog is opened with the variable, bounds, stepping mode, distribution, and optional `RUN` method pre-populated.
- If `VARY` is present and `NOW` is supplied, the pre-populated study is executed immediately without waiting for the user to press `OK`.
- Solving then proceeds through the existing interactive Study dialog behavior.

Current GTK limitation:

- `FILE` is not yet implemented as an export path in the GTK browser. The GUI currently reports this and keeps the results in the Observer.

Not yet implemented:

- `ON_ERROR STOP | SKIP | CONTINUE`

## Tcl/Tk

The METHOD `STUDY` statement is currently not implemented in the Tcl/Tk GUI.
