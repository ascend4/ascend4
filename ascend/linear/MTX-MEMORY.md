# Matrix cleanup symbol collision {#sec:mtx-memory}

## Diagnosis {#sec:mtx-memory-diagnosis}

On Linux, ASCEND's exported `mtx_destroy(mtx_matrix_t)` collided with the
C11 mutex function `mtx_destroy(mtx_t *)` exported by glibc. Python loads
libc before loading ASCEND; ELF lookup then binds cleanup calls in
`libascend.so` and solver plugins to libc's mutex function. ASCEND matrix
storage is never released by that call. This is an ASCEND symbol-namespace
bug, independent of IDA's DENSE/KLU choice.

The original build was fboard2 commit
`885290c75513f54e63a26953578886291e7c2e7d`, with MALLOC_DEBUG disabled.
`LD_DEBUG=bindings` on the ordinary CLI invocation reports:

```text
binding file libascend.so.1 to libc.so.6: normal symbol `mtx_destroy'
binding file libqrslv_ascend.so to libc.so.6: normal symbol `mtx_destroy'
```

The actual trace contains absolute filesystem paths. An `nm -D
--defined-only` comparison finds `mtx_destroy` as the sole shared export
between this build of libascend and the local libc.

The [glibc C11 mutex documentation](https://sourceware.org/glibc/manual/latest/html_node/ISO-C-Mutexes.html)
confirms that the standard function has a different purpose and argument
type. An ASCEND matrix pointer is not a C11 mutex pointer.

## Measurements {#sec:mtx-memory-measurements}

The fixed-parameter kinetics case `rho3713_nb65_X28` has 11,331 IDA unknowns
and about 35,411 Jacobian entries. KLU reports about 6.8 MB of peak factor
storage. The original full-horizon run takes 275.71 s and reaches
7,346,264 KiB peak RSS (7.006 GiB), completing all 2,379 simulated seconds.

A phase probe exits at entry to `runmodel._run_integration`, after the
initial steady solve but before IDA setup. It records about 3.0 GiB RSS.
Massif on that initialisation path reports about 3.50 GB peak live heap:

| Allocation | Live bytes at sampled peak | Share |
|---|---:|---:|
| Matrix permutation vectors | 1,634,943,584 | 46.72% |
| Matrix row headers | 816,817,448 | 23.34% |
| Matrix column headers | 816,817,448 | 23.34% |
| Pools, predominantly matrix storage | 168,528,096 | 4.82% |
| Other allocations | 62,187,981 | 1.78% |

The dominant path is `qrslv_iterate -> find_next_unconverged_block ->
reorder_new_block -> slv_spk1_reorder_block -> mtx_set_order`. Each block
reorder creates a temporary sparse matrix with whole-system index arrays.
The function calls its destructor, but dynamic lookup sends that call to
libc. Repeated linear-sized allocations accumulate; no single giant dense
Jacobian is needed to explain the memory use.

As a diagnostic only, preloading libascend ahead of libc reduces the
pre-integration RSS to about 73 MiB. This load-order experiment is not a
production workaround: it could misdirect an actual C11 mutex caller in
the opposite direction. The durable fix must give ASCEND a distinct symbol.
The full diagnostic preload run takes 273.45 s and peaks at 116,048 KiB
RSS (113.33 MiB): a 63.3-fold reduction from the original 7.006 GiB.
The complete output TSV is byte-for-byte identical to the original. This
isolates the symbol binding change without changing numerical equations or
integration settings. The actual source-fix measurements are recorded below.

## Why the existing memory tests missed it {#sec:mtx-memory-tests}

A C executable linked directly against libascend commonly places ASCEND
before libc in lookup order. A direct-linked fixture here resolves the old
destructor to libascend; a Python-loaded shared-library fixture resolves it
to libc. MALLOC_DEBUG checks allocation accounting within the process under
test, and therefore can pass when that process resolves the correct cleanup
function. A different loading path is required to expose this bug.

The regression in `test/test_mtx_symbol_binding.py` checks the exported
namespace and builds a small dlopen client. It checks the address of the
ASCEND destructor and exercises both matrix cleanup
and the C11 mutex API in the same translation unit. The export check fails
against the original build. The earlier diagnostic fixture independently
confirmed that the incorrectly resolved function address equals libc's.

## Fix and validation {#sec:mtx-memory-fix}

The branch `mtx-destroy-namespace` renames the exported function and all
in-tree callers to `asc_mtx_destroy`. There is deliberately no unprefixed
compatibility export or macro alias: the old name belongs to the standard
mutex API. Rebuild libascend, solver plugins, and external callers together.
External source callers must update the function name; old binary plugins
must not be reused with this API change.

Build from the ASCEND checkout with the existing build configuration:

```sh
scons -j6
scons -j6 test/test
python3 -m unittest discover -s test -p test_mtx_symbol_binding.py
./a4 cutest 'linear_*' solver_qrslv solver_lrslv integrator_ida
```

Large raw logs, heap profiles and generated model wrappers are temporary
artifacts. This note and the small regression are the durable record. The
kinetics repository retains the scripts that generate the measured case.

## Validation record {#sec:mtx-memory-validation}

- Full ASCEND build succeeds with the existing configured solver set.
- Two Python/ELF regressions pass, including a dlopen caller and C11 mutex
  operations in the same translation unit as ASCEND matrix cleanup.
- `./a4 cutest 'linear_*' solver_qrslv solver_lrslv integrator_ida` passes
  96 tests and 2,950 assertions across six suites, with no failures/skips.
- The change to all 41 implementation/caller files is mechanically verified
  as the exact destructor-token substitution, preserving original line
  endings; the public header additionally explains the naming requirement.

The memory-enabled C tests' historical pass is consistent with the observed
link-order difference. The direct-linked diagnostic resolves the legacy
name to libascend, whereas the dynamically loaded client resolves it to
libc. This diagnosis does not require enabling MALLOC_DEBUG again.


### Full-case source-fix result {#sec:mtx-memory-full-case}

After rebuilding on `mtx-destroy-namespace`, the ordinary CLI invocation
without LD_PRELOAD completes the same case in **271.77 s** at **115,932 KiB
peak RSS (113.21 MiB)**. This is a **63.37-fold memory reduction** (98.42%)
from 7,346,264 KiB. All 2,379 simulated seconds complete; the physical and
packing checks pass, there are zero solver-error messages, and the output
TSV is **byte-for-byte identical** to the original run. Runtime differences
across these single measurements are small; no additional speedup is claimed.
`a4 solvers` also loads the configured solver set and reports IDA with KLU.

The currently running Gadi packing study `unification_u4_packing_885290c7`
uses the original runtime and its larger memory allocation. Keep it on that
runtime through completion. Apply the namespace fix and rebuild before a
subsequent campaign. The 113-MiB measurement is for one 65-cell case; use it
as evidence that the leak is removed, not a universal bound for every model.
