FPROPS
======

Code in this directory is a standalone pure fluid property evaluation library
primarily based on the Helmholtz free energy formulation used by various
authors for high-accuracy thermodynamic property correlations.

This is currently experimental software, with many bugs. You should carefully
test the results from this library before you rely on any of its values.

Integration is provided to allow these routines to be called from ASCEND, but
the support is still partial, and still lacks the derivative expressions that
allow the most efficient possible convergence during iteration.
                               ________________________________________________
                              |                                                |
                              | More details are available on the ASCEND Wiki: |
                              | http://ascendwiki.cheme.cmu.edu/FPROPS         |
                              |________________________________________________|

Ideal gas properties
--------------------

Helmholtz property correlations are given in terms of the 'ideal' part and the
'residual' part. The ideal part is based on the zero-pressure limit, where
the specific heat capacity of a fluid approaches that of an ideal gas, and
consequently is a function of temperature only, cp0(T).

We provide a means for specifying cp0(T) as a sum of power terms and
'exponential terms'; see the Span paper (nitrogen.c) for some discussion of 
this.

The ideal part of the Helmholtz free energy is calculated by integrating
the cp0(T) function as described in Tillner-Roth (see ammonia.c).

In some cases, authors give their results for the Helmholtz free energy
without providing their original correlation for cp0(T). In this case
we back-calculate what the temps in the cp0(T) correlation must be, and check
that the form of the resulting re-integrated phi0 expression is correct.

Supported fluids
----------------

Each fluid that is supported by this library must currently have .c and .h
file in which the correlation parameters are provided, and optionally including
a test suite to validate the results.

We propose to add support for a text-based input file format, and possibly a
database for simple fluid correlations of the type currently provided by ASCEND,
Sim42, and other process simulators. But that is a job for later.

-- 
John Pye
09 Oct 2008.

Let me add to what John Pye wrote - this code is experimental, we have
added methane, CO and ethanol (and we know ethanol does not give the
right answers) - we are checking the code and the coefficients, use them
at your risk, if at all.

HongKe Zhu, a graduate student at the Univ of ALabama Huntsville has
been working on fprops/adding components, making some changes.
We have not checked the results, yet.

Krishnan Chittur
February 1, 2010

