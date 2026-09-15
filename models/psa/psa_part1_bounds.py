"""Rounding-aware zero-PE adsorption check, not a full-cycle feasibility test.

Part I eqs (7), (10)–(13); methane captured in feed repressurisation gives
sA0 = y*FR when phiDes=1 and there is no PE. No K, source-pressure, mixing
or pressure-change heat model is used. Units: m, Pa, mol, K, J/mol.

The LRC transcription is independently checked against the ASCEND model.
Interval arithmetic deliberately loses correlations to give an outer
enclosure, rather than mistaking sampled corners for proven extrema.
This is a double-precision numerical check, not formal interval certification.
"""
import math

R, PSI = 8.31446261815324, 6894.757293168
Y, PHI, ETA, EPS = .05, .75, .95, .44
RHO, CV, CPA, CPB, TFEED = 800., 804000., 36.8, 29.3, 298.


def _add(a, b):
    return math.nextafter(a[0]+b[0], -math.inf), math.nextafter(a[1]+b[1], math.inf)


def _sub(a, b):
    return _add(a, (-b[1], -b[0]))


def _mul(a, b):
    values = [x*y for x in a for y in b]
    return math.nextafter(min(values), -math.inf), math.nextafter(max(values), math.inf)


def _div(a, b):
    if b[0] <= 0 <= b[1]:
        raise ValueError('Zero-containing denominator in source-data enclosure')
    return _mul(a, (math.nextafter(1/b[1], -math.inf), math.nextafter(1/b[0], math.inf)))


def loading(T, P):
    """Cen–Yang methane q in mol/kg; P is total pressure, y=0.05."""
    activity = math.exp(-10.245+1756/T)*Y*P/PSI
    return (-.76+40539/T)*1e-3/(R*273.15/101325)*activity/(1+activity)


def _temperature(V, P, total_feed):
    # On 220..650 K, q and 1/T decrease strictly with T. At fixed T both
    # terms increase with V and P. These monotonicities enclose ALL inputs
    # in the rounding box, not just a grid of temperatures/designs.
    def balance(T):
        return PHI*ETA*V*(1-EPS)*RHO*loading(T, P)+Y*P*PHI*V*EPS/(R*T)-Y*total_feed
    lo, hi = 220., 650.
    if not balance(lo) > 0 > balance(hi):
        raise ValueError('Source data have no bracketed adsorption root in 220..650 K')
    for _ in range(60):
        mid = (lo+hi)/2
        if balance(mid) > 0: lo = mid
        else: hi = mid
    return (lo+hi)/2


def zero_pe_heat_check(*, d, L, P, ads_feed, fr_feed, gross_H2):
    """Necessary heat range under the stated zero-PE inventory assumptions.

    Last printed digits: d,L ±0.005 m; P ±0.05 international psi;
    adsorption feed/gross product ±0.5 mol; FR feed ±0.05 mol. Parameters
    y, phi, eta, eps, rho, Cv, cp, Tfeed and LRC coefficients are held fixed.
    A range containing a proposed heat does NOT establish cycle feasibility.
    """
    values = (d, L, P, ads_feed, fr_feed, gross_H2)
    if not all(math.isfinite(x) and x > 0 for x in values):
        raise ValueError('Source quantities must be finite and positive')

    def calculate(width):
        def interval(x, half): return x-width*half, x+width*half
        diam, length, pressure = interval(d,.005), interval(L,.005), interval(P,.05*PSI)
        F, FR, G = interval(ads_feed,.5), interval(fr_feed,.05), interval(gross_H2,.5)
        if min(diam+length+pressure+F+FR+G) <= 0:
            raise ValueError('Rounding interval contains nonpositive source quantities')
        V = _mul(_mul(diam, diam), _mul(length, (math.pi/4,)*2))
        total = _add(F, FR)
        # Small guard on scalar roots, much larger than roundoff here.
        Te = (_temperature(V[0],pressure[0],total[1])-1e-7,
              _temperature(V[1],pressure[1],total[0])+1e-7)
        c = _mul(_mul(pressure,V), (PHI*EPS/R,)*2)
        # Eq (13) minus (1-y)*eq (12), with void inventory eliminated.
        delta = _sub(G, _mul((1-Y,)*2,F))
        T0 = _div((1.,1.), _add(_div((1-Y,)*2,Te), _div(delta,c)))
        if T0[0] <= 0:
            raise ValueError('Source data imply a nonphysical starting temperature')
        processed = _sub(F, _div(c,Te))
        sAe = _mul((Y,)*2, _add(processed,FR))
        if processed[0] <= 0 or sAe[0] <= 0:
            raise ValueError('Source data do not enclose positive adsorption inventories')
        capacity = _mul(V, (PHI*(1-EPS)*CV,)*2)
        sensible_A = _mul(_mul(processed,(Y*CPA,)*2),_sub(Te,(TFEED,)*2))
        sensible_B = _mul(_mul(processed,((1-Y)*CPB,)*2),_sub(T0,(TFEED,)*2))
        heat = _div(_add(_mul(capacity,_sub(Te,T0)),_add(sensible_A,sensible_B)),sAe)
        return dict(adsorption_final_K=Te, adsorption_initial_K=T0, heat_J_mol=heat)

    centre, enclosure = calculate(0), calculate(1)
    return dict(scope='Necessary zero-PE adsorption check, NOT sufficient full-cycle feasibility',
                centre={key: sum(value)/2 for key,value in centre.items()},
                rounding_enclosure=enclosure, reference_heat_J_mol=20920.,
                reference_heat_excluded=not enclosure['heat_J_mol'][0] <= 20920 <= enclosure['heat_J_mol'][1],
                assumptions='Table pressures are adsorption pressures; complete regeneration; all FR methane captured; '
                            'Cen–Yang LRC; fixed Part I efficiencies, utilisation and caloric parameters',
                independent_of=['purge K', 'feed reservoir pressure', 'adsorption-exit mixing',
                                'blowdown/repressurisation thermal equations'])
