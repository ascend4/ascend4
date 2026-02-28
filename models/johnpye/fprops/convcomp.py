#!/usr/bin/env python
"""
Convert Reid, Prausnitz and Poling data from components.a4l into a form that
FPROPS can swallow. We will assume cubic equation of state with quartic
polynomial in cp0 to start with.

NOTE NOTE NOTE 
There may have been custom edits to _rpp.c, check for these before committing
changes to _rpp.c!
"""

import re, os
try:
	import periodictable as _periodictable
except ImportError:
	_periodictable = None

f = open("../../components.a4l","r").read()

rcc = re.compile(r"^MODEL\s+td_component_constants\s*\(.*^\s*SELECT\s*\(\s*component_name\s*\)(.*)^METHODS",re.MULTILINE|re.DOTALL)

scc = rcc.search(f).group(1).strip()

rc = re.compile(r"^\s*CASE",re.M|re.S)

cases = re.split(rc,scc)

r1 = re.compile(r"^\s+'([a-z0-9_]+)':(.*)",re.M|re.S)

fluids = {}

for c in cases:
	m = r1.match(c)
	if m:
		fluids[m.group(1)] = m.group(2)
	
# we have split out each fluid, now get the parameters


req = re.compile(r"^\s*([A-Za-z][A-Za-z0-9_]*)\s*:==\s*([^;]*);\s*$",re.M)

data = {}

for ff in fluids:
	i = 0
	o = {}
	s = fluids[ff]
	m = req.search(s[i:])
	while m:
		i += m.end(0)
		o[m.group(1)] = m.group(2)
		m = req.search(s[i:])	
	data[ff]=o

# number and units (-|+)?[0-9]+(.[0-9]+)?(e(+|-)?[0-9]+)?)\s*\{([^}]+\)}

ru = re.compile(r"((-)?[0-9]+(\.[0-9]+)?([Ee](\+|\-)?[0-9]+)?)\s*\{([^\}]+)\}")

class ExpectedUnits:
	def __init__(self,s,u):
		m = ru.match(s);
		if not m:
			raise RuntimeError("Invalid value '%s'"%s)
		self.val = m.group(1)
		self.units = m.group(6)
		if self.units != u:
			raise TypeError("Unexpected units '%s' should be '%s' in '%s'"%self.units,u,s)

fields = ['formula','Zc','omega']
fieldunits = {
	'Tc':'K'
	,'mw':'g/g_mole'
	,'Vc':'cm^3/g_mole'
	,'Tb':'K'
	,'Pc':'bar'
	,'Hf':'J/g_mole'
	,'Gf':'J/g_mole'
	,'cpvapa':'J/g_mole/K'
	,'cpvapb':'J/g_mole/K^2'
	,'cpvapc':'J/g_mole/K^3'
	,'cpvapd':'J/g_mole/K^4'
}

ctemplate = """
static const IdealData ideal_data_%(name)s = {
	IDEAL_CP0
	,.data = {.cp0 = {
		.cp0star = 1
		,.Tstar = 1
		,.np = 4
		,.pt = (const Cp0PowTerm[]){
			{%(cpvapa)s, 0}
			,{%(cpvapb)s, 1}
			,{%(cpvapc)s, 2}
			,{%(cpvapd)s, 3}
		}
	}}
};

static const CubicData cubic_data_%(name)s = {
	.M = %(mw)s
	,.T_c = %(Tc_K)s
	,.p_c = %(Pc_Pa)s
	,.rho_c = %(rhoc_kgm3)s
	,.T_t = %(Tt_K)s
	,.omega = %(omega)s
	,.ref0 = {FPROPS_REF_TPHG,{.tphg={%(T_ref)s, 101325, %(h_f0)s, %(g_f0)s}}}
	,.ref = {FPROPS_REF_IIR}
	,.ideal = &ideal_data_%(name)s
};

static const ElementComp elements_rpp_%(name)s[] = {
%(elements_decl)s
};

const EosData eos_rpp_%(name)s = {
	"%(name)s"
	,"%(source)s"
	,"%(url)s"
	,%(priority)d
	,FPROPS_CUBIC
	,.data = {.cubic=&cubic_data_%(name)s}
	,.elements = elements_rpp_%(name)s
	,.nelements = (int)(sizeof(elements_rpp_%(name)s) / sizeof(elements_rpp_%(name)s[0]))
};

"""

ELEMENT_SYMBOLS = {
	'H','He','Li','Be','B','C','N','O','F','Ne','Na','Mg','Al','Si','P','S','Cl','Ar','K','Ca',
	'Sc','Ti','V','Cr','Mn','Fe','Co','Ni','Cu','Zn','Ga','Ge','As','Se','Br','Kr','Rb','Sr','Y',
	'Zr','Nb','Mo','Tc','Ru','Rh','Pd','Ag','Cd','In','Sn','Sb','Te','I','Xe','Cs','Ba','La','Ce',
	'Pr','Nd','Pm','Sm','Eu','Gd','Tb','Dy','Ho','Er','Tm','Yb','Lu','Hf','Ta','W','Re','Os','Ir',
	'Pt','Au','Hg','Tl','Pb','Bi','Po','At','Rn','Fr','Ra','Ac','Th','Pa','U','Np','Pu','Am','Cm',
	'Bk','Cf','Es','Fm','Md','No','Lr','Rf','Db','Sg','Bh','Hs','Mt','Ds','Rg','Cn','Nh','Fl','Mc',
	'Lv','Ts','Og','D','T'
}

FALLBACK_ELEMENT_MASS = {
	'Al': 26.9815385, 'Ar': 39.948, 'As': 74.921595, 'B': 10.81, 'Br': 79.904,
	'C': 12.011, 'Cl': 35.45, 'D': 2.0141017781, 'F': 18.998403163, 'H': 1.008,
	'He': 4.002602, 'Hg': 200.592, 'I': 126.90447, 'Kr': 83.798, 'N': 14.007,
	'Ne': 20.1797, 'O': 15.999, 'P': 30.973761998, 'Rn': 222.0, 'S': 32.06,
	'Se': 78.971, 'Si': 28.085, 'Ti': 47.867, 'T': 3.0160492779, 'U': 238.02891, 'Xe': 131.293
}

def element_mass(symbol):
	"""Atomic mass in g/mol for one element/isotope symbol."""
	if _periodictable is not None:
		if symbol == 'D':
			return float(_periodictable.H[2].mass)
		if symbol == 'T':
			return float(_periodictable.H[3].mass)
		elem = getattr(_periodictable, symbol, None)
		if elem is not None and getattr(elem, 'mass', None) is not None:
			return float(elem.mass)
	return FALLBACK_ELEMENT_MASS.get(symbol)

def formula_mass(counts):
	m = 0.0
	for sym, cnt in counts.items():
		mass = element_mass(sym)
		if mass is None:
			return None
		m += mass * cnt
	return m

def _merge_counts(dst, src):
	out = dict(dst)
	for k, v in src.items():
		out[k] = out.get(k, 0) + v
	return out

def parse_formula(formula, mw=None, species_name=None):
	"""Return element counts from a chemical formula.

	Parses all valid element-token interpretations (including all-caps legacy
	spelling) and uses species molecular weight, when provided, to disambiguate.
	"""
	s = formula.strip()
	n = len(s)

	def parse_number(i):
		j = i
		while i < n and s[i].isdigit():
			i += 1
		return (int(s[j:i]) if i > j else 1), i

	def skip_separators(i):
		while i < n and (s[i].isspace() or s[i] in ".-·"):
			i += 1
		return i

	def parse_group(i, stop_char=None):
		i = skip_separators(i)
		if i >= n or (stop_char and i < n and s[i] == stop_char):
			return [({}, i)]

		ch = s[i]
		options = []

		if ch == '(':
			sub_results = parse_group(i + 1, ')')
			for sub_counts, j in sub_results:
				if j >= n or s[j] != ')':
					continue
				mult, k = parse_number(j + 1)
				scaled = {}
				for sym, cnt in sub_counts.items():
					scaled[sym] = cnt * mult
				options.append((scaled, k))
		elif ch.isupper():
			# one-letter element token
			one = ch.upper()
			if one in ELEMENT_SYMBOLS:
				mult, j = parse_number(i + 1)
				options.append(({one: mult}, j))
			# two-letter token (canonical or all-caps legacy)
			if i + 1 < n and s[i + 1].isalpha():
				two = ch.upper() + s[i + 1].lower()
				if two in ELEMENT_SYMBOLS:
					mult, j = parse_number(i + 2)
					options.append(({two: mult}, j))
		else:
			raise ValueError("Unexpected character '%s' in formula '%s'" % (ch, formula))

		if not options:
			raise ValueError("Unknown element token near '%s' in formula '%s'" % (s[i:i+2], formula))

		results = []
		for term_counts, j in options:
			try:
				rem_results = parse_group(j, stop_char)
			except ValueError:
				continue
			for rem_counts, k in rem_results:
				results.append((_merge_counts(term_counts, rem_counts), k))
		if not results:
			raise ValueError("Unable to parse formula '%s'" % formula)
		return results

	cands = parse_group(0, None)
	complete = []
	for counts, i in cands:
		i = skip_separators(i)
		if i == n:
			complete.append(counts)

	if not complete:
		raise ValueError("Unparsed remainder in formula '%s'" % formula)

	# de-duplicate identical compositions
	uniq = []
	seen = set()
	for counts in complete:
		key = tuple(sorted(counts.items()))
		if key in seen:
			continue
		seen.add(key)
		uniq.append(counts)

	chosen = uniq[0]
	if mw is not None:
		best = None
		for counts in uniq:
			mass = formula_mass(counts)
			if mass is None:
				continue
			err = abs(mass - mw)
			if best is None or err < best[0]:
				best = (err, counts, mass)
		if best is not None:
			chosen = best[1]

	if mw is not None:
		mass = formula_mass(chosen)
		if mass is not None:
			err = abs(mass - mw)
			if err > 0.25:
				tag = species_name if species_name else formula
				print("WARNING: formula/mw mismatch for '%s': formula=%s mw=%.6g est=%.6g err=%.6g" % (
					tag, formula, mw, mass, err
				))

	return chosen

class CubicFluid:
	def __init__(self,name,o):
		self.name = name
		for x in fields:
			try:
				setattr(self,x,o[x])
			except KeyError as e:
				pass#print "%s: missing key '%s'"%(name,str(e))
			except Exception as e:
				pass#print str(e)
		for x in fieldunits:
			try:
				setattr(self,x,ExpectedUnits(o[x],fieldunits[x]).val)
			except KeyError as e:
				pass#print "%s: missing key '%s'"%(name,str(e))
			except Exception as e:
				print(str(e))
		self.o = o
	def whyfail(self):
		for n in ['Tc','mw','Pc','omega','cpvapa','cpvapb','cpvapc','cpvapd']:
			if not hasattr(self,n):
				return n
		return None
	def ccode(self):
		pc = '-1'
		if hasattr(self,'Pc'):pc = '(%s * 1e5)'%self.Pc
		rhoc = '-1'
		if hasattr(self,'Vc'):rhoc = '(1000 * %s / %s)'%(self.mw,self.Vc)
		h_f0 = 'NAN'
		if hasattr(self,'Hf'):h_f0 = '(%s / %s)'%(self.Hf,self.mw)
		g_f0 = 'NAN'
		if hasattr(self,'Gf'):g_f0 = '(%s / %s)'%(self.Gf,self.mw)
		
		formula = self.formula.strip().strip("'\"")
		counts = parse_formula(formula, mw=float(self.mw), species_name=self.name)
		elems = sorted(counts.items())
		decl_lines = []
		for idx, (sym, cnt) in enumerate(elems):
			prefix = "\t"
			if idx > 0:
				prefix += ","
			decl_lines.append('%s{"%s", %d}' % (prefix, sym, cnt))
		return ctemplate % {
			'name':self.name
			,'source':'RPP'#'Reid, Prausnitz, and Poling, 1987, The Properties of '+
			#' Gases and Liquids, 4th Edition, McGraw-Hill (used with permission)'
			,'url':''#'http://code.ascend4.org/viewvc/code/trunk/models/components.a4l'
			,'priority':40
			,'mw':self.mw
			,'Tc_K':self.Tc
			,'Pc_Pa':pc
			,'rhoc_kgm3':rhoc
			,'Tt_K' : 0
			,'T_ref' : 298.2
			,'omega':self.omega
			,'h_f0':h_f0
			,'g_f0':g_f0
			,'cpvapa':self.cpvapa
			,'cpvapb':self.cpvapb
			,'cpvapc':self.cpvapc
			,'cpvapd':self.cpvapd
			,'elements_decl':'\n'.join(decl_lines)
		}

cf = {}

nfail = 0
for d in data:
	f = CubicFluid(d,data[d])
	nn = f.whyfail()
	if not nn:
		cf[d] = f
	else:
		print("Failing '%s' for missing '%s'" % (d,nn))
		nfail += 1
print("Found %d good fluids (rejected %d others due to missing data)"%(len(cf),nfail))

#for d in cf:
#	print d,cf[d].Tc,cf[d].mw,cf[d].Pc,cf[d].omega,cf[d].cpvapa,cf[d].cpvapb,cf[d].cpvapc,cf[d].cpvapd

f = open("fluids/_rpp.c","w")
f.write("""/* this is an autogenerated file... do not edit! */

/*  Copyright (C) 2011 Carnegie Mellon University

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
... MCGRAW-HILL LICENSE ...

	Data in this file, except as OTHERWISE noted in the code given, are taken
	from THE BOOK:

		The Properties of Gases and Liquids, 4th Edition
		by Reid, Prausnitz, and Poling, Copyright 1987 McGraw-Hill Companies,

	with the publisher's permission dated July 20, 1998.
	This file must not be modified, distributed, or otherwise used
	without this License notice attached. The conditions of this license,
	and the GNU Public License, both apply.

	Terms from McGraw-Hill:
	M1. The data may not be used outside of ASCEND and derivative works.
	M2. McGraw-Hill makes no representations or warranties as to the
		accuracy of any information contained in THE BOOK, or in this
		transcription of it, including any warranties of merchantability
		or fitness for a particular purpose. In no event shall McGraw-Hill
		have any liability to any party for special, incidental, tort, or
		consequential damages arising out of or in connection with THE
		BOOK or this transcription of it, even if McGraw-Hill has been
		advised of the possibility of such damages. All users of ASCEND
		(or any derivative work) must be provided with written notice of
		this disclaimer and limitation on liability in the end-user license
		of ASCEND or any derivative work.
	M3. Credit to McGraw-Hill and the authors of THE BOOK shall be visible
		each time ASCEND is accessed, and at all other reasonable points.
		Such credit shall include the copyright notice of the McGraw-Hill
		Companies.
	M4. Any work incorporating this information in any way on the WWW
		(Internet) shall include a hypertext reference to:
		http://www.bookstore.mcgraw-hill.com

... end of McGraw-Hill License ...

...	CMU DISCLAIMER ...

	The authors of ASCEND and Carnegie Mellon University make
	absolutely NO WARRANTY about the accuracy of this transcription
	of the RPP data or of the original data itself, nor do they
	provide any guarantee that the data here represented is
	suitable for any purpose academic or commercial.

... end of CMU disclaimer ...
*/

#include "../filedata.h"

""")

for d in cf:
	f.write(cf[d].ccode())
f.close()

f = open("fluids/_rpp.h","w")
f.write("/* this is an autogenerated file... do not edit! */\n\n")
f.write("\n\n#define RPPFLUIDS(F,X)")
first=1
for d in cf:
	s = "\\\n\t"
	if not first:
		s += "X "
	else:
		first = 0
	f.write("%sF(%s)"%(s,cf[d].name))
f.write("\n\n/* end of auto-generated file */\n")
f.close()
	

#print d,f.mw,f.Tc
