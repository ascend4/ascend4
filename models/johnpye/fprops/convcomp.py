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

const EosData eos_rpp_%(name)s = {
	"%(name)s"
	,"%(source)s"
	,"%(url)s"
	,%(priority)d
	,FPROPS_CUBIC
	,.data = {.cubic=&cubic_data_%(name)s}
};

"""

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
