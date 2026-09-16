# Running clang-tidy on ASCEND

clang-tidy analyses C/C++, not Python or ASCEND models. Its checks and
severity scheme differ from SonarCloud's impact ratings; a clean clang-tidy
run does not imply a clean Sonar quality gate. See the
[LLVM documentation](https://clang.llvm.org/extra/clang-tidy/).

## Use the real build flags

Configure/build ASCEND normally, with the optional solvers you want checked.
Then request SCons's opt-in compilation-database target:

```sh
scons -Q compile_commands.json
clang-tidy -p . ascend/system/lp_export.c \
  --checks='-*,clang-analyzer-core.*,clang-analyzer-unix.*,bugprone-*'
```

The database includes the include paths, definitions and flags of each
build environment, including solver/test clones. Generating it does not
force an object rebuild. Keep generated headers/sources from a normal build
available. `compile_commands.json` is machine-specific and ignored by Git.
This was tested with SCons 4.5.2 and Ubuntu clang-tidy 18.1.3.

For a branch-wide pass, run from the repository root and choose the actual
PR base (here `origin/python3`):

```sh
git diff --name-only origin/python3...HEAD -- '*.c' '*.cpp' |
  xargs -r -P 4 -n 1 clang-tidy -p . \
    --checks='-*,clang-analyzer-core.*,clang-analyzer-unix.*,clang-analyzer-cplusplus.*,bugprone-*' \
    > /tmp/ascend-clang-tidy.log 2>&1
```

This selects committed branch changes, not untracked/new files. Check those
explicitly too. Headers are analysed through their translation units.
Diagnostics cover whole files, so some warnings predate the branch.
Neither command uses `--fix`: review suggested changes before applying them.
Check for compiler errors in the log; a file that failed to parse was not
successfully analysed. A zero exit status alone does not mean no warnings.

An additional complexity pass can help with Sonar's maintainability findings:

```sh
clang-tidy -p . ascend/system/lp_export.c ascend/compiler/typedef.c \
  --checks='-*,readability-function-cognitive-complexity' \
  --config='{CheckOptions: {readability-function-cognitive-complexity.Threshold: 25, readability-function-cognitive-complexity.IgnoreMacros: true}}'
```

Ignoring macro expansions in this **complexity metric** avoids counting
CUnit assertion internals or allocation/TRY macros as handwritten control
flow. It does not disable any static-analyser safety check. Clang and Sonar
still need not report identical complexity scores.

## PR 78 safety pass, September 2026

The [SonarCloud PR report](https://sonarcloud.io/project/issues?id=ascend4_ascend4&pullRequest=78)
snapshot contained 40 HIGH maintainability findings, one HIGH security
finding and two BLOCKER reliability findings. The work prioritised the
reliability/security paths and a bounded set of maintainability refactors:

- **Shared LP export:** validate signed dimensions before allocation and
  indexing, reserve the objective row, reject invalid mapped indices and
  enforce the allocated nonzero capacity when writing. Separate domain
  preparation, sparse allocation/bounds/coefficients and objective evaluation
  into small helpers. This addresses the two reported heap-access paths and
  makes their preconditions explicit; it is not evidence that a normal,
  correctly prepared LP previously reached those invalid states.
- **System analysis:** correct a definite wrong-pointer diagnostic for an
  all-fixed logical relation; also make the independent-variable pointer
  guard explicit instead of relying on repeated list-length calls.
- **Conditional case lists:** correct reversed source/destination arguments
  when copying existing case numbers during list growth. An 85-WHEN
  regression crosses both the 40- and 80-entry boundaries. Check allocation
  and capacity overflow and propagate failure instead of using a NULL or
  partially populated list.
- **PSA worker invocation:** require an allowed task, an integer cell count
  and a supported stencil at the callable API boundary, and bind arguments
  using `--option=value`. The original CLI already used integer parsing and
  an argument list with no shell, so the reported command-injection exploit
  was not demonstrated. The additional checks protect direct callers too.
- **Maintainability:** split declarative-loop distribution and Gurobi's
  LP/MIP callbacks; reduce LP-export complexity; replace the six reported
  C++ null-pointer literals with `nullptr`.

The initial broad clang-tidy pass covered all 28 changed C/C++ translation
units relative to `origin/python3`; the new LP-export and case-growth tests
were checked separately. No suppression comments were added to close Sonar issues.

This is **not a claim that every HIGH finding has been fixed**. Larger
TABLE/compiler routines, the Gurobi solve/test harness and Python showcase
validation/plotting functions still have complexity findings. Broader
clang-tidy warnings also remain, including paths dependent on legacy list
invariants and CUnit fatal-assert modelling.
They need individual review, not blanket suppression or mechanical fixes.
Only a subsequent Sonar analysis can confirm which findings it closes.

## Regression checks

```sh
scons -Q -j4 ascend solvers ascxx test
./a4 cutest system_lp_export system_cond_config system_link compiler_for_distribution compiler_tables system_der solver_qrslv solver_highs
./a4 cutest solver_gurobi
./a4 cutest solver_cmslv
./a4 script models/psa/test/test_psa_dynamic.py
./a4 pytest test/test_array_indexing.py -q
./a4 script test/test_instantiation_errors.py
./a4 script test/test_derivative_interfaces.py
./a4 script /usr/bin/valgrind --quiet --error-exitcode=97 --leak-check=full \
  ./test/test system_lp_export system_cond_config system_link.fixed_logrelation
```

The C groups passed 211, 72 and 8 tests respectively (291 total);
dynamic PSA passed 10.
Wrapper checks passed 14 tests with one optional-integrator skip. Run these
Python suites in separate processes: the shared ASCEND Library can otherwise
conflict when `system.a4l` is loaded before `ivpsystem.a4l`.

The five focused tests passed Valgrind with no reported memory errors/leaks
and no ASCEND-tracked allocations remaining. The LP tests require neither
optional LP adapter nor a commercial licence. Gurobi's licensed tests need
network access with the current licence configuration: a sandboxed run
failed with `GRB_ERROR_NETWORK` (10022), then passed when network access
was allowed. This is not a reason to skip Gurobi tests silently.
