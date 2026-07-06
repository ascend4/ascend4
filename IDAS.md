---
title: ASCEND IDAS Integrator Plan
number-sections: true
autocite: doi
---

# Purpose

This note maps a practical path for adding SUNDIALS IDAS support to ASCEND.
The first target is forward sensitivities for smooth, non-event DAE
integrations, with enough C++/Python API exposure to use those sensitivities
from the existing standalone A4SQP shooting driver for the TGA examples.

The second target, event-handling models, is deliberately separated. IDAS can
integrate sensitivities on smooth segments, but ASCEND must define how
sensitivities pass through `WHEN`, boundary crossing, and reinitialization
logic. That is a modelling/API problem as much as a SUNDIALS problem.

# Background

The current `solvers/ida` package builds one ASCEND integrator plugin named
`ida` and registers one integrator engine:

```text
package_load('ida')
  -> INTEGRATOR IDA
```

The current SCons detection links against `sundials_ida`, and the current C
plugin registers a single `IntegratorInternals` record named `IDA`.

The proposed design is to prefer the SUNDIALS IDAS library at build time when
available. IDAS is a superset of IDA for DAE integration: it provides ordinary
IDA-style DAE integration calls and additional forward/adjoint sensitivity
interfaces. The SUNDIALS documentation describes IDAS as extending ordinary IVP
integration with sensitivity calls such as `IDASensInit`, `IDASetSensParams`,
and `IDAGetSens` [SUNDIALS IDAS FSA documentation][sundials-idas-fsa]. It also
states that IDAS provides a superset of IDA functionality with forward and
adjoint sensitivity analysis added to the main integrator
[SUNDIALS IDAS introduction][sundials-idas-intro].

# Build And Registration Strategy

## Backend Selection

At build time, detect IDAS first, then IDA:

```text
if SUNDIALS IDAS is available:
    compile ida plugin against sundials_idas
    define ASC_IDA_BACKEND_IDAS
    register IDA and IDAS
elif SUNDIALS IDA is available:
    compile ida plugin against sundials_ida
    define ASC_IDA_BACKEND_IDA
    register IDA only
else:
    skip ida plugin
```

Do not link both `sundials_ida` and `sundials_idas` into the same plugin.
Both libraries expose ordinary IDA symbols such as `IDACreate`, `IDASolve`, and
`IDAFree`, so a single selected backend keeps symbol resolution simple.

On a system with IDAS, `INTEGRATOR IDA` should still work and should not enable
sensitivities by default. `INTEGRATOR IDAS` should mean that the user wants a
sensitivity-capable integration whose sensitivities can be queried after the
simulation, for the configured observed variables and sensitivity parameters.
It should fail clearly if the plugin was built against plain IDA.

## Fallback IDA Cost When Linked Against IDAS

Using `libsundials_idas` as the backend for ordinary `INTEGRATOR IDA` should
not impose the main sensitivity memory or runtime costs, provided ASCEND does
not call the IDAS sensitivity initialization functions.

The important distinction is:

```text
IDAS backend loaded, sensitivity calls not used:
    ordinary DAE integration path
    slightly larger shared library
    no Ns-by-N sensitivity vector allocation

IDAS backend with IDASensInit called:
    state integration plus sensitivity integration
    memory and runtime scale with sensitivity parameter count Ns
```

SUNDIALS documents `IDASensInit` as the call that activates forward
sensitivity computations and allocates internal sensitivity memory. The
documented additional workspace scales with the number of sensitivity
parameters and the DAE state size. In simplified terms, the extra real
workspace is proportional to:

```text
(maxord + 5) * Ns * N
```

where `Ns` is the number of sensitivity parameters and `N` is the DAE state
dimension. Additional workspace is needed when vector sensitivity tolerances
are used.

Therefore the intended compatibility behavior is:

```text
INTEGRATOR IDA:
    may be backed by libsundials_idas
    must not call IDASensInit
    should behave like current IDA apart from a modest library footprint change

INTEGRATOR IDAS:
    calls IDASensInit only when sensitivity parameters are configured
    pays the sensitivity memory/runtime cost only in sensitivity mode
```

## SCons Changes

Extend `solvers/ida/SConscript` along these lines:

1. Replace `_default_sundials_libs(major)` with a backend-aware helper:

   ```python
   def _default_sundials_libs(major, backend):
       first = 'sundials_idas' if backend == 'idas' else 'sundials_ida'
       libs = [first, 'sundials_nvecserial']
       ...
       return libs
   ```

2. Add `idas_test_text` using `<idas/idas.h>` and `IDACreate`.

3. Add `CheckIDAS(context)` before `CheckIDA(context)`.

4. If `CheckIDAS` passes, set:

   ```python
   env['SUNDIALS_DAE_BACKEND'] = 'idas'
   env.AppendUnique(CPPDEFINES=['ASC_IDA_BACKEND_IDAS'])
   env.AppendUnique(SUNDIALS_LIBS=_default_sundials_libs(major, 'idas'))
   ```

5. If IDAS fails but IDA passes, set:

   ```python
   env['SUNDIALS_DAE_BACKEND'] = 'ida'
   env.AppendUnique(CPPDEFINES=['ASC_IDA_BACKEND_IDA'])
   env.AppendUnique(SUNDIALS_LIBS=_default_sundials_libs(major, 'ida'))
   ```

6. Keep the package name and output library name as `ida`.

This preserves:

```text
package_load('ida')
```

as the single user-facing load point.

## Integrator Registration

Add a second engine ID and registration record when IDAS is available:

```c
static const IntegratorInternals integrator_ida_internals = {
    integrator_ida_create,
    integrator_ida_params_default,
    integrator_ida_analyse,
    integrator_ida_initialise,
    integrator_ida_solve,
    integrator_ida_write_matrix,
    integrator_ida_debug,
    integrator_ida_free,
    INTEG_IDA,
    "IDA"
};

#ifdef ASC_IDA_BACKEND_IDAS
static const IntegratorInternals integrator_idas_internals = {
    integrator_ida_create,
    integrator_idas_params_default,
    integrator_ida_analyse,
    integrator_ida_initialise,
    integrator_ida_solve,
    integrator_ida_write_matrix,
    integrator_ida_debug,
    integrator_ida_free,
    INTEG_IDAS,
    "IDAS"
};
#endif
```

`integrator_ida_create` can be shared if `IntegratorIdaData` stores an internal
mode flag:

```c
typedef enum{
    ASC_IDA_MODE_IDA,
    ASC_IDA_MODE_IDAS
} AscIdaMode;
```

The mode can be set from the selected `IntegratorInternals` name during create
or initialise. The important behavior is:

```text
IDA engine:
    use the IDAS backend if that is what was built
    do not call sensitivity initialization

IDAS engine:
    require ASC_IDA_BACKEND_IDAS
    enable sensitivity options and output
```


## Current Implementation Status

As of this implementation pass, the first build/registration layer is in place:

1. `solvers/ida/SConscript` checks for SUNDIALS IDAS first and falls back to
   SUNDIALS IDA if IDAS is unavailable.
2. When IDAS is found, `solvers/ida/libida_ascend.so` links against
   `libsundials_idas` and compiles the IDA wrapper with
   `ASC_IDA_BACKEND_IDAS`.
3. `package_load('ida')` now registers both `INTEGRATOR IDA` and
   `INTEGRATOR IDAS` from the same plugin when the backend is IDAS.
4. The existing `IDA` behavior still uses the ordinary IDA solve path and does
   not call `IDASensInit`.
5. The core integrator API now has storage and accessors for:
   - explicit sensitivity-driver parameter instances;
   - row-major observed-output sensitivity matrices;
   - C++/Python methods for setting parameter instances and querying the stored
     sensitivity matrix.

The implementation has been built and checked with:

```text
scons -j7
./a4 solvers
./a4 cutest integrator_ida
./a4 cutest integrator_idas
./a4 run models/twinslabs_der.a4c --model twinslabs_der --engine IDAS --output /tmp/twinslabs_der_idas.tsv --no-test
```

On the development machine used for this pass, `./a4 solvers` reported both:

```text
IDA: SUNDIALS 6.4.1
IDAS: SUNDIALS 6.4.1
```

The existing `integrator_ida` CUnit suite passed with 44 selected tests and 937
assertions. The current-syntax `twinslabs_der` example runs through the
`./a4 run` pathway and produced byte-identical output for `--engine IDA` and
`--engine IDAS` over the 10 h default integration.

The IDAS-specific regression coverage is now in a separate `integrator_idas`
CUnit suite. Its first test, `twinslabs_hc_sensitivity`, runs
`models/twinslabs_der.a4c` with `h_c` as the configured sensitivity parameter
and compares the final temperatures and final sensitivities against the
closed-form two-exponential solution. The test does not assert against finite
differences.

The same source file also contains a test-only `twinslabs_der_exact` model that
encodes the closed-form solution as ordinary ASCEND relations. The
`integrator_idas` CUnit test builds that analytical model as a solver system,
uses ASCEND's `relman_diff3` relation-gradient path to calculate
$\partial T_1/\partial h_c$ and $\partial T_2/\partial h_c$, and compares those
ASCEND-derived analytical derivatives with the IDAS sensitivities. SymPy is
useful for deriving or checking the formula offline, but it is not a test
dependency.

The first smooth, non-event sensitivity path is now prototyped:

1. `IntegratorIdaData` stores `sens_p`, `sens_p_nominal`, `sens_pbar`,
   `sens_plist`, `sens_y`, and `sens_yp`.
2. For `INTEGRATOR IDAS`, configured sensitivity parameters are read from
   assigned real ASCEND instances after consistent initial-condition setup.
3. `IDASensInit(..., NULL, sens_y, sens_yp)` enables IDAS' internal
   difference-quotient sensitivity residuals.
4. `IDASetSensParams(..., sens_p, sens_pbar, sens_plist)` supplies actual
   ASCEND internal-unit parameter values and order-of-magnitude scaling.
5. The existing IDA residual callback now copies the current `sens_p[i]` values
   into the selected ASCEND parameter instances before `relman_eval`.
6. After each successful output point, `IDAGetSens` is used to populate the
   stored observed-output sensitivity matrix for observed items that are direct
   state variables.

The prototype intentionally rejects sensitivity mode when IDA/IDAS event roots
are active. It also does not yet apply ASCEND's chain-rule layer for derived
observations; observed variables that are not direct state variables are stored
as unavailable (`NaN`) in the current matrix.

The current `integrator_idas` regression uses `models/twinslabs_der.a4c`,
`INTEGRATOR IDAS`, and `h_c` as the sensitivity parameter. The CUnit test
asserts against analytical final temperatures from the closed-form
two-exponential solution. For sensitivities, it asks ASCEND to differentiate
the closed-form `twinslabs_der_exact` relations using `relman_diff3`, then
compares IDAS output with those ASCEND-derived analytical derivatives. The
final 10 h sensitivities reported for `OBSERVE T_1, T_2` are approximately:

```text
dT_1/dh_c = 0.675723
dT_2/dh_c = 0.083816
```

A separate central finite-difference rerun with
`h_c = 5 +/- 0.001 {W/m^2/K}` was used only as an independent smoke check while
developing the prototype. It gave:

```text
dT_1/dh_c = 0.675731
dT_2/dh_c = 0.083829
```

# Non-Event IDAS Milestone

This is the first implementation target.

## Mathematical Scope

Support models whose integration path is smooth over the requested time range:

- no boundary root crossing;
- no `WHEN` branch change that rebuilds or reinitializes the DAE;
- no discontinuous assignment to state variables during the integration;
- no event-time sensitivity needed.

For a DAE

$$
F(t, y, \dot{y}, p) = 0
$$

IDAS forward sensitivities integrate, for each selected parameter `p_j`,
the linearized sensitivity DAE for:

```text
S_j  = dy/dp_j
Sd_j = dydot/dp_j
```

The SUNDIALS IDAS documentation formulates the DAE with parameters and
initial conditions depending on parameters, and describes extracting
sensitivities after each successful `IDASolve` call using `IDAGetSens`,
`IDAGetSens1`, `IDAGetSensDky`, or `IDAGetSensDky1`
[SUNDIALS IDAS FSA documentation][sundials-idas-fsa].

## Sensitivity Parameter Selection

Add an integrator-level parameter list separate from ordinary observed values.
These are the sensitivity parameters $p_j$: the scalar inputs whose effect on
the trajectory and observations should be tracked. The API needs to identify
ASCEND instances that are treated as parameters for IDAS:

```c
int integrator_set_sensitivity_parameters(
    IntegratorSystem *sys,
    struct Instance **instances,
    int n
);

int integrator_get_num_sensitivity_parameters(
    IntegratorSystem *sys
);

struct Instance *integrator_get_sensitivity_parameter(
    IntegratorSystem *sys,
    const long i
);
```

The C++ API can mirror this:

```c++
void clearSensitivityParameters();
void addSensitivityParameter(const Instanc &inst);
long getNumSensitivityParameters();
Instanc getSensitivityParameter(const long &i);
```

The Python API then naturally becomes:

```python
integrator.clearSensitivityParameters()
integrator.addSensitivityParameter(param_inst)
integrator.getNumSensitivityParameters()
integrator.getSensitivityParameter(i)
```

For the first milestone, require each sensitivity instance to be a real scalar.
Arrays can be handled by adding each scalar child explicitly. Selected
parameters should also be fixed inputs, or at least not active DAE unknowns. If
an instance is simultaneously part of the dynamic state/algebraic unknown vector
and an externally perturbed sensitivity parameter, IDAS and ASCEND would be
trying to own the same value in incompatible ways.

IDAS itself provides the low-level parameter hooks. `IDASensInit` receives
`Ns`, the number of sensitivity parameters/sensitivity systems. `IDASetSensParams`
then supplies:

```c
IDASetSensParams(void *ida_mem, realtype *p, realtype *pbar, int *plist);
```

where `p` is the parameter value array, `pbar` gives parameter scaling for
internal difference-quotient sensitivity residuals, and `plist` optionally maps
the sensitivity systems to entries in `p`. ASCEND's task is to map selected
ASCEND instances to those `p` entries and keep the values synchronized before
IDAS initialization/reinitialization and during residual evaluation.

For optimisation wrappers such as the TGA A4SQP script, the optimisation free
variables are the natural sensitivity parameters. They do not need to originate
from ASCEND model source initially; the Python/C++ integrator API can provide
them explicitly. Later, ASCEND model metadata such as `fit.contract` could be
used as a convenience layer, but it is not required for the first prototype.

## Parameter Value Bridge

IDAS needs access to the current parameter values through `IDASetSensParams`
when using the internal difference-quotient sensitivity residual. ASCEND should
store a dense `realtype *p` array in `IntegratorIdaData`, filled from the
selected instances before each IDAS initialization and refreshed before
reinitialization. This array is the object IDAS sees as the parameter vector.
The ASCEND instances remain the objects the relation evaluator sees.

That separation creates the parameter-value bridge problem: if IDAS perturbs
`p[j]` while estimating sensitivity residuals, ASCEND residual evaluation must
observe the same perturbed value. Otherwise IDAS will ask for a residual at
$p_j + \delta$, but ASCEND will still evaluate relations using the unperturbed
instance value.

Schematic IDAS internal difference-quotient evaluation is:

$$
F_p \approx
\frac{F(t, y, \dot{y}, p + \delta e_j) - F(t, y, \dot{y}, p)}{\delta}
$$

For ASCEND, the second argument list in that expression is not enough. The
relation manager computes $F$ from model instances, so the selected ASCEND
parameter instance must also be made to contain $p_j + \delta$ for the perturbed
call.

### Simple Model Example

Consider a first-order DAE model with one parameter `k`:

```text
MODEL first_order;
    x IS_A solver_var;
    k IS_A solver_var;
    t IS_A solver_var;
    dyn: der(x) = -k*x;

    METHODS
        METHOD specify;
            FIX k, t;
            FREE x, der(x);
        END specify;
    END METHODS;
END first_order;
```

The residual is:

$$
F = \dot{x} + kx
$$

For sensitivity with respect to `k`, IDAS needs the effect of changing `k` on
that residual. If `p[0]` is changed from `0.4` to `0.400001` inside an IDAS
finite-difference call but the ASCEND instance `sim.k` still contains `0.4`,
ASCEND evaluates the same residual twice and the computed $\partial F/\partial k$
will be zero or wrong. The bridge must copy the current `p[0]` value into
`sim.k` before the relation manager evaluates `dyn`.

### TGA/A4SQP Example

For a toy TGA optimisation, the Python driver may set a candidate parameter and
then request sensitivities:

```python
sim.k0.setRealValue(candidate[0])
sim.ea.setRealValue(candidate[1])

integrator.clearSensitivityParameters()
integrator.addSensitivityParameter(sim.k0.getInstance())
integrator.addSensitivityParameter(sim.ea.getInstance())
integrator.analyse()
integrator.solve()

sens = integrator.getCurrentObservationSensitivities()
```

At initialization, ASCEND reads `sim.k0` and `sim.ea` into:

```text
p[0] = value(sim.k0)
p[1] = value(sim.ea)
```

During an internal IDAS sensitivity residual evaluation, IDAS may temporarily
use a perturbed `p[0]`. The bridge has to make the model instance values match:

```text
sim.k0 := p[0]
sim.ea := p[1]
evaluate ASCEND residuals
```

Without that copy-in step, IDAS can still integrate the base trajectory, but
its sensitivity equations do not correspond to the ASCEND model that the user
is trying to fit.

### Prototype Implementation

The lowest-risk first implementation is a copy-in bridge in the IDA residual
callback:

1. build `enginedata->p`, `enginedata->pbar`, and `enginedata->plist` from
   `integrator_get_sensitivity_parameter(sys, i)` before `IDASensInit`;
2. call `IDASetSensParams(ida_mem, enginedata->p, enginedata->pbar,
   enginedata->plist)`;
3. at the start of every residual callback, before evaluating ASCEND relations,
   copy `enginedata->p[i]` into the corresponding selected ASCEND instance;
4. after `IDASolve`, `IDACalcIC`, and any failed solve exit, restore the model
   instances to the nominal parameter values that should be visible to ASCEND
   reporters and user code.

The storage in `IntegratorIdaData` should include both current IDAS values and
a restoration copy:

```c
typedef struct IntegratorIdaDataStruct {
    ...
    realtype *sens_p;
    realtype *sens_p_nominal;
    realtype *sens_pbar;
    int *sens_plist;
    long sens_np;
} IntegratorIdaData;
```

The residual callback then does, conceptually:

```c
static int integrator_ida_fex(realtype t, N_Vector y, N_Vector ydot,
        N_Vector residual, void *res_data)
{
    IntegratorSystem *integ = res_data;
    IntegratorIdaData *enginedata = integrator_ida_enginedata(integ);

    ida_sync_sensitivity_parameters_from_p(integ, enginedata);

    integrator_set_t(integ, t);
    integrator_set_y(integ, NV_DATA_S(y));
    integrator_set_ydot(integ, NV_DATA_S(ydot));
    relman_eval(...);
}
```

The copy should happen on every residual call, not only once at solve start,
because IDAS may call the residual many times with different temporary
parameter values while approximating sensitivity residuals.

Aliasing `p[i]` directly to ASCEND instance storage would avoid copies, but it
is a riskier second step. It depends on instance storage layout, type stability,
unit conversion expectations, and whether IDAS ever treats the `p` array as
ordinary mutable contiguous storage. A bridge copy is less clever and easier to
validate.

Providing an explicit IDAS sensitivity residual callback is another future
option. In that design ASCEND would compute:

$$
\frac{\partial F}{\partial y} S_j
+ \frac{\partial F}{\partial \dot{y}} \dot{S}_j
+ \frac{\partial F}{\partial p_j}
$$

This avoids relying on IDAS finite differences for $F_{p_j}$, but it requires a
reliable ASCEND API for differentiating relations with respect to arbitrary
fixed parameter instances. That is more work than the first milestone needs.

### Scaling And Units

`pbar` matters for IDAS finite-difference increments and error scaling. ASCEND
stores real values numerically in canonical units, so `pbar` must be a numeric
scale in the same internal unit system as `p`.

A reasonable first heuristic is:

$$
pbar_j = \max(|p_j|, 1)
$$

This is adequate for dimensionless or order-one parameters, but it is weak for
very small kinetic constants, large activation energies, or parameters whose
natural uncertainty scale differs substantially from their magnitude. The API
should later allow the caller to provide explicit parameter scales, for example
from an optimisation driver's variable scaling.

### Consistency And Events

The bridge is necessary but not sufficient for fully correct sensitivities.
Initial sensitivity values must still be consistent with the DAE initial
conditions. The first milestone should assume parameter-independent initial
conditions and start with zero initial sensitivities; models whose initial state
is itself a function of fitted parameters need a later consistent-sensitivity
initialization path.

For event-handling models, the same bridge must be applied around every IDAS
restart and any guard or `WHEN` evaluation that depends on selected parameters.
The first implementation can reasonably reject `IDAS` sensitivities when event
handling is active. Supporting hybrid models later will require restoring
nominal parameter values after failed/aborted event steps, reapplying the bridge
before each `IDAReInit`, and deciding how parameter sensitivities propagate
through reset maps.

## Initial Sensitivities

Initial sensitivities are the hardest non-event detail.

For the DAE:

$$
F(t, y, \dot{y}, p) = 0
$$

ordinary consistent initial conditions satisfy:

$$
F(t_0, y_0, \dot{y}_0, p) = 0
$$

For a sensitivity parameter `p_j`, define:

$$
S_j = \frac{\partial y}{\partial p_j}
$$

$$
\dot{S}_j = \frac{\partial \dot{y}}{\partial p_j}
$$

The initial sensitivity values are consistent when they satisfy the
differentiated DAE and differentiated initialization constraints at `t0`:

$$
\frac{\partial F}{\partial y} S_j
+ \frac{\partial F}{\partial \dot{y}} \dot{S}_j
+ \frac{\partial F}{\partial p_j}
= 0
$$

If the initial dynamic state is independent of `p_j`, zero initial
sensitivities may be correct. If setup methods, fixed/free choices, initial
assignments, or algebraic consistent-initial-condition solves make the initial
state depend on `p_j`, then zero initial sensitivities may be wrong. For
example:

$$
y(0) = p_j
$$

implies:

$$
\frac{\partial y(0)}{\partial p_j} = 1
$$

not zero.

For the first milestone, support two modes:

```text
zero
    yS0 = 0 and ypS0 = 0 for all sensitivity parameters

consistent
    yS0 and ypS0 start at zero, then IDAS computes consistent sensitivity
    initial conditions where supported
```

Expose an option:

```text
sensitivity_initial = ZERO | CONSISTENT
```

Use `IDAGetSensConsistentIC` for diagnostics/extraction after IDAS consistent
IC calculation when using the IDAS backend. If this is not enough for a model
whose initial state is explicitly parameter-dependent, the model should be
declared unsupported for sensitivity mode until ASCEND can differentiate the
initialization method or let users supply initial sensitivity values.

## IDAS Setup Sequence

In the existing IDA setup path, after ordinary `IDACreate`, `IDAInit`,
tolerances, linear solver, optional inputs, root setup, and consistent IC setup,
add the sensitivity initialization only when:

```text
backend == IDAS
and engine mode == IDAS
and number of sensitivity parameters > 0
```

Sequence:

```c
yS0  = N_VCloneVectorArray(Ns, y0);
ypS0 = N_VCloneVectorArray(Ns, y0);

load_initial_sensitivities(yS0, ypS0);

flag = IDASensInit(ida_mem, Ns, IDA_STAGGERED, NULL, yS0, ypS0);
flag = IDASetSensParams(ida_mem, p, pbar, plist);
flag = IDASensEEtolerances(ida_mem);
flag = IDASetSensErrCon(ida_mem, SUNFALSE);
```

Recommended defaults:

```text
sensitivity_method = STAGGERED
sensitivity_residual = INTERNAL_DQ
sensitivity_tolerances = ESTIMATED
sensitivity_error_control = OFF
```

Rationale:

- `STAGGERED` is less intrusive for existing IDA behavior.
- internal difference quotients avoid needing exact ASCEND `dF/dp` work first.
- estimated tolerances are easier than exposing a full sensitivity tolerance
  vector at the start.
- leaving sensitivity variables out of local error control initially prevents
  sensitivity accuracy demands from destabilizing ordinary integration.

## Sensitivity Extraction At Observation Times

The current reporting path records observed values. Add a parallel observed
sensitivity matrix at each sample:

```text
time
observed value i
sensitivity parameter j
d observed_i / d parameter_j
```

At each successful sample point:

1. Call `IDAGetSens(ida_mem, &tret, yS)`.
2. For each observed instance, map it to its integrator state index.
3. Extract `NV_Ith_S(yS[j], state_index)` for each sensitivity parameter.
4. Store a row-major matrix:

   ```text
   observed_sens[i_obs][j_param]
   ```

The first milestone should support any observed instance that maps directly to
an IDAS unknown in the DAE vector. This includes both differential states and
algebraic variables that IDA/IDAS solves as part of $y$; algebraic variables are
not automatically excluded.

## Chain Rule For Derived Observations

A "derived observation" means an observed quantity that is not itself an IDAS
unknown, for example a report-only expression, postprocessed value, assignment
result, or helper variable computed from several solved variables after the DAE
solve. For those values, IDAS can provide sensitivities of the underlying DAE
unknowns, but ASCEND must still apply the observation chain rule.

If the observed value is:

$$
g = g(y, p)
$$

then for sensitivity parameter $p_j$:

$$
\frac{\partial g}{\partial p_j}
= \frac{\partial g}{\partial y} S_j
+ \left.\frac{\partial g}{\partial p_j}\right|_{y}
$$

where:

- $S_j = \partial y / \partial p_j$ is the state/algebraic sensitivity
  returned by IDAS;
- $\partial g / \partial y$ is the derivative of the observation expression
  with respect to the IDAS unknown vector;
- $\left.\partial g / \partial p_j\right|_{y}$ is any direct dependence of
  the observation expression on the sensitivity parameter, holding the IDAS
  unknowns fixed.

This distinction matters because an ASCEND observed instance may fall into one
of several categories:

1. Direct IDAS unknown: the observed instance maps to an entry in the IDAS $y$
   vector. ASCEND can read $S_j$ directly from the IDAS sensitivity vector.
2. Alias or simple model variable with known relation to an IDAS unknown: ASCEND
   may be able to resolve the alias and still use the direct sensitivity.
3. Algebraic variable solved by IDAS: this is still a direct IDAS unknown if it
   is included in the DAE vector, even though it is not differential.
4. Derived expression or postprocessed value: ASCEND must differentiate the
   expression or calculation that defines the observation.

For the first prototype, support categories 1 and 3, and category 2 where the
existing instance/solver-variable mapping already makes it unambiguous. Return
"not available" for category 4 until ASCEND has a reliable expression-level
chain-rule path.

A later implementation could compute derived-observation sensitivities by
reusing ASCEND relation/expression derivative machinery, provided the observed
quantity can be represented as a differentiable expression of solved variables
and selected parameters. Non-smooth observations, conditional expressions, or
postprocessing done outside the ASCEND expression system should remain explicit
unsupported cases unless a user supplies a custom derivative.

### ASCEND Examples

A continuous derived observation can be as simple as an algebraic reporting
variable. In `models/johnpye/iron/firstorder.a4c`, `X`, `reduction_degree`,
`tau`, and `X_error` are derived from the dynamic state and parameters:

```ascend
MODEL firstorder_shrinking_core;
    t IS_A time;
    INDEPENDENT t;

    k_scm IS_A frequency;
    r_core IS_A fraction;
    X IS_A fraction;
    reduction_degree IS_A fraction;
    tau IS_A time;
    X_exact IS_A factor;
    X_error IS_A factor;

    core_ode:
        der(r_core) = -k_scm;
    conversion_eq:
        X = 1 - r_core^3;
    reduction_degree_eq:
        reduction_degree = X;
    tau_eq:
        tau = 1 / k_scm;
    error_eq:
        X_error = X - X_exact;

METHODS
    METHOD observe_default;
        OBSERVE r_core, X, reduction_degree, tau, X_error;
    END observe_default;
END firstorder_shrinking_core;
```

If `X`, `reduction_degree`, `tau`, or `X_error` are present in the IDAS DAE
unknown vector, their sensitivities can be read directly from IDAS. If ASCEND
chooses not to include one of them in the IDAS vector and instead treats it as a
postprocessed observed expression, ASCEND must compute the chain rule from the
underlying IDAS sensitivities.

A second common example is a reporting variable with units conversion or
normalization:

```ascend
MODEL reporting_example;
    T IS_A temperature;
    T_C IS_A factor;
    conversion IS_A fraction;
    conversion_percent IS_A factor;

    temperature_report_eq:
        T_C = T / 1 {K} - 273.15;
    conversion_report_eq:
        conversion_percent = 100 * conversion;

METHODS
    METHOD observe_default;
        OBSERVE T_C, conversion_percent;
    END observe_default;
END reporting_example;
```

These are differentiable derived observations if `T` and `conversion` have
valid IDAS sensitivities. ASCEND can compute their sensitivities with the
observation-expression derivatives.

Selectors are different. A selector can be observed, but it is a discrete mode,
not a smooth derived observation:

```ascend
MODEL selector_observe_example;
    stage IS_A integer;
    modes IS_A set OF symbol_constant;
    mode IS_A selector OF modes DEFAULT 'low';

METHODS
    METHOD on_load;
        modes := ['low', 'high'];
        OBSERVE stage, mode;
    END on_load;
END selector_observe_example;
```

For `mode`, there is no ordinary derivative $\partial mode / \partial p_j$ to
return. If parameters change when a selector switches, that belongs to the
hybrid/event sensitivity problem, not to the smooth observation chain rule. A
first IDAS prototype should either report selector sensitivities as unavailable
or expose only the selector value/event path metadata.

## C API For Sensitivity Output

Add a minimal query surface:

```c
ASC_DLLSPEC int integrator_has_sensitivities(IntegratorSystem *sys);

ASC_DLLSPEC int integrator_get_num_sensitivity_parameters(
    IntegratorSystem *sys
);

ASC_DLLSPEC int integrator_get_current_observation_sensitivities(
    IntegratorSystem *sys,
    double *matrix,
    int nobs,
    int nparams
);
```

The matrix layout should be documented as:

```text
matrix[i_obs * nparams + j_param]
```

Add a lower-level state-vector query only if needed:

```c
ASC_DLLSPEC int integrator_get_current_state_sensitivities(
    IntegratorSystem *sys,
    double *matrix,
    int nstates,
    int nparams
);
```

For error reporting, distinguish:

```text
ASC_INTEG_SENS_NOT_BUILT
ASC_INTEG_SENS_NOT_ENABLED
ASC_INTEG_SENS_EVENT_UNSUPPORTED
ASC_INTEG_SENS_OBS_UNSUPPORTED
```

Exact enum names can follow local conventions.

## C++ And Python API

Extend `ascxx::Integrator`:

```c++
bool hasSensitivities() const;

void clearSensitivityParameters();
void addSensitivityParameter(const Instanc &inst);
long getNumSensitivityParameters();
Instanc getSensitivityParameter(const long &i);

std::vector<std::vector<double> >
getCurrentObservationSensitivities();
```

Expose through SWIG so Python can do:

```python
integrator.clearSensitivityParameters()
for param_path in param_paths:
    integrator.addSensitivityParameter(_instance_by_path(sim, param_path))

integrator.solve()

sens = integrator.getCurrentObservationSensitivities()
```

For reporter-based workflows, add an optional callback hook after observed
values have been recorded:

```c++
virtual int recordObservedSensitivities();
```

That callback can be deferred if it is too invasive. The first Python
implementation can have the reporter call:

```python
integrator.getCurrentObservationSensitivities()
```

inside `recordObservedValues()`.

## Use In The TGA A4SQP Driver

The current `models/johnpye/iron/tga_fit_a4sqp.py` computes gradients by
finite differences around repeated IDA solves. With IDAS sensitivities, the
evaluation path becomes:

```text
one A4SQP objective evaluation:
    set candidate parameters
    run ASCEND/IDAS once for each trial
    collect residual vector r
    collect sensitivity matrix dr/dp from IDAS
    return objective and gradient
```

For least squares:

```text
r_i = sqrt(w_i) * (y_i - y_i_data)

dJ/dp_j = sum_i r_i * dr_i/dp_j
        = sum_i r_i * sqrt(w_i) * dy_i/dp_j
```

Driver changes:

1. Add CLI option:

   ```text
   --gradient fd|idas
   ```

2. When `--gradient idas` is selected:

   - set integrator engine to `IDAS`;
   - add sensitivity instances matching active fit parameters;
   - return both residuals and residual Jacobian from each trial;
   - compute gradient by chain rule.

3. If the integrator reports `ASC_INTEG_SENS_EVENT_UNSUPPORTED`, fall back to
   finite differences unless the user passes:

   ```text
   --gradient-strict
   ```

4. Keep finite differences as the baseline for validation.

Validation test for the driver:

```text
small first-order model
known analytic or finite-difference gradient
compare IDAS gradient with finite difference within tolerance
```

# Event-Handling Models

## Why Events Are Different

For a smooth segment, IDAS integrates sensitivity equations. At an event,
ASCEND changes the mathematical problem:

- a boundary root is crossed;
- a `WHEN` case changes;
- state variables may be reassigned;
- algebraic/differential status may change;
- the DAE may be rebuilt and reinitialized.

The sensitivity state must pass through the event with the same semantics as
the physical state. IDAS cannot infer ASCEND's modelling meaning for a
`WHEN` reinitialization.

For a reset map:

$$
y^+ = R(y^-, p, t_{event})
$$

the sensitivity jump is, schematically:

$$
S^+ =
\frac{\partial R}{\partial y} S^-
+ \frac{\partial R}{\partial p}
+ \text{event-time terms}
$$

If the event time depends on the parameter, the event-time sensitivity must be
computed from the guard equation. For a guard:

$$
h(y, \dot{y}, z, p, t_{event}) = 0
$$

the transition-time derivative `dt_event/dp` comes from differentiating the
guard. Recent hybrid DAE sensitivity work formulates this explicitly and shows
that consistent initialization and state transfer must also be solved for the
sensitivity system at switching points [@doi:10.48550/arXiv.1904.08734].

## Event Policy Classes

ASCEND should not pretend that all events have the same uncertainty behavior.
Introduce event sensitivity policy classes:

```text
continuous
    no state reset; carry sensitivities through, then reinitialize ydot/DAE

clamp_known
    reset selected states to known constants; selected sensitivities become 0

measurement_reset
    reset selected states to measured estimates; sensitivities may become 0,
    but uncertainty/covariance receives a measurement noise injection

map
    reset selected states with explicit differentiable expressions; compute
    dR/dy and dR/dp

unsupported
    disable IDAS gradient for this trajectory and require finite differences
    or smoothing
```

This is separate from deterministic sensitivity. If ASCEND later exposes
uncertainty or covariance, reset policies need an additional process-noise or
measurement-noise term:

$$
P^+ = A P^- A^T + Q_{event}
$$

where $A = \partial R / \partial y$ for the reset map. That should not be
conflated with IDAS state sensitivities.

## First Event Milestone

Before attempting correct event sensitivity jumps, make event use visible:

1. Add an event counter to `IntegratorIdaData`.
2. Increment it for any boundary/guard crossing that causes reanalysis or
   reinitialization.
3. When sensitivity mode is enabled and event count is nonzero, mark the
   sensitivity result:

   ```text
   invalid_due_to_events
   ```

4. Return a clear API status so Python can fall back to finite differences.

This gives a safe first version:

```text
smooth model:
    IDAS gradients allowed

event model:
    IDAS integration may still solve the state trajectory
    sensitivities are not exposed as valid gradients
```

## Later Event Support

To support event sensitivities correctly:

1. Represent each event transition as an explicit reset relation:

   ```text
   T(y_plus, ydot_plus, z_plus, y_minus, ydot_minus, z_minus, p, t) = 0
   ```

2. Differentiate the guard and transition equations.

3. Solve for:

   ```text
   dt_event/dp
   S_plus
   Sd_plus
   algebraic sensitivities
   ```

4. Reinitialize IDAS sensitivity vectors with `IDASensReInit`.

5. Record the event path. Sensitivities are local to the event sequence
   selected by the forward simulation and may be invalid if a perturbation
   changes event ordering.

6. Add validation models:

   - no-reset root crossing;
   - known clamp reset;
   - affine reset map;
   - event time depending on a parameter;
   - event order change detection.

Hybrid sensitivity literature emphasizes that direct and adjoint sensitivities
can be discontinuous at events and need jump conditions. Corner, Sandu, and
Sandu describe jump sensitivity matrices for hybrid multibody systems
[@doi:10.48550/arXiv.1802.07188]. Serban and Recuero formulate hybrid
ODE/DAE transition conditions, transition-time sensitivities, and consistent
post-transition initialization for sensitivity systems
[@doi:10.48550/arXiv.1904.08734]. A recent DAE optimization treatment
similarly frames event-split integration and reset maps as constraints whose
gradients are valid for a fixed event ordering and transversal guard crossings
[@doi:10.48550/arXiv.2605.05395].

# Validation Plan

## Backend Equivalence

When IDAS is available, run existing IDA tests with:

```text
INTEGRATOR IDA
```

linked through the IDAS backend. Results should match the old IDA backend to
existing tolerances.

## Sensitivity Smoke Tests

Add small models with known sensitivities:

1. Scalar exponential decay:

   ```text
   ydot = -k*y
   y(0) = y0
   y(t) = y0 * exp(-k*t)
   dy/dk = -t * y
   ```

2. First-order TGA-style model from `models/johnpye/iron/firstorder.a4c`.

3. Algebraic observation sensitivity for an algebraic variable that is part of
   the IDAS DAE vector.

4. Derived observation sensitivity for a postprocessed value:

   ```text
   observed = a * y
   ```

   This can be deferred until ASCEND can apply the observation chain rule.

Compare:

```text
IDAS sensitivity
central finite difference sensitivity
analytic sensitivity where available
```

## A4SQP Driver Validation

In `tga_fit_a4sqp.py`, validate:

```text
--gradient fd
--gradient idas
```

on a small single-trial model and one or two parameters. Require matching
gradient signs and relative magnitudes before trusting multi-parameter fits.

# Design Decisions And Remaining Questions

1. `INTEGRATOR IDAS` should mean sensitivity-capable integration with
   sensitivity data available for query after simulation. Sensitivity matrices
   are meaningful only when sensitivity parameter instances have been supplied.
   If no sensitivity parameters are configured, the engine can still run the
   state trajectory, but `hasSensitivities()` should report false or an empty
   parameter dimension.

   - Status: implemented as a distinct `INTEGRATOR IDAS` engine registered by
     `package_load('ida')` when the build links against SUNDIALS IDAS.
   - Status: fallback state-only behavior works; if no sensitivity parameters
     are configured, IDAS follows the ordinary IDA solve path and does not call
     `IDASensInit`.
   - Remaining: no `hasSensitivities()` API exists yet. Current callers can
     infer availability from `getNumSensitivityParameters()` and the returned
     observation-sensitivity matrix.

2. Sensitivity parameter selection should be API-driven first. These parameters
   are the $p_j$ values supplied to IDAS through `IDASetSensParams`. In an
   optimisation wrapper, the optimiser's free variables naturally define this
   list. ASCEND language metadata can be added later as a convenience, but the
   first prototype does not need the model source to declare the sensitivity
   parameters.

   - Status: implemented in the C integrator API as
     `integrator_set_sensitivity_parameters`,
     `integrator_get_num_sensitivity_parameters`, and
     `integrator_get_sensitivity_parameter`.
   - Status: exposed through C++/Python as `clearSensitivityParameters`,
     `addSensitivityParameter`, `getNumSensitivityParameters`, and
     `getSensitivityParameter`.
   - Remaining: there is no ASCEND-language declaration for sensitivity-driver
     parameters yet, and no parameter-scale API beyond the first heuristic for
     `pbar`.

3. Deterministic sensitivities and uncertainty propagation should stay
   separated. IDAS gives local derivatives such as $\partial y / \partial p_j$.
   Uncertainty/covariance propagation can later be built on top of those
   derivatives, but it should not be part of the first IDAS implementation.

   - Status: implemented prototype only stores deterministic local
     sensitivities. It does not attempt covariance, intervals, or uncertainty
     reset logic.
   - Remaining: uncertainty propagation can be layered above the deterministic
     sensitivity matrix once the parameter/output contracts are stable.

4. Observed algebraic variables should not be excluded just because they are
   algebraic. If an observed variable is part of the IDAS DAE unknown vector, it
   can have a direct IDAS sensitivity. The harder case is a derived observation
   that is not an IDAS unknown; that requires ASCEND to apply a chain rule for
   the observation expression.

   - Status: current implementation records sensitivities only for observed
     instances that map directly to `integ->y[]` state-vector entries.
   - Status: the `twinslabs_hc_sensitivity` regression covers direct observed
     state variables, `T_1` and `T_2`.
   - Remaining: direct algebraic entries in the IDAS DAE vector need a test
     case. Derived observations still need the ASCEND chain-rule layer and are
     currently stored as unavailable (`NaN`).

5. IDAS should use internal difference quotients for `dF/dp` in the first
   prototype. Exact ASCEND residual derivatives with respect to selected
   parameters should be treated as a later performance and accuracy upgrade.

   - Status: implemented using `IDASensInit(..., NULL, sens_y, sens_yp)`, so
     IDAS constructs sensitivity residuals internally using difference
     quotients.
   - Status: `IDASetSensParams` is called with actual ASCEND internal-unit
     parameter values in `sens_p` and first-pass scales in `sens_pbar`.
   - Remaining: exact $\partial F / \partial p_j$ support should be a later
     optimization using ASCEND relation derivatives with respect to selected
     parameter instances.

6. Define the exact mapping from an observed ASCEND instance to an IDAS vector
   index, including algebraic variables, and return a clear status for
   observations that do not have a direct mapping.

   - Status: implemented for direct state observations by matching the observed
     instance pointer against `var_instance(integ->y[i])` and copying
     `sens_y[j][i]` into the row-major observation-sensitivity matrix.
   - Status: observations without a direct state-vector mapping are filled with
     `NaN` in the current matrix.
   - Remaining: extend the mapping to algebraic DAE-vector entries where
     appropriate, and replace implicit `NaN` status with a more explicit
     per-observation availability/status API if users need to distinguish
     unsupported observations from numerical `NaN`.

7. Keep the IDAS `p` array synchronized with ASCEND parameter instances during
   IDAS internal difference-quotient residual calls. Without this,
   `IDASetSensParams` can hold the right numeric values but perturbing them
   will not necessarily change ASCEND residual evaluations.

   - Status: implemented via the parameter-value bridge. `IntegratorIdaData`
     stores `sens_p`, `sens_p_nominal`, `sens_pbar`, and `sens_plist`.
   - Status: the residual callback calls `integrator_ida_sens_sync` before
     `integrator_set_t`, `integrator_set_y`, `integrator_set_ydot`, and
     `relman_eval`, copying current `sens_p[i]` values into the selected ASCEND
     parameter instances.
   - Status: nominal parameter values are restored after `IDASolve` and after
     sensitivity recording so reporters and user code do not see temporary IDAS
     finite-difference perturbations.
   - Remaining: event/restart handling is still intentionally unsupported for
     sensitivities, so this synchronization has only been validated for smooth,
     non-event integrations.

# References

::: references
:::

- [SUNDIALS IDAS introduction][sundials-idas-intro].
- [SUNDIALS IDAS forward sensitivity analysis documentation][sundials-idas-fsa].
- [SUNDIALS IDAS mathematical considerations][sundials-idas-math].

[sundials-idas-intro]: https://sundials.readthedocs.io/en/latest/idas/Introduction_link.html
[sundials-idas-fsa]: https://sundials.readthedocs.io/en/latest/idas/Usage/FSA.html
[sundials-idas-math]: https://sundials.readthedocs.io/en/latest/idas/Mathematics_link.html
