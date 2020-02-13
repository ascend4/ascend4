# coding=utf-8
import matplotlib
matplotlib.use('GtkAgg')
import matplotlib.pyplot as pl
import numpy as np
import math

class Label:
	def __init__(self,ix,iy,i):
		self.ix = ix
		self.iy = iy
		self.i = i
	def __repr__(self):
		return "{%d,%d}"%(self.ix,self,iy)

class Node:
	def __init__(self,x,y,lab):
		self.x = x
		self.y = y
		self.lab = lab
		self.links_to = []
		self.links_from = []
	def distance_from(self,cnode):
		return math.sqrt((cnode.x-self.x)**2 + (cnode.y-self.y)**2)

	def __repr__(self):
		return "<Node %s @%.0f,%.0f>" % (repr(self.lab),self.x, self.y)

class Link:
	def __init__(self,lab1,lab2,id):
		self.lab1 = lab1
		self.lab2 = lab2
		self.id = id
	def __repr__(self):
		return "<Link %d: %s--%s>" % (self.id,repr(self.lab1),repr(self.lab2))
	def plot(self,arr,xsep,ysep):
		#print [arr[self.lab1].x,arr[self.lab2].x],[arr[self.lab1].y,arr[self.lab1].y]
		x = arr[self.lab1].x
		y = arr[self.lab1].y
		dx = arr[self.lab2].x - arr[self.lab1].x
		dy = arr[self.lab2].y - arr[self.lab1].y
		l = math.sqrt(dx**2+dy**2)
		hw = 0.2 * xsep
		hl = 1 * hw
		l1 = l + hl
		dx *= l/l1
		dy *= l/l1
		pl.arrow(x,y,dx,dy,width=xsep*0.03,head_width=hw,head_length=hl,color='#ff0000')
		pl.annotate("id=%d"%(self.id,),(x+dx/2,y+dy/2))
	def node_from(self,arr):
		return arr[self.lab1]
	def node_to(self,arr):
		return arr[self.lab2]
	def reverse(self):
		l = self.lab1
		self.lab1 = self.lab2
		self.lab2 = l
	def length(self,arr):
		dx = arr[self.lab2].x - arr[self.lab1].x
		dy = arr[self.lab2].y - arr[self.lab1].y
		l = math.sqrt(dx**2+dy**2)
		return l

def create_field_model(nx,ny,Qdot_onecoll_MW=10.,mdot_onecoll=None
		,ix_pb=None,iy_pb=None,suffix=None,f=None):
	xsep = 290.
	ysep = 240.

	if f is None:
		import sys
		f = sys.stdout
	if ix_pb is None:
		ix_pb = int(nx)/2
	if iy_pb is None:
		iy_pb = int(ny)/2
	if mdot_onecoll is None:
		mdot_onecoll = 40 * (Qdot_onecoll_MW / 10)
	if suffix is None:
		suffix = "_%dx%d_%fMW"%(nx,ny,Qdot_onecoll_MW)
	
	ix = np.arange(0,nx) 
	x = ix*xsep;
	iy = np.arange(0,ny)
	y = iy*ysep;
	IX,IY = np.meshgrid(ix,iy)
	X,Y = np.meshgrid(x,y)
	lab = zip(IX.ravel(),IY.ravel())
	loc = zip(X.ravel(),Y.ravel())

	arr = {}
	for i,l in enumerate(lab):
		print "node label",l
		arr[l] = Node(x=loc[i][0],y=loc[i][1],lab=l)

	print "arr =",arr

	centre = arr[(ix_pb,iy_pb)]

	# create the link network...
	links = []
	id = 0
	for ix1 in ix:
		print "col",ix1
		for iy1 in np.arange(ny-1):
			l = Link((ix1,iy1),(ix1,iy1+1),id); id += 1
			print "row",iy1,"to",iy1+1,":",l
			links.append(l)
		if ix1 != 0:
			print ix1
			links.append(Link((ix1-1,iy_pb),(ix1,iy_pb),id)); id += 1

	print links

	f.write("""
MODEL layout{suffix};
	n_DI IS_A integer_constant;
	n_DI :== {n};
	n_PH IS_A integer_constant;
	n_PH :== {nl};
	n_PC ALIASES n_PH;
	DI[0..n_DI-1] IS_A boiler_simple;
	TH[0..n_DI-1] IS_A throttle;
	PH[0..n_PH-1] IS_A pipe_heat_loss_insul;
	PC[0..n_PC-1] IS_A pipe_heat_loss_insul;
	JO[0..n_DI-1] IS_A merge_generic;
	TE[0..n_DI-1] IS_A tee_generic;

	PC[0..n_PH-1].config, PH[0..n_PH-1].config ARE_THE_SAME;

	corr_intconv ALIASES PC[0].config.corr_intconv;
	k_pipe ALIASES PC[0].config.k_pipe;
	k_insul ALIASES PC[0].config.k_insul;	
	h_ext ALIASES PC[0].config.h_ext;
	emiss ALIASES PC[0].config.emiss;
	eps ALIASES PC[0].config.eps;

	corr_intconv :== 'none';\n\n""".format(
		n=len(arr)
		,nl=len(links)
		,suffix=suffix
	))

	# reverse mappings

	rarr = {}
	rlinks = {}
	for i,lab in enumerate(arr.keys()):
		rarr[lab] = i
	for i,l in enumerate(links):
		rlinks[l] = i

	# create the ASCEND code...

	if 0:
		f.write("\tNOTES\n");
		# output the positions of each collector (in comments)
		for i,lab in enumerate(arr.keys()):
			print i
			f.write("\t\t'location' DI[{i}].inlet {{{x},{x}}};\n".format(
			i=i
			,x=arr[lab].x
			,y=arr[lab].y
		))
		f.write("\tEND NOTES;\n\n");

	for i,l in enumerate(links):
		A = l.node_from(arr)
		B = l.node_to(arr)
		print "link",l,": A_dist = %f, B_dist = %f" % (A.distance_from(centre), B.distance_from(centre))
		if A.distance_from(centre) > B.distance_from(centre):
			l.reverse()
			B.links_from.append(l)
			A.links_to.append(l)
			print "link reverse",l
		else:
			A.links_from.append(l)
			B.links_to.append(l)

	for i,n in enumerate(arr):
		assert len(arr[n].links_to) <= 1
	#		print "NOTE: Too many links_to node %s: %s"%(n,arr[n].links_to)

		f.write("\t(* links at node {i} {lab} *)\n".format(i=i,lab=n))

		if len(arr[n].links_from) == 0:
			f.write("\tTE[{i}] IS_REFINED_TO tee_trivial;\n\tJO[{i}] IS_REFINED_TO merge_trivial;\n".format(i=i))
		else:
			f.write("\tTE[{i}] IS_REFINED_TO tee_n({nout});\n\tJO[{i}] IS_REFINED_TO merge_n({nout});\n".format(
				i=i
				,lab = n
				,nin = len(arr[n].links_to)
				,nout = len(arr[n].links_from)+1
			))

		for j,lt in enumerate(arr[n].links_to):
			f.write("\tTE[{i}].inlet, PC[{li}].outlet ARE_THE_SAME;\n".format(
				i=i
				,li=rlinks[lt]
			))

		f.write("\tTE[{i}].outlet[0], DI[{i}].inlet ARE_THE_SAME;\n".format(i=i))
		for j,lt in enumerate(arr[n].links_from):
			f.write("\tTE[{i}].outlet[{j}], PC[{li}].inlet ARE_THE_SAME;\n".format(
				i=i
				,j=j+1
				,li=rlinks[lt]
			))

		f.write("\tDI[{i}].outlet, TH[{i}].inlet ARE_THE_SAME;\n".format(i=i));
		f.write("\tTH[{i}].outlet, JO[{i}].inlet[0] ARE_THE_SAME;\n".format(i=i));

		if len(arr[n].links_from) == 0:
			f.write("\tTH[{i}] IS_REFINED_TO throttle_trivial;\n".format(i=i));

		for j,lt in enumerate(arr[n].links_from):
			f.write("\tJO[{i}].inlet[{j}], PH[{li}].outlet ARE_THE_SAME;\n".format(
				i=i
				,j=j+1
				,li=rlinks[lt]
			))

		for j,lt in enumerate(arr[n].links_to):
			f.write("\tJO[{i}].outlet, PH[{li}].inlet ARE_THE_SAME;\n".format(
				i=i
				,li=rlinks[lt]
			))



	import sys
	print "centre.lab",centre.lab
	print "rarr[centre.lab]",rarr[centre.lab]


	f.write("""
	inlet, outlet IS_A stream_node;
	inlet, TE[{i}].inlet ARE_THE_SAME;
	outlet, JO[{i}].outlet ARE_THE_SAME;

	cd ALIASES inlet.cd;
	cd.component :== 'sodium';
	cd.type :== 'incomp';

METHODS
	""".format(
		i=rarr[centre.lab]
	))

	f.write("METHOD set_pipe_lengths;\n");
	for i,l in enumerate(links):
		f.write("""\tFIX PH[{i}].L := {L} {{m}};
		FIX PC[{i}].L := {L} {{m}};\n""".format(
			i=i
			,L = l.length(arr)
		))
	f.write("END set_pipe_lengths;\n");


	f.write("""
METHOD default_self;
	FOR i IN [0..n_DI-1] DO
		RUN TE[i].default_self;
		RUN DI[i].default_self;
		RUN TH[i].default_self;
		RUN JO[i].default_self;
	END FOR;
	FOR i IN [0..n_PH-1] DO
		RUN PH[i].default_self;
		RUN PC[i].default_self;
	END FOR;
END default_self;
END layout{suffix};
	""".format(
		i=rarr[centre.lab]
		,suffix=suffix
	))

	# note, we don't use str.format() here because lots of units with curly {}:
	f.write("""
UNITS
(* currency conversions as of 10 Feb 2020 *)
AUD = {USD/1.49};
EUR = {1.10*USD};
END UNITS;
	""");

	f.write("""
MODEL towerarray{suffix} REFINES layout{suffix};
	""".format(suffix=suffix))

	f.write("""
	PC[0..n_PH-1].Vel_out, PH[0..n_PH-1].Vel_out ARE_THE_SAME;
	Vel ALIASES PC[0].Vel_out;

	PC[0..n_PH-1].t_insul, PH[0..n_PH-1].t_insul ARE_THE_SAME;

	FOR i IN [0..n_PH-1] CREATE
		(* curve fit to Sch10S pipe sizes *)
		PC[i].t_pipe = 1{mm} * (PC[i].D/1{mm})^0.3 / 1.222;
		PH[i].t_pipe = 1{mm} * (PH[i].D/1{mm})^0.3 / 1.222;
	END FOR;
	(*PC[0..n_PH-1].t_pipe, PH[0..n_PH-1].t_pipe ARE_THE_SAME;
	t_pipe ALIASES PC[0].t_pipe;*)
	t_insul ALIASES PC[0].t_insul;


	(* total materials *)
	m_pipe, m_insul, m_sodium IS_A mass;
	V_insul IS_A volume;
	rho_pipe, rho_insul IS_A mass_density;
	L_pipe_tot IS_A distance;
	m_pipe = rho_pipe * SUM[PC[i].solid_pipe.V + PH[i].solid_pipe.V | i IN [0..n_PH-1]];
	V_insul = SUM[PC[i].solid_insul.V + PH[i].solid_insul.V | i IN [0..n_PH-1]];
	L_pipe_tot = SUM[PC[i].L + PH[i].L | i IN [0..n_PH-1]];
	m_sodium = SUM[PC[i].solid.V / PC[i].inlet.v + PH[i].solid.V / PH[i].inlet.v | i IN [0..n_PH-1]];
	m_insul = rho_insul * V_insul;

	(* costs *)
	C_pipe, C_insul, C_inst_pipe, C_inst_insul, C_supp, C_sodium, C_tot IS_A monetary_unit;
	c_pipe IS_A cost_per_mass;
	c_insul IS_A cost_per_volume;
	C_pipe = c_pipe * m_pipe;
	C_insul = c_insul * V_insul;
	C_sodium = c_sodium * m_sodium;	

	c_inst_pipe_0, c_inst_insul_0, c_supp_0 IS_A cost_per_length;
	c_inst_pipe_1, c_inst_insul_1 IS_A cost_per_area;
	c_sodium IS_A cost_per_mass;

	C_inst_pipe = SUM[PC[i].L * (c_inst_pipe_1*PC[i].D_o + c_inst_pipe_0) 
					+ PH[i].L * (c_inst_pipe_1*PH[i].D_o + c_inst_pipe_0) | i IN [0..n_PH-1]];

	C_inst_insul = SUM[PC[i].L * (c_inst_insul_1*PC[i].D_o + c_inst_insul_0) 
					 + PH[i].L * (c_inst_insul_1*PH[i].D_o + c_inst_insul_0) | i IN [0..n_PH-1]];

	C_supp = c_supp_0 * SUM[PC[i].L + PH[i].L | i IN [0..n_PH-1]];

	C_tot = C_pipe + C_insul + C_inst_pipe + C_inst_insul + C_supp;

	T_amb IS_A temperature;
	T_amb, PC[0..n_PH-1].T_amb, PH[0..n_PH-1].T_amb ARE_THE_SAME;

	Qdot_onecoll IS_A energy_rate;
	Qdot_onecoll, DI[0..n_DI-1].Qdot ARE_THE_SAME;
	DI[0..n_DI-1].eta ARE_THE_SAME; eta_DI ALIASES DI[1].eta;

	Qdot_coll_tot IS_A energy_rate;
	Qdot_coll_tot = SUM[DI[i].Qdot | i IN [0..n_DI-1]];

	Qdot_loss_tot IS_A energy_rate;
	Qdot_loss_tot = SUM[PC[i].Q + PH[i].Q|i IN [0..n_PH-1]];

	Qdot_net IS_A energy_rate;
	Qdot_net = outlet.mdot*outlet.h - inlet.mdot*inlet.h;

	eta_th_array IS_A fraction;
	eta_th_array = Qdot_net / Qdot_coll_tot;

	(* try: same mass flow for each dish *)
	DI[0..n_DI-1].mdot ARE_THE_SAME;
	mdot_onecoll IS_A mass_rate;
	mdot_onecoll, DI[0].mdot ARE_THE_SAME;

	T_in ALIASES inlet.T;
	T_out ALIASES outlet.T;

	p_in ALIASES inlet.p;
	p_out ALIASES outlet.p;
	dp IS_A delta_pressure;
	dp = p_in - p_out;

METHODS
METHOD on_load;
	RUN default_self;

	RUN set_pipe_lengths;

	FIX Qdot_onecoll := %f {MW}; (* value substituted in script! *)
	FIX eta_DI := 1;

	FIX Vel := 15 {ft/s}; (* NAK Hbk v3 p6: 4-16" size, 15 *)

	(*FIX t_pipe := 4 {mm}; (* FIXME implement equation here *)*)
	FIX t_insul := 100 {mm};

	FIX eps := 0.09 {mm};
	FIX h_ext := 10 {W/m^2/K};
	FIX emiss := 0.8;
	FIX k_insul := 0.05 {W/m/K}; (* 'microporous insulation board' @ 700°C, https://is.gd/5j9Gkw *)
	FIX k_pipe := 22.4 {W/m/K}; (* Haynes 230 @ 700°C: https://is.gd/F0RICh *)
	FIX rho_pipe := 9 {g/cm^3}; (* Haynes 230 @ 700°C: https://is.gd/F0RICh *)
	FIX rho_insul := 380 {kg/m^3}; (* 'microporous insulation board', approx value from Felix *)

	FIX c_pipe := 8 {USD/kg} (* 316 stainless steel assumed *);

	FIX c_insul := 10000 {USD/m^3}; (* old quote from brandname product...or maybe 250 USD/kg if alibaba.com *)
	FIX c_inst_pipe_0 := 22 {AUD/m};
	FIX c_inst_pipe_1 := 0.3734 {AUD/m/mm};
	FIX c_inst_insul_0 := 29 {AUD/m};
	FIX c_inst_insul_1 := 0.6257 {AUD/m/mm};
	FIX c_supp_0 := 100 {USD/m}; (* this one's in yankee dollars *)
	FIX c_sodium := 3000 {USD/t}; (* based on a quick look at alibaba https://is.gd/pPKCYt *)

	(*
	installation cost for pipe, per length installed
	0.3734 AUD/m/mm * d_o_pipe + 22 AUD/m

	installation cost for insulation, per length installed
	0.6257 AUD/m/mm * d_o_pipe + 29 AUD/m

	anchors and supports 
		- supports with 2.5 m spacing, 175 AUD/support, plus
		- 400 AUD/anchor, one per every second support.
	totals to 100 USD/m.

	insulation cost
	10000 USD/m^3 (or maybe 250....?)
	*)

	FIX inlet.p := 50 {bar};
	FIX inlet.T := 520 {K} +  273.15 {K};
	FIX mdot_onecoll := %f {kg/s}; (* same for all dishes *)
	FIX T_amb := 300 {K};

	(* initial guesses to help the solver *)
	PC[0..n_PH-1].inlet.h := 400 {kJ/kg};

	SOLVER QRSlv;
	OPTION convopt 'RELNOM_SCALE';	
END on_load;
METHOD correct_dp_and_Tout;
	FREE Vel;
	FIX p_in := 5 {bar};
	FIX p_out := 3 {bar};
	FREE mdot_onecoll;
	FIX T_out := 740 {K} + 273.15{K};
END correct_dp_and_Tout;
METHOD show_temperatures;
	EXTERNAL disharray_temperature_list(SELF);
END show_temperatures;
	""" % (Qdot_onecoll_MW,mdot_onecoll));

	f.write("END towerarray{suffix};\n".format(suffix=suffix));

	pl.figure()
	pl.axes().set_aspect('equal', 'datalim')	
	pl.plot(X,Y,'bo')
	pl.axis([x.min() - xsep/2, x.max() + xsep/2, y.min() - ysep/2, y.max() + ysep/2])
	for lab in arr:
		pl.annotate("%s\n%d"%(lab,rarr[lab]),(arr[lab].x,arr[lab].y),ha='right')

	for l in links:
		print "link",l
		l.plot(arr,xsep,ysep)

if __name__=='__main__':

	f = open("layout.a4c","w")
	f.write("""
REQUIRE "johnpye/fprops/pipe.a4c";
	""");

	create_field_model(1,4,iy_pb=0,suffix="_chain1",f=f)

	create_field_model(5,1,ix_pb=0,Qdot_onecoll_MW=70,suffix="_chain2",f=f)

	f.close()

	pl.show()


