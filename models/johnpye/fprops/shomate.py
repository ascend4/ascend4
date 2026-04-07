import numpy as np

# ----------------------------
# Constants / user choices
# ----------------------------
TREF = 298.15
PREF = 1.0e5  # Pa (0.1 MPa)
TLO = 873.15  # 600 C
THI = 1173.15 # 900 C
NSAMP = 2001

# Molar masses (kg/mol)
M_NI  = 58.6934e-3
M_NIO = 74.6928e-3

# Reference anchors at 298.15 K (molar basis)
# Ni(cr): hf = 0 by element reference convention; S from OECD
Hf_NI_molar  = 0.0                 # J/mol
S_NI_molar   = 29.87               # J/mol/K  (OECD)

# NiO(cr): from OECD selections
Hf_NIO_molar = -239.7e3            # J/mol    (OECD)
S_NIO_molar  = 38.4                # J/mol/K  (OECD)

# Optional densities (kg/m3) (room temp typical values)
rho_NI  = 8908.0   # Nickel metal density
rho_NIO = 6670.0   # NiO density (6.67 g/cm3)

# ----------------------------
# Cp correlations (OECD/NEA)
# ----------------------------

def Cp_Ni_molar(T):
    """
    Cp,m°(T) for Ni(cr), piecewise:
      Cp = a + b T + c T^2 + e T^-2
    Coefficients from OECD/NEA vol6-nickel, Table V-1.
    """
    # intervals: [298,450], [450,600], [600,631], [631,640], [640,690], [690,1728]
    pieces = [
        (298.0, 450.0,  3.36472e+01, -2.47530e-02,  4.35458e-05, -3.61955e+05),
        (450.0, 600.0, -4.93235e+01,  1.676480e-01,-7.48518e-05,  3.76214e+06),
        (600.0, 631.0,  1.05023e+05, -2.29221e+02,  1.40772e-01, -6.52809e+09),
        (631.0, 640.0,  5.29406e+04, -1.65828e+02,  1.29936e-01,  0.0),
        (640.0, 690.0,  1.82638e+02, -4.20407e-01,  2.90521e-04,  0.0),
        (690.0, 1728.0, 1.23677e+01,  2.22800e-02,  4.45300e-06,  2.46766e+06),
    ]
    for Tmin, Tmax, a, b, c, e in pieces:
        if Tmin <= T <= Tmax:
            return a + b*T + c*T*T + e*(T**-2)
    # if slightly out of range, clamp to nearest valid piece
    if T < 298.0:
        Tmin, Tmax, a, b, c, e = pieces[0]
        return a + b*Tmin + c*Tmin*Tmin + e*(Tmin**-2)
    Tmin, Tmax, a, b, c, e = pieces[-1]
    return a + b*Tmax + c*Tmax*Tmax + e*(Tmax**-2)

def Cp_NiO_molar(T):
    """
    Cp,m°(T) for NiO(cr), selected as:
      298.15–519 K: Cp = 4110.64 - 5.302412 T + 3.52061e-3 T^2 - 53039.297 T^-0.5 + 2.43067e7 T^-2
      519–1800 K  : Cp = -8.776 + 4.2232e-2 T - 7.5267e-6 T^2 + 787.25 T^-0.5 + 3.6067e6 T^-2
    """
    if T < 519.0:
        return (4110.64
                - 5.302412*T
                + 3.52061e-3*T*T
                - 53039.297*(T**-0.5)
                + 2.43067e7*(T**-2))
    else:
        return (-8.776
                + 4.2232e-2*T
                - 7.5267e-6*T*T
                + 787.25*(T**-0.5)
                + 3.6067e6*(T**-2))

# ----------------------------
# Helper: mean Cp over window
# ----------------------------
def mean_cp_molar(cp_func, Tlo, Thi, n=2001):
    Ts = np.linspace(Tlo, Thi, n)
    Cps = np.array([cp_func(T) for T in Ts])
    return Cps.mean()

# Compute mean molar Cp over the user window
CpNi_m = mean_cp_molar(Cp_Ni_molar,  TLO, THI, NSAMP)
CpNiO_m = mean_cp_molar(Cp_NiO_molar, TLO, THI, NSAMP)

# Convert to mass basis
cp_NI  = CpNi_m  / M_NI   # J/kg/K
cp_NIO = CpNiO_m / M_NIO  # J/kg/K

# Convert reference anchors to mass basis
href_NI  = Hf_NI_molar  / M_NI
sref_NI  = S_NI_molar   / M_NI
href_NIO = Hf_NIO_molar / M_NIO
sref_NIO = S_NIO_molar  / M_NIO

print("Ni(cr):")
print("  cp =", cp_NI, "J/kg/K")
print("  href =", href_NI, "J/kg at", TREF)
print("  sref =", sref_NI, "J/kg/K at", TREF)
print("  rho =", rho_NI, "kg/m3")

print("NiO(cr):")
print("  cp =", cp_NIO, "J/kg/K")
print("  href =", href_NIO, "J/kg at", TREF)
print("  sref =", sref_NIO, "J/kg/K at", TREF)
print("  rho =", rho_NIO, "kg/m3")
