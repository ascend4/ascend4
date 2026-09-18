[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/ascend4/ascend4/ubuntu.yml?label=Ubuntu%20build)](https://github.com/ascend4/ascend4/actions/workflows/msys2.yml)
[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/ascend4/ascend4/msys2.yml?label=MSYS2%20build)](https://github.com/ascend4/ascend4/actions/workflows/msys2.yml)
[![Codecov](https://img.shields.io/codecov/c/github/ascend4/ascend4?label=CUnit%20test%20coverage)](https://app.codecov.io/gh/ascend4/ascend4)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/4d3d0850360a4809abdaaabc0ea76ca2)](https://app.codacy.com/gh/ascend4/ascend4/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

ASCEND is a free open-source software program for solving small to very large mathematical models. ASCEND can solve systems of non-linear equations, linear and nonlinear optimisation problems, and dynamic systems expressed in the form of differential/algebraic equations.

Please see website [ascend4.org](https://ascend4.org/Main_Page) for more information. There is an [ASCEND Overview](https://ascend4.org/ASCEND_overview) with more information about [ASCEND capabilities](https://ascend4.org/ASCEND_capabilities). Alternatively, you can look at some [screenshots](https://ascend4.org/PyGTK_Screenshots) or some [example problems](https://ascend4.org/Worked_examples) solved using ASCEND. Then look at the [User Documentation](https://ascend4.org/Category:Documentation) and [Developer's Manual](https://ascend4.org/Developer%27s_Manual).

We are in the process of migrating our code to Github, and updating our [MediaWiki site](https://ascend4.org/Main_Page).

## Python tests

After building, run `./a4 pytest` (or select files, such as
`./a4 pytest pygtk/test/ -v`). The launcher sets up ASCEND's runtime environment
and always uses a private Xvfb display, even when a desktop display is available,
so GUI tests do not open windows on your desktop. On Debian/Ubuntu, install the
display tools with `sudo apt-get install xvfb xauth`. Missing display tools cause
an explanatory error; the launcher does not silently fall back to your desktop.

Each selected test file runs sequentially in a fresh Python subprocess. ASCEND's
model library is process-global, so this prevents conflicting model definitions
and stale native handles from leaking between files. Tests within a file still
share their usual module/class fixtures; session fixtures run once per worker.
No additional pytest plugins need to be installed. Selection (`-k`, `-m`, node
IDs), fail-fast options, tracebacks and JUnit XML are handled by pytest, with
worker results combined into one report. A crashed worker is reported as an
error, and later files still run unless fail-fast was requested.

Use `./a4 pytest --no-isolation ...` to reproduce shared-state issues in one
process. Isolation is also disabled for `--pdb`, `--trace`, and the launcher's
GDB/Valgrind modes so those tools attach to the process actually running tests.
`python3 -m pytest` does not enable ASCEND's isolation or private display by itself.

For interactive GUI debugging, explicitly opt in with
`./a4 pytest --visible pygtk/test/ -v`. Put `--visible` before pytest arguments;
it uses the current display without starting Xvfb. Other `a4` commands are
unaffected. Run `scons a4` after changing the launcher template, `a4.in`.
