/*
	Python Bindings for FPROPS 2.x with support
	for numerous different equations of state.
*/
%module fprops
%feature("autodoc");

%{
#include "../fprops.h"
#include "../fluids.h"
#include "../sat.h"
#include "../common.h"
#include "../solve_ph.h"
#include "../solve_Tx.h"
#include "../solve_px.h"
#include "../solve_pT.h"
#include "../refstate.h"
#include "../filedata.h"
#include "../derivs.h"
#include "../eqm_phase.h"

/*----------------- REFERENCE STATES -------------------*/

const ReferenceState REF_IIR = {FPROPS_REF_IIR};
const ReferenceState REF_NBP = {FPROPS_REF_NBP};
const ReferenceState REF_TPF = {FPROPS_REF_TPF};
const ReferenceState REF_TPFU = {FPROPS_REF_TPFU};


ReferenceState *REF_TRHS(double T0, double rho0, double h0, double s0){
	ReferenceState ref = {FPROPS_REF_TRHS,{.trhs={T0,rho0,h0,s0}}};
	ReferenceState *p = FPROPS_NEW(ReferenceState);
	*p = ref;
	return p;
}

ReferenceState *REF_TPUS(double T0, double p0, double u0, double s0){
	ReferenceState ref = {FPROPS_REF_TPUS,{.trhs={T0,p0,u0,s0}}};
	ReferenceState *p = FPROPS_NEW(ReferenceState);
	*p = ref;
	return p;
}

ReferenceState *REF_TPHS(double T0, double p0, double h0, double s0){
	ReferenceState ref = {FPROPS_REF_TPHS,{.trhs={T0,p0,h0,s0}}};
	ReferenceState *p = FPROPS_NEW(ReferenceState);
	*p = ref;
	return p;
}

ReferenceState *REF_PHI0(double c, double m){
	ReferenceState ref = {FPROPS_REF_PHI0,{.phi0={.c=c,.m=m}}};
	ReferenceState *p = FPROPS_NEW(ReferenceState);
	*p = ref;
	return p;
}

static const char *fprops_py_string(PyObject *obj, const char *what){
	if(PyUnicode_Check(obj)){
		return PyUnicode_AsUTF8(obj);
	}
	if(PyBytes_Check(obj)){
		return PyBytes_AsString(obj);
	}
	PyErr_Format(PyExc_TypeError, "%s must be a string", what);
	return NULL;
}

static PyObject *fprops_py_string_list(const char *(*name_fn)(const FpropsEqm *, const char *, int),
		const FpropsEqm *eqm, const char *phase, int n){
	PyObject *list;
	if(n < 0){
		PyErr_Format(PyExc_ValueError, "unknown phase '%s'", phase ? phase : "(null)");
		return NULL;
	}
	list = PyList_New(n);
	if(!list){
		return NULL;
	}
	for(int i = 0; i < n; ++i){
		const char *name = name_fn(eqm, phase, i);
		PyObject *pyname = PyUnicode_FromString(name ? name : "");
		if(!pyname){
			Py_DECREF(list);
			return NULL;
		}
		PyList_SET_ITEM(list, i, pyname);
	}
	return list;
}

static int fprops_py_set_status_error(const char *op, int status){
	PyErr_Format(PyExc_RuntimeError, "%s failed with status %d (%s)",
		op, status, fprops_eqm_status_text(status));
	return 0;
}

static int fprops_py_eqm_add_phases(FpropsEqm *eqm, PyObject *specs){
	PyObject *seq;
	Py_ssize_t n;
	if(PyUnicode_Check(specs) || PyBytes_Check(specs)){
		PyErr_SetString(PyExc_TypeError, "phase list must be a sequence of strings");
		return 0;
	}
	seq = PySequence_Fast(specs, "phase list must be a sequence of strings");
	if(!seq){
		return 0;
	}
	n = PySequence_Fast_GET_SIZE(seq);
	for(Py_ssize_t i = 0; i < n; ++i){
		PyObject *item = PySequence_Fast_GET_ITEM(seq, i);
		const char *spec = fprops_py_string(item, "phase specification");
		int status;
		if(!spec){
			Py_DECREF(seq);
			return 0;
		}
		status = fprops_eqm_add_phase(eqm, spec, NULL);
		if(status < 0){
			Py_DECREF(seq);
			return fprops_py_set_status_error("add_phase", status);
		}
	}
	Py_DECREF(seq);
	return 1;
}

static int fprops_py_eqm_add_pair(FpropsEqm *eqm, PyObject *name_obj, PyObject *amount_obj){
	const char *name = fprops_py_string(name_obj, "component name");
	double amount;
	int status;
	if(!name){
		return 0;
	}
	amount = PyFloat_AsDouble(amount_obj);
	if(PyErr_Occurred()){
		return 0;
	}
	status = fprops_eqm_add_formula(eqm, name, amount);
	if(status != 0){
		return fprops_py_set_status_error("add_comp", status);
	}
	return 1;
}

static int fprops_py_eqm_add_comps(FpropsEqm *eqm, PyObject *items){
	if(PyDict_Check(items)){
		PyObject *key, *value;
		Py_ssize_t pos = 0;
		while(PyDict_Next(items, &pos, &key, &value)){
			if(!fprops_py_eqm_add_pair(eqm, key, value)){
				return 0;
			}
		}
		return 1;
	}

	PyObject *seq = PySequence_Fast(items,
		"components must be a dict or a sequence of (name, amount) pairs");
	if(!seq){
		return 0;
	}
	Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
	for(Py_ssize_t i = 0; i < n; ++i){
		PyObject *pair = PySequence_Fast(PySequence_Fast_GET_ITEM(seq, i),
			"component item must be a (name, amount) pair");
		if(!pair){
			Py_DECREF(seq);
			return 0;
		}
		if(PySequence_Fast_GET_SIZE(pair) != 2){
			Py_DECREF(pair);
			Py_DECREF(seq);
			PyErr_SetString(PyExc_ValueError, "component item must contain exactly two values");
			return 0;
		}
		if(!fprops_py_eqm_add_pair(eqm, PySequence_Fast_GET_ITEM(pair, 0),
				PySequence_Fast_GET_ITEM(pair, 1))){
			Py_DECREF(pair);
			Py_DECREF(seq);
			return 0;
		}
		Py_DECREF(pair);
	}
	Py_DECREF(seq);
	return 1;
}

static int fprops_py_eqm_add_phase_feed(FpropsEqm *eqm, const char *phase,
		double amount, PyObject *coords){
	double values[FPROPS_EQM_PHASE_MAX_VARS];
	PyObject *seq = PySequence_Fast(coords,
		"phase-feed coordinates must be a sequence of numbers");
	if(!seq){
		return 0;
	}
	Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
	if(n > FPROPS_EQM_PHASE_MAX_VARS){
		Py_DECREF(seq);
		PyErr_SetString(PyExc_ValueError, "too many phase-feed coordinates");
		return 0;
	}
	for(Py_ssize_t i = 0; i < n; ++i){
		values[i] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(seq, i));
		if(PyErr_Occurred()){
			Py_DECREF(seq);
			return 0;
		}
	}
	Py_DECREF(seq);
	int status = fprops_eqm_add_phase_feed_values(eqm, phase, amount, values, (int)n);
	if(status != 0){
		return fprops_py_set_status_error("add_phase_feed", status);
	}
	return 1;
}

static int fprops_py_coord_item(FpropsEqmCoord *coord, PyObject *name_obj, PyObject *value_obj){
	const char *name = fprops_py_string(name_obj, "coordinate name");
	double value;
	if(!name){
		return 0;
	}
	value = PyFloat_AsDouble(value_obj);
	if(PyErr_Occurred()){
		return 0;
	}
	coord->name = name;
	coord->value = value;
	return 1;
}

static int fprops_py_eqm_add_phase_feed_vars(FpropsEqm *eqm, const char *phase,
		double amount, PyObject *items){
	FpropsEqmCoord coords[FPROPS_EQM_PHASE_MAX_VARS];
	int n = 0;
	if(PyDict_Check(items)){
		PyObject *key, *value;
		Py_ssize_t pos = 0;
		while(PyDict_Next(items, &pos, &key, &value)){
			if(n >= FPROPS_EQM_PHASE_MAX_VARS){
				PyErr_SetString(PyExc_ValueError, "too many phase-feed coordinates");
				return 0;
			}
			if(!fprops_py_coord_item(&coords[n++], key, value)){
				return 0;
			}
		}
	}else{
		PyObject *seq = PySequence_Fast(items,
			"phase-feed coordinates must be a dict or a sequence of (name, value) pairs");
		if(!seq){
			return 0;
		}
		Py_ssize_t nseq = PySequence_Fast_GET_SIZE(seq);
		if(nseq > FPROPS_EQM_PHASE_MAX_VARS){
			Py_DECREF(seq);
			PyErr_SetString(PyExc_ValueError, "too many phase-feed coordinates");
			return 0;
		}
		for(Py_ssize_t i = 0; i < nseq; ++i){
			PyObject *pair = PySequence_Fast(PySequence_Fast_GET_ITEM(seq, i),
				"coordinate item must be a (name, value) pair");
			if(!pair){
				Py_DECREF(seq);
				return 0;
			}
			if(PySequence_Fast_GET_SIZE(pair) != 2){
				Py_DECREF(pair);
				Py_DECREF(seq);
				PyErr_SetString(PyExc_ValueError, "coordinate item must contain exactly two values");
				return 0;
			}
			if(!fprops_py_coord_item(&coords[n++], PySequence_Fast_GET_ITEM(pair, 0),
					PySequence_Fast_GET_ITEM(pair, 1))){
				Py_DECREF(pair);
				Py_DECREF(seq);
				return 0;
			}
			Py_DECREF(pair);
		}
		Py_DECREF(seq);
	}

	int status = fprops_eqm_add_phase_feed_var_items(eqm, phase, amount, n, coords);
	if(status != 0){
		return fprops_py_set_status_error("add_phase_feed_vars", status);
	}
	return 1;
}

static int fprops_py_double_seq(PyObject *items, double *values, int maxn, const char *what){
	PyObject *seq = PySequence_Fast(items, what);
	if(!seq){
		return -1;
	}
	Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
	if(n > maxn){
		Py_DECREF(seq);
		PyErr_Format(PyExc_ValueError, "%s has too many values", what);
		return -1;
	}
	for(Py_ssize_t i = 0; i < n; ++i){
		values[i] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(seq, i));
		if(PyErr_Occurred()){
			Py_DECREF(seq);
			return -1;
		}
	}
	Py_DECREF(seq);
	return (int)n;
}

static PyObject *fprops_py_double_list(const double *values, int n){
	PyObject *list = PyList_New(n);
	if(!list){
		return NULL;
	}
	for(int i = 0; i < n; ++i){
		PyObject *value = PyFloat_FromDouble(values[i]);
		if(!value){
			Py_DECREF(list);
			return NULL;
		}
		PyList_SET_ITEM(list, i, value);
	}
	return list;
}

%}

// TODO can we make these into a python submodule instead?
const ReferenceState REF_IIR;
const ReferenceState REF_NBP;
const ReferenceState REF_TPF;
const ReferenceState REF_TPFU;
const ReferenceState *REF_TRHS(double T0, double rho0, double h0, double s0);
const ReferenceState *REF_TPUS(double T0, double p0, double u0, double s0);
const ReferenceState *REF_TPHS(double T0, double p0, double h0, double s0);
const ReferenceState *REF_PHI0(double c, double m);

%typemap(in) PyObject * {
	$1 = $input;
}
%typemap(out) PyObject * {
	$result = $1;
}

%rename(eqm_status_text) fprops_eqm_status_text;
const char *fprops_eqm_status_text(int status);

%rename(eqm_status_ok) fprops_eqm_status_ok;
int fprops_eqm_status_ok(int status);

%rename(eqm_nlp_solver_name) fprops_eqm_nlp_solver_name;
const char *fprops_eqm_nlp_solver_name(int solver);

%constant double FPROPS_R = 8.31446261815324;

#if 0 // no way to interrogate reference states from python so far
typedef struct ReferenceState_struct ReferenceState;
%extend ReferenceState{
	const char *typename;
}

%{
const char *ReferenceState_typename_get(const ReferenceState *ref){
	return fprops_refstate_type(ref->type);
}
%}
#endif

/* ----------------- ACCESSING FLUID SPECIFICATIONS -----------------*/

// load and initialise a PureFluid
%rename(fluid) fprops_fluid;
%exception {
	$action
	if(!result){
		PyErr_SetString(PyExc_RuntimeError,"Invalid fluid requested");
		return NULL;
	}
}
PureFluid *fprops_fluid(char *name, const char *corrtype = NULL, const char *source = NULL);

// get a fluid by index position (don't assume that these numbers are constant!)
%rename(get_fluid) fprops_get_fluid;
PureFluid *fprops_get_fluid(int i);

%exception;

// how many fluids are available
%rename(num_fluids) fprops_num_fluids;
int fprops_num_fluids();

/*------------------- PURE FLUID OBJECT -------------------*/

%nodefaultctor;

typedef struct{} FluidState2;
typedef struct{} PureFluid;

/* FIXME what should we do with ctors and dtors...? */
//%nodefaultdtor PureFluid;
%nodefaultctor PureFluid;

// use SWIG's generalised exceptions
%include <exception.i>

/*------------------- PHASE EQUILIBRIUM OBJECTS -------------------*/

%rename(Eqm) FpropsEqm;
typedef struct{} FpropsEqm;

%rename(EqmPhaseResult) FpropsEqmPhaseResult;
typedef struct{} FpropsEqmPhaseResult;

%rename(EqmPhase) FpropsEqmPhaseModel;
typedef struct{} FpropsEqmPhaseModel;

%extend FpropsEqmPhaseModel{
	FpropsEqmPhaseModel(const char *spec, const char *source = NULL){
		FpropsEqmPhaseModel *phase = FPROPS_NEW(FpropsEqmPhaseModel);
		if(!phase){
			PyErr_NoMemory();
			return NULL;
		}
		if(!fprops_eqm_phase_resolve(spec, source, phase)){
			FPROPS_FREE(phase);
			PyErr_Format(PyExc_ValueError, "unable to resolve phase '%s'", spec ? spec : "(null)");
			return NULL;
		}
		return phase;
	}

	~FpropsEqmPhaseModel(){
		FPROPS_FREE($self);
	}

	const char *name(){
		return $self->name;
	}

	const char *source(){
		return $self->source;
	}

	const char *basis(){
		return $self->basis;
	}

	const char *kind(){
		return fprops_eqm_phase_kind_name($self->kind);
	}

	int coord_count(){
		return $self->nvar;
	}

	int element_count(){
		return $self->nelem;
	}

	int member_count(){
		return $self->nmember;
	}

	PyObject *coord_names(){
		PyObject *list = PyList_New($self->nvar);
		if(!list){
			return NULL;
		}
		for(int i = 0; i < $self->nvar; ++i){
			PyObject *name = PyUnicode_FromString($self->var_names[i] ? $self->var_names[i] : "");
			if(!name){
				Py_DECREF(list);
				return NULL;
			}
			PyList_SET_ITEM(list, i, name);
		}
		return list;
	}

	PyObject *element_names(){
		PyObject *list = PyList_New($self->nelem);
		if(!list){
			return NULL;
		}
		for(int i = 0; i < $self->nelem; ++i){
			PyObject *name = PyUnicode_FromString($self->elements[i] ? $self->elements[i] : "");
			if(!name){
				Py_DECREF(list);
				return NULL;
			}
			PyList_SET_ITEM(list, i, name);
		}
		return list;
	}

	PyObject *member_names(){
		PyObject *list = PyList_New($self->nmember);
		if(!list){
			return NULL;
		}
		for(int i = 0; i < $self->nmember; ++i){
			PyObject *name = PyUnicode_FromString($self->members[i] ? $self->members[i] : "");
			if(!name){
				Py_DECREF(list);
				return NULL;
			}
			PyList_SET_ITEM(list, i, name);
		}
		return list;
	}

	PyObject *lower_bounds(){
		return fprops_py_double_list($self->lower, $self->nvar);
	}

	PyObject *upper_bounds(){
		return fprops_py_double_list($self->upper, $self->nvar);
	}

	double gibbs(double T, double P, PyObject *coords = Py_None){
		double y[FPROPS_EQM_PHASE_MAX_VARS];
		double g = NAN;
		int n = 0;
		if(coords != Py_None){
			n = fprops_py_double_seq(coords, y, FPROPS_EQM_PHASE_MAX_VARS,
				"phase coordinates must be a sequence of numbers");
			if(n < 0){
				return NAN;
			}
		}
		if(n != $self->nvar){
			PyErr_Format(PyExc_ValueError, "phase '%s' expects %d coordinates, got %d",
				$self->name ? $self->name : "(unnamed)", $self->nvar, n);
			return NAN;
		}
		int status = fprops_eqm_phase_gibbs($self, T, P, $self->nvar ? y : NULL, &g);
		if(status < 0){
			fprops_py_set_status_error("phase_gibbs", status);
			return NAN;
		}
		return g;
	}

	PyObject *elements(PyObject *coords = Py_None){
		double y[FPROPS_EQM_PHASE_MAX_VARS];
		double a[FPROPS_EQM_PHASE_MAX_ELEMS];
		int n = 0;
		if(coords != Py_None){
			n = fprops_py_double_seq(coords, y, FPROPS_EQM_PHASE_MAX_VARS,
				"phase coordinates must be a sequence of numbers");
			if(n < 0){
				return NULL;
			}
		}
		if(n != $self->nvar){
			PyErr_Format(PyExc_ValueError, "phase '%s' expects %d coordinates, got %d",
				$self->name ? $self->name : "(unnamed)", $self->nvar, n);
			return NULL;
		}
		int status = fprops_eqm_phase_elements($self, $self->nvar ? y : NULL, a);
		if(status < 0){
			fprops_py_set_status_error("phase_elements", status);
			return NULL;
		}
		return fprops_py_double_list(a, $self->nelem);
	}

	PyObject *entry_residual(double T, double P, PyObject *lambda){
		double lam[FPROPS_EQM_PHASE_MAX_ELEMS];
		double y[FPROPS_EQM_PHASE_MAX_VARS];
		double phi = NAN;
		int n = fprops_py_double_seq(lambda, lam, FPROPS_EQM_PHASE_MAX_ELEMS,
			"element potentials must be a sequence of numbers");
		if(n < 0){
			return NULL;
		}
		if(n != $self->nelem){
			PyErr_Format(PyExc_ValueError, "phase '%s' expects %d element potentials, got %d",
				$self->name ? $self->name : "(unnamed)", $self->nelem, n);
			return NULL;
		}
		int status = fprops_eqm_phase_entry_residual($self, T, P, lam, &phi, y);
		if(!fprops_eqm_status_ok(status)){
			fprops_py_set_status_error("phase_entry_residual", status);
			return NULL;
		}
		PyObject *coords = fprops_py_double_list(y, $self->nvar);
		if(!coords){
			return NULL;
		}
		PyObject *out = Py_BuildValue("(dO)", phi, coords);
		Py_DECREF(coords);
		return out;
	}
}

%extend FpropsEqm{
	FpropsEqm(){
		FpropsEqm *eqm = FPROPS_NEW(FpropsEqm);
		if(!eqm){
			PyErr_NoMemory();
			return NULL;
		}
		fprops_eqm_init(eqm);
		return eqm;
	}

	~FpropsEqm(){
		FPROPS_FREE($self);
	}

	int add_phase(const char *spec, const char *source = NULL){
		return fprops_eqm_add_phase($self, spec, source);
	}

	PyObject *add_phases(PyObject *specs){
		if(!fprops_py_eqm_add_phases($self, specs)){
			return NULL;
		}
		Py_RETURN_NONE;
	}

	int phase_count(){
		return fprops_eqm_phase_count($self);
	}

	const char *phase_name(int iphase){
		const char *name = fprops_eqm_phase_name($self, iphase);
		if(!name){
			PyErr_SetString(PyExc_IndexError, "phase index out of range");
			return NULL;
		}
		return name;
	}

	PyObject *phase_names(){
		int n = fprops_eqm_phase_count($self);
		PyObject *list = PyList_New(n);
		if(!list){
			return NULL;
		}
		for(int i = 0; i < n; ++i){
			const char *name = fprops_eqm_phase_name($self, i);
			PyObject *pyname = PyUnicode_FromString(name ? name : "");
			if(!pyname){
				Py_DECREF(list);
				return NULL;
			}
			PyList_SET_ITEM(list, i, pyname);
		}
		return list;
	}

	int find_phase(const char *phase){
		return fprops_eqm_find_phase($self, phase);
	}

	PyObject *phase_coord_names(const char *phase){
		int n = fprops_eqm_phase_coord_count($self, phase);
		return fprops_py_string_list(fprops_eqm_phase_coord_name, $self, phase, n);
	}

	PyObject *phase_member_names(const char *phase){
		int n = fprops_eqm_phase_member_count($self, phase);
		return fprops_py_string_list(fprops_eqm_phase_member_name, $self, phase, n);
	}

	int set_TP(double T, double P){
		return fprops_eqm_set_TP($self, T, P);
	}

	int set_algorithm(const char *algorithm){
		return fprops_eqm_set_algorithm($self, algorithm);
	}

	int set_nlp_solver(const char *solver){
		return fprops_eqm_set_nlp_solver_name($self, solver);
	}

	const char *nlp_solver(){
		return fprops_eqm_nlp_solver_name(fprops_eqm_nlp_solver($self));
	}

	void clear_feed(){
		fprops_eqm_clear_feed($self);
	}

	int add_element(const char *element, double amount){
		return fprops_eqm_add_element($self, element, amount);
	}

	int set_element(const char *element, double amount){
		return fprops_eqm_set_element($self, element, amount);
	}

	double element_amount(const char *element){
		return fprops_eqm_element_amount($self, element);
	}

	int add_formula(const char *formula, double amount){
		return fprops_eqm_add_formula($self, formula, amount);
	}

	PyObject *add_comps(PyObject *items){
		if(!fprops_py_eqm_add_comps($self, items)){
			return NULL;
		}
		Py_RETURN_NONE;
	}

	PyObject *add_phase_feed(const char *phase, double amount, PyObject *coords){
		if(!fprops_py_eqm_add_phase_feed($self, phase, amount, coords)){
			return NULL;
		}
		Py_RETURN_NONE;
	}

	PyObject *add_phase_feed_vars(const char *phase, double amount, PyObject *coords){
		if(!fprops_py_eqm_add_phase_feed_vars($self, phase, amount, coords)){
			return NULL;
		}
		Py_RETURN_NONE;
	}

	FpropsEqmPhaseResult *try_solve(){
		FpropsEqmPhaseResult *result = FPROPS_NEW_CLEAR(FpropsEqmPhaseResult);
		if(!result){
			PyErr_NoMemory();
			return NULL;
		}
		fprops_eqm_solve($self, result);
		return result;
	}

	FpropsEqmPhaseResult *solve(){
		FpropsEqmPhaseResult *result = FPROPS_NEW_CLEAR(FpropsEqmPhaseResult);
		if(!result){
			PyErr_NoMemory();
			return NULL;
		}
		fprops_eqm_solve($self, result);
		if(result->status != 0){
			fprops_py_set_status_error("solve", result->solver_status);
			FPROPS_FREE(result);
			return NULL;
		}
		return result;
	}
}

%extend FpropsEqmPhaseResult{
	~FpropsEqmPhaseResult(){
		FPROPS_FREE($self);
	}

	int status(){
		return $self->status;
	}

	int solver_status(){
		return $self->solver_status;
	}

	const char *status_text(){
		return fprops_eqm_status_text($self->solver_status);
	}

	int phase_count(){
		return $self->nphase;
	}

	int member_count(){
		return $self->nmember;
	}

	PyObject *phase_names(){
		const FpropsEqm *eqm = $self->eqm;
		int n = eqm ? fprops_eqm_phase_count(eqm) : 0;
		PyObject *list = PyList_New(n);
		if(!list){
			return NULL;
		}
		for(int i = 0; i < n; ++i){
			const char *name = fprops_eqm_phase_name(eqm, i);
			PyObject *pyname = PyUnicode_FromString(name ? name : "");
			if(!pyname){
				Py_DECREF(list);
				return NULL;
			}
			PyList_SET_ITEM(list, i, pyname);
		}
		return list;
	}

	PyObject *phase_coord_names(const char *phase){
		int n = fprops_eqm_phase_coord_count($self->eqm, phase);
		return fprops_py_string_list(fprops_eqm_phase_coord_name, $self->eqm, phase, n);
	}

	PyObject *phase_member_names(const char *phase){
		int n = fprops_eqm_phase_member_count($self->eqm, phase);
		return fprops_py_string_list(fprops_eqm_phase_member_name, $self->eqm, phase, n);
	}

	double phase_amount(const char *phase){
		return fprops_eqm_phase_amount($self, phase);
	}

	int phase_active(const char *phase){
		int p = fprops_eqm_find_phase($self->eqm, phase);
		if(p < 0 || p >= $self->nphase){
			return -1;
		}
		return $self->phase_active[p];
	}

	double phase_coord(const char *phase, const char *coord){
		return fprops_eqm_phase_coord($self, phase, coord);
	}

	PyObject *phase_coords(const char *phase){
		double values[FPROPS_EQM_PHASE_MAX_VARS];
		int n = fprops_eqm_phase_coord_values($self, phase, values);
		if(n < 0){
			PyErr_Format(PyExc_ValueError, "unknown phase '%s'", phase ? phase : "(null)");
			return NULL;
		}
		PyObject *list = PyList_New(n);
		if(!list){
			return NULL;
		}
		for(int i = 0; i < n; ++i){
			PyObject *value = PyFloat_FromDouble(values[i]);
			if(!value){
				Py_DECREF(list);
				return NULL;
			}
			PyList_SET_ITEM(list, i, value);
		}
		return list;
	}

	double phase_member_amount(const char *phase, const char *member){
		return fprops_eqm_phase_member_amount($self, phase, member);
	}

	double phase_member_fraction(const char *phase, const char *member){
		return fprops_eqm_phase_member_fraction($self, phase, member);
	}

	PyObject *phase_member_amounts(const char *phase){
		double values[FPROPS_EQM_PHASE_MAX_MEMBERS];
		int n = fprops_eqm_phase_member_amounts($self, phase, values);
		if(n < 0){
			PyErr_Format(PyExc_ValueError, "unknown phase '%s'", phase ? phase : "(null)");
			return NULL;
		}
		PyObject *list = PyList_New(n);
		if(!list){
			return NULL;
		}
		for(int i = 0; i < n; ++i){
			PyObject *value = PyFloat_FromDouble(values[i]);
			if(!value){
				Py_DECREF(list);
				return NULL;
			}
			PyList_SET_ITEM(list, i, value);
		}
		return list;
	}
}

%pythoncode %{
_Eqm_solve_native = Eqm.solve
_Eqm_try_solve_native = Eqm.try_solve

def _Eqm_status_error(op, status):
	raise RuntimeError("%s failed with status %d (%s)" %
		(op, status, eqm_status_text(status)))

def _Eqm_solve_keepalive(self):
	result = _Eqm_solve_native(self)
	result._eqm_owner = self
	return result

def _Eqm_try_solve_keepalive(self):
	result = _Eqm_try_solve_native(self)
	result._eqm_owner = self
	return result

def _Eqm_solve_TP(self, T, P):
	status = self.set_TP(T, P)
	if status != 0:
		_Eqm_status_error("set_TP", status)
	return self.solve()

Eqm.solve = _Eqm_solve_keepalive
Eqm.try_solve = _Eqm_try_solve_keepalive
Eqm.solve_TP = _Eqm_solve_TP
%}

%extend PureFluid{
	// destructor: doesn't see to work
	~PureFluid();

	// use a local _fprops___err variable to catch and raise errors from FPROPS
	%typemap(in,numinputs=0) FpropsError *err (FpropsError _fprops___err = 0) {
		$1 = &_fprops___err;
	}
	%typemap(argout) FpropsError *err {
		if(*$1 != 0) {
		    SWIG_exception(SWIG_ValueError,fprops_error(*$1));
		}
	}

	// set the reference state for a fluid
	void set_ref(ReferenceState *ref, FpropsError *err){
		int res;
		res = fprops_set_reference_state($self, ref);
		if(res)*err = FPROPS_NUMERIC_ERROR;
	}

	FluidState2 set_Trho(double T, double rho, FpropsError *err){
		FluidState2 state = fprops_set_Trho(T,rho,$self,err);
		return state;
	}

	FluidState2 set_ph(double p, double h, FpropsError *err){
		FluidState2 state = fprops_solve_ph(p, h, $self, err);
		return state;
	}
	FluidState2 set_pT(double p, double T, FpropsError *err){
		FluidState2 state = fprops_solve_pT(p, T, $self, err);
		return state;
	}

	int region_ph(double p, double h, FpropsError *err){
		return fprops_region_ph(p, h, $self,err);
	}

	FluidState2 set_Tx(double T, double x, FpropsError *err){
		FluidState2 state = fprops_solve_Tx(T, x, $self, err);
		//state.T = T;
		//state.fluid = $self;
		return state;
	}

	int region_Tx(double T, double x, FpropsError *err){
		return fprops_region_Tx(T, x, $self,err);
	}

	FluidState2 set_px(double p, double x, FpropsError *err){
		FluidState2 state = fprops_solve_px(p, x, $self, err);
		return state;
	}

	int region_px(double p, double x, FpropsError *err){
		return fprops_region_px(p, x, $self,err);
	}


	double psat_T_acentric(double T){
		return fprops_psat_T_acentric(T, $self->data);
	}
	
	double psat_T_xiang(double T){
		return fprops_psat_T_xiang(T, $self->data);
	}

	double rhof_T_rackett(double T){
		return fprops_rhof_T_rackett(T, $self->data);
	}

	double rhog_T_chouaieb(double T){
		return fprops_rhog_T_chouaieb(T, $self->data);
	}

	%apply double *OUTPUT { double *rho_f };
	%apply double *OUTPUT { double *rho_g };
	double triple_point(double *rho_f, double *rho_g, FpropsError *err){
		double p;
		fprops_triple_point(&p,rho_f,rho_g,$self, err);
		return p;
	}

	int can_sat(){
		// FIXME this should be implemented elsewhere...
		switch($self->type){
		case FPROPS_PENGROB:
		case FPROPS_HELMHOLTZ:
			return 1;
		case FPROPS_IDEAL:
		case FPROPS_INCOMP:
		default:
			return 0;
		}
	}

	double sat_T(double T, double *rho_f, double *rho_g, FpropsError *err){
		double p;
		fprops_sat_T(T, &p, rho_f, rho_g, $self, err);
		return p;
	}

	double sat_p(double p, double *rho_f, double *rho_g, FpropsError *err){
		double T;
		fprops_sat_p(p, &T, rho_f, rho_g, $self, err);
		return T;
	}

	// raise exception if user attempts to write to these variables
	//%typemap(in) double{
	//	SWIG_exception(SWIG_ValueError,"Read-only attribute");
	//	return NULL;
	//}
	%immutable;
	double T_t;
	double T_c;
	double rho_c;
	double p_c;
	double omega;
	double M;
	double R;
	%immutable;
	char *name;
	int type;
	const char *typename;
	char *source;
}

%{
void delete_PureFluid(PureFluid *P){
	fprintf(stderr,"DESTROY\n");
	fprops_fluid_destroy(P);
}

// TODO trim this stuff down using some macro magic

double PureFluid_T_t_get(const PureFluid *fluid){
	return fluid->data->T_t;
}
double PureFluid_T_c_get(PureFluid *fluid){
	return fluid->data->T_c;
}
double PureFluid_p_c_get(PureFluid *fluid){
	return fluid->data->p_c;
}
double PureFluid_rho_c_get(PureFluid *fluid){
	return fluid->data->rho_c;
}
double PureFluid_omega_get(PureFluid *fluid){
	return fluid->data->omega;
}
double PureFluid_M_get(PureFluid *fluid){
	return fluid->data->M;
}
double PureFluid_R_get(PureFluid *fluid){
	return fluid->data->R;
}
const char *PureFluid_name_get(const PureFluid *fluid){
	return fluid->name;
}
const char *PureFluid_source_get(const PureFluid *fluid){
	return fluid->source;
}
int PureFluid_type_get(PureFluid *fluid){
	return fluid->type;
}

// FIXME get enumerations working more 'natively' in Python to avoid needing these string functions.
const char *PureFluid_typename_get(PureFluid *fluid){
	return fprops_corr_type(fluid->type);
}
%}

/*----------------------- FLUID STATE OBJECT ------------------*/

%{
static __thread FpropsError _fprops_fluidstate_err = 0;
%}

/* TODO be more sophisticated about the types of error returned to Python 
eg ValueError for FPROPS_VALUE_UNDEFINED, perhaps, or some FPROPS-specific
exception types? */
%exception{
	_fprops_fluidstate_err = 0;
	$action
	if(_fprops_fluidstate_err){
	    SWIG_exception(SWIG_RuntimeError,fprops_error(_fprops_fluidstate_err));
		return NULL;
	}
}

%extend FluidState2{
	// use a local _fprops___err variable to catch and raise errors from FPROPS
	%typemap(in,numinputs=0) FpropsError *err (FpropsError _fprops___err = 0) {
		$1 = &_fprops___err;
	}
	%typemap(argout) FpropsError *err {
		if(*$1 != 0) {
		    SWIG_exception(SWIG_ValueError,fprops_error(*$1));
		}
	}
	
	double deriv(char *spec, FpropsError *err){
		return fprops_deriv(*$self, spec, err);
	}
	%immutable;
	double T, rho, v;
	double x, p, u, h, s, a, cv, cp, w, g, alphap, betap, cp0, dpdT_rho;
	double mu, lam;
}

%{
#define FNS(G,X) G(T) X G(rho) X G(v) X G(x) X G(p) X G(u) X G(h) X G(s) X G(a) X G(cv) \
	X G(cp) X G(w) X G(g) X G(alphap) X G(betap) X G(cp0) X G(dpdT_rho) \
	X G(mu) X G(lam)
#define GETTER(N) \
	double FluidState2_##N##_get(FluidState2 *state){\
		return fprops_##N(*state,&_fprops_fluidstate_err);\
	}
#define SPACE
FNS(GETTER,SPACE)
#undef GETTER
#undef SPACE

%}
