/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/
/**
	@file
	Unit test functions reverse ad & second derivatives
	Created by: Mahesh Narayanamurthi
	Revised by: Ben Allan - Rev.1
				Mahesh Narayanamurthi - Rev. 2
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ascend/general/env.h>
#include <ascend/general/ospath.h>
#include <ascend/general/list.h>
#include <ascend/general/ltmatrix.h>

#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/relation_io.h>
#include <ascend/compiler/reverse_ad.h>
#include <ascend/compiler/relation_util.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/watchpt.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/visitinst.h>
#include <ascend/compiler/functype.h>
#include <ascend/compiler/safe.h>
#include <ascend/compiler/qlfdid.h>
#include <ascend/compiler/instance_io.h>
// FIXME include anything else?

#include <test/common.h>
#include <test/assertimpl.h>
#include <test/test_globals.h>

#define RAD_TOL 1e-05

extern char ASC_TEST_PATH[PATH_MAX];

struct FileList
{
	FILE * yacas; /* Meant for YACAS */
	FILE * safeder; /* Meant for Safe Der */
	FILE * nonsafeder; /* Meant for Non-Safe Der */
};

struct TestFileList
{
	FILE * FirstDerYacas;
	FILE * SecondDerYacas;
};

struct DiffTestData
{
	FILE * outfile; /* Meant for Logging Debug Info */
	FILE * varfile; /* Meant for recording the Variable Values */
	struct FileList FirstDer;
	struct FileList SecondDer;
	int use_yacas;
	FILE * first_yacas;
	FILE * second_yacas;
	struct Instance *root;
	const char *infile;
	int d0errors;
	int d1errors;
	int d1errors_yacas;
	int d2errors_yacas;
};



#define INITDTD(d, log, varfile, FirstDer, SecondDer, use_yacas, YacasInFirst, YacasInSecond ,instroot, inputname) \
do { \
	d.outfile = log; \
	d.varfile = varfile; \
	d.FirstDer = FirstDer;\
	d.SecondDer = SecondDer;\
	d.use_yacas = use_yacas; \
	d.first_yacas = YacasInFirst;\
	d.second_yacas = YacasInSecond;\
	d.root = instroot; \
	d.infile = inputname; \
	d.d0errors= 0; \
	d.d1errors= 0; \
	d.d1errors_yacas= 0; \
	d.d2errors_yacas= 0; \
} while (0)

#define LOG(d, ...) do { \
	if ( d != NULL && d-> outfile != NULL) { \
		fprintf(d->outfile,  __VA_ARGS__); \
	} \
	} while ( 0 )

/** Temporary Declarations */
static void AutomateDiffTest(struct Instance *inst, VOIDPTR ptr);


static void test_autodiff(void){
#define OUTENV "../ascend/compiler/test/LOG.html"
#define VARFILE "../ascend/compiler/test/Vars.txt"

#define SAFEDER_2ND "../ascend/compiler/test/Safes2nd.txt"
#define NONSAFEDER_2ND "../ascend/compiler/test/Nonsafes2nd.txt"
#define YACAS_2ND "../ascend/compiler/test/Yacas2nd.txt"

#define SAFEDER_1ST "../ascend/compiler/test/Safes1st.txt"
#define NONSAFEDER_1ST "../ascend/compiler/test/Nonsafes1st.txt"
#define YACAS_1ST "../ascend/compiler/test/Yacas1st.txt"

#define YACAS_IN_1ST "../ascend/compiler/test/FirstDeriv.txt"
#define YACAS_IN_2ND "../ascend/compiler/test/SecondDeriv.txt"

#define CASEFILE "test/reverse_ad/allmodels.a4c"

#define USE_YACAS_ENV "ASC_YACAS_GEN"

	int status;
	int use_yacas = 0;

	//struct module_t *m;

	struct Name *name;
	enum Proc_enum pe;
	struct Instance *root;

	struct DiffTestData data;

	struct FilePath* out_osp;
	struct FilePath* varfile_osp;

	struct FilePath* safe_osp_2nd;
	struct FilePath* yacas_osp_2nd;
	struct FilePath* nonsafe_osp_2nd;

	struct FilePath* safe_osp_1st;
	struct FilePath* yacas_osp_1st;
	struct FilePath* nonsafe_osp_1st;

	struct FilePath* first_yacas_osp;
	struct FilePath* second_yacas_osp;


	FILE * outfile = NULL;
	FILE * varfile = NULL;

	FILE * first_yacas = NULL;
	FILE * second_yacas = NULL;

	struct FileList FirstDer;
	struct FileList SecondDer;

	FirstDer.yacas = FirstDer.safeder = FirstDer.nonsafeder = NULL;
	SecondDer.yacas = SecondDer.safeder = SecondDer.nonsafeder = NULL;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");


	// FIXME Use the environment variables here
	CONSOLE_DEBUG("ASC_TEST_PATH = '%s'",ASC_TEST_PATH);

	struct FilePath *rootfp;
	{
		CONSOLE_DEBUG("setting root path");
		struct FilePath *tmp = ospath_new(ASC_TEST_PATH);
		rootfp = ospath_getabs(tmp);
		ospath_free(tmp);
		char *str = ospath_str(rootfp);
		CONSOLE_DEBUG("ROOTFP = %s",str);
		ASC_FREE(str);
	}

#define OPENTESTFILE(FNAME,OSP,VAR,MODE) {\
		struct FilePath *tmp = ospath_new_noclean(FNAME);\
		CONSOLE_DEBUG("file = %s",FNAME);ospath_debug(tmp);\
		CONSOLE_DEBUG("concat with root...");\
		ospath_debug(rootfp);\
		CONSOLE_DEBUG("results...");\
		OSP = ospath_concat(rootfp,tmp);\
		ospath_cleanup(OSP);\
		ospath_debug(OSP);\
		VAR = ospath_fopen(OSP,"w");\
		CU_ASSERT_PTR_NOT_NULL_FATAL(VAR);\
		ospath_free(tmp);\
	}

	OPENTESTFILE(OUTENV,out_osp,outfile,"w");

	/** @TODO Open the following streams only if Environment Variable is set */
	if(getenv(USE_YACAS_ENV) != NULL ){
		OPENTESTFILE(VARFILE,varfile_osp,varfile,"w");
		OPENTESTFILE(YACAS_2ND,yacas_osp_2nd,SecondDer.yacas,"w");
		OPENTESTFILE(SAFEDER_2ND,safe_osp_2nd,SecondDer.safeder,"w");
		OPENTESTFILE(NONSAFEDER_2ND,nonsafe_osp_2nd,SecondDer.nonsafeder,"w");
		OPENTESTFILE(YACAS_1ST,yacas_osp_1st,FirstDer.yacas,"w");
		OPENTESTFILE(SAFEDER_1ST,safe_osp_1st,FirstDer.safeder,"w");
		OPENTESTFILE(NONSAFEDER_1ST,nonsafe_osp_1st,FirstDer.nonsafeder,"w");
		use_yacas=1;
	}else{
		OPENTESTFILE(YACAS_IN_1ST,first_yacas_osp,first_yacas,"r");
		OPENTESTFILE(YACAS_IN_2ND,second_yacas_osp,second_yacas,"r");
	}

	ospath_free(rootfp);

	/* load the file */
	Asc_OpenModule(CASEFILE,&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	symchar * type = AddSymbol("allmodels");
	CU_ASSERT(FindType(type)!=NULL);

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(type, AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	/*root of Simulation object*/
	root = GetSimulationRoot(sim);

	/** Call on_load */
	error_reporter_tree_start();

	name = CreateIdName(AddSymbol("on_load"));
	pe = Initialize(root,name,"sim1",ASCERR,0, NULL, NULL);
	int haserror=0;
	if(error_reporter_tree_has_error()){
		haserror=1;
	}

	error_reporter_tree_end();

	if(pe == Proc_all_ok){
		if(haserror){
			ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Method 'on_load' returned all_ok status but has error(s).");
		}else{
			ERROR_REPORTER_NOLINE(ASC_USER_SUCCESS,"Method 'on_load' returned 'all_ok' and output no errors.\n");
		}
		CONSOLE_DEBUG("Method 'on_load' : COMPLETED OK");
	}else{
		ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Method 'on_load' returned the following error: %d",pe);
	}

	/* check for vars and rels */

	INITDTD(data, outfile, varfile, FirstDer, SecondDer,use_yacas, first_yacas, second_yacas, root, CASEFILE);
	if(data.outfile!=NULL){
		fprintf(data.outfile,"<html><head><title>LOG File of Test-Suite for Automatic Differentiation</title></head><body style='font-size: 9pt;font-family: serif;'>\n");
		fprintf(data.outfile,"<center><h3>LOG File of Test-Suite for Automatic Differentiation</h3></center></br></br>\n");
		fprintf(data.outfile,"<center><h4><u>Legend</u></h4><b>RAD_TOL		=		%21.17g</br>\n",RAD_TOL);
		fprintf(data.outfile,"Difference(a,b)		=		a - b</br>\n");
		fprintf(data.outfile,"Error(a,b)		=		(a - b)/a </br>\n");
		fprintf(data.outfile,"<font color='gold'>gold</font>		=	Residual Error </br>\n");
		fprintf(data.outfile,"<font color='indigo'>indigo</font>=		Gradient Error </br>\n");
		fprintf(data.outfile,"<font color='purple'>purple</font>=		Gradient Mismatch </br>\n");
		fprintf(data.outfile,"<font color='red'>red</font>=		Second Partials Mismatch</b> </center></br></br>\n");
	}

	VisitInstanceTreeTwo(root, AutomateDiffTest, 0, 0, &data);
	sim_destroy(sim);
	Asc_CompilerDestroy();

	if(data.outfile!=NULL){
		fprintf(data.outfile,"</br><center><u><h4>SUMMARY:</h4></u></br>\n");
		fprintf(data.outfile,"<b>No. of Residual Errors</b> =<b> %d </b></br>\n",data.d0errors);
		fprintf(data.outfile,"<b>No. of First RAD Errors</b> =<b> %d </b></br>\n",data.d1errors);
		fprintf(data.outfile,"<b>No. of Total Errors</b> =<b> %d</b></br></br>\n",data.d0errors+data.d1errors);
		fprintf(data.outfile,"<b>No. of First YACAS Mismatches</b> =<b> %d</b></br>\n",data.d1errors_yacas);
		fprintf(data.outfile,"<b>No. of Second YACAS Mismatches</b> =<b> %d</b></br>\n",data.d2errors_yacas);
		fprintf(data.outfile,"<b>No. of Total Mismatches</b> =<b> %d</b></center></br>\n</body>\n</html>"
			,data.d1errors_yacas + data.d2errors_yacas
		);
	}

#define OSPCLEAN(OSPNAME,FNAME) if(OSPNAME!=NULL){ospath_free(OSPNAME);if(FNAME!=NULL){fclose(FNAME);data.FNAME = NULL;}}

	OSPCLEAN(out_osp,outfile);

	if (use_yacas){
		OSPCLEAN(varfile_osp,varfile);
		OSPCLEAN(yacas_osp_2nd,SecondDer.yacas);
		OSPCLEAN(safe_osp_2nd,SecondDer.safeder);
		OSPCLEAN(nonsafe_osp_2nd,SecondDer.nonsafeder);
		OSPCLEAN(yacas_osp_1st,FirstDer.yacas);
		OSPCLEAN(safe_osp_1st,FirstDer.safeder);
		OSPCLEAN(nonsafe_osp_1st,FirstDer.nonsafeder);
	}else{
		OSPCLEAN(first_yacas_osp,first_yacas);
		OSPCLEAN(second_yacas_osp,second_yacas);
	}

	CU_ASSERT( 0 == (data.d0errors + data.d1errors) );
	CONSOLE_DEBUG("For non-fatal errors refer ascend/compiler/test/LOG.html");
}


/**
		Files into which information is extracted
		outfile - comment
		safeder - all safe 2nd derivative values
		nonsafeder - all non-safe 2nd derivative values
		outfile - End of a Relation
		yacas  - O/P to YACAS

*/
static void AutomateDiffTest(struct Instance *inst, VOIDPTR ptr){
#define RETURN if (rname != NULL) { ASC_FREE(rname); } return
	double residual_rev,residual_fwd;
	double *gradients_rev,*gradients_fwd,*deriv_2nd;

	float yacas_first_der = 0.0;  /* compiler complains type mismatch when using */
	float yacas_second_der = 0.0; /*double to read using fscanf(File*,"%21.17g"...) */


	double err;


	unsigned long num_var;
	unsigned long i,j;

	enum Expr_enum reltype;

	struct relation *r;

	int32 status;

	char *rname = NULL;
	char *infix_rel = NULL;
	char *varname;
	char buf[20];

    struct RXNameData myrd = {"x",NULL,""};
	struct DiffTestData *data = (struct DiffTestData*) ptr;
	struct Instance *var_inst;

	if (inst==NULL || InstanceKind(inst)!=REL_INST){
		return;
	}
	if (data != NULL && data->outfile != NULL) {
		rname = WriteInstanceNameString(inst, data->root);
	}

	CONSOLE_DEBUG("<<<<<<<<<<<<<<<<< The relation that follows: %s >>>>>>>>>>>>>>>>>>>>",rname);

	r = (struct relation *)GetInstanceRelation(inst, &reltype);

	if (r == NULL) {
		LOG(data, "<font color='orange'><b>! skipping instance with null struct relation:</b></font> %s</br>\n", rname);
		RETURN;
	}

	if (reltype != e_token) {
		LOG(data, "<font color='orange'><b>! skipping non-token relation</b></font> %s</br>\n", rname);
		RETURN;
	}

	LOG(data, "</br><font color='green'><b> Evaluating token relation </b></font> <b> %s </b> </br>\n", rname);

	infix_rel = WriteRelationString(inst,
									data->root,
									(WRSNameFunc)RelationVarXName,
									NULL,
									relio_yacas,
									NULL);

	LOG(data,"</br></br><b>Relation:  %s  </br>\n",infix_rel); // Do I need to escape this??

	num_var = NumberVariables(r); // FIXME or should we use rel_n_incidences

	for(i=0; i<num_var; i++) {
		varname = RelationVarXName(r,i+1,&myrd);
		if (varname!=NULL){
			LOG(data,"%s:=",varname);
			var_inst = RelationVariable(r,i+1);
			LOG(data,"%21.17g</br>\n",RealAtomValue(var_inst));
		}
	}

	LOG(data,"</b>\n");

	gradients_rev = ASC_NEW_ARRAY(double,num_var); // or rel_n_incidences
	CU_ASSERT_PTR_NOT_NULL_FATAL(gradients_rev);

	gradients_fwd = ASC_NEW_ARRAY(double,num_var); // or rel_n_incidences
	CU_ASSERT_PTR_NOT_NULL_FATAL(gradients_fwd);

	deriv_2nd = ASC_NEW_ARRAY(double,num_var); // or rel_n_incidences
	CU_ASSERT_PTR_NOT_NULL_FATAL(deriv_2nd);

	/** @todo log the infix form of the relation and associated variables values */ /*FIXME*/
	if(data->use_yacas){
		if(data->FirstDer.yacas!=NULL && data->SecondDer.yacas!=NULL && data->varfile!=NULL){
			/** Print Variables Values First */
			for(i=0; i<num_var; i++) {
				varname = RelationVarXName(r,i+1,&myrd);
				if (varname!=NULL){
					fprintf(data->varfile,"%s:=",varname);
					var_inst = RelationVariable(r,i+1);
					fprintf(data->varfile,"%21.17g\n",RealAtomValue(var_inst));
				}
			}
			fprintf(data->varfile,"@ Relation: %s follows\n",rname);

			if (infix_rel!=NULL){
				for(i=0; i<num_var;i++){
					varname = RelationVarXName(r,i+1,&myrd);
					strncpy(buf,varname,20);

					// Generating Output for First Derivative Yacas Input file
					fprintf(data->FirstDer.yacas,"ToStdout() [Echo({D(%s) ",buf);
					fprintf(data->FirstDer.yacas,"%s});];\n",infix_rel);


					// Generating Output for Second Derivative Yacas Input file
					for (j=0;j<num_var;j++){
						varname = RelationVarXName(r,j+1,&myrd);
						fprintf(data->SecondDer.yacas,"ToStdout() [Echo({D(%s) ",buf);
						fprintf(data->SecondDer.yacas,"D(%s) ",varname);
						fprintf(data->SecondDer.yacas,"%s});];\n",infix_rel);
					}
				}
			}
			else{
				ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Infix Relation is NULL");
			}
		}
	}
	/** Testing non-safe routines */

	ASC_FREE(infix_rel);

	/* we need to sigfpe trap this code or use the safe versions. */
	RelationCalcResidGradRev(inst,&residual_rev,gradients_rev);
	RelationCalcResidGrad(inst,&residual_fwd,gradients_fwd);

	LOG(data,"</br> <b> Table of Values for Residuals </b> </br>\n");
	LOG(data,"\n<table BORDER>\n");
	LOG(data,"<tr><td>ASCEND(NONSAFE,REV)</td><td>ASCEND(NONSAFE,FWD)</td><td>Error</td></tr>\n");
	CU_ASSERT(fabs(residual_rev - residual_fwd) <= RAD_TOL);
	if ( fabs(residual_rev - residual_fwd) > RAD_TOL) {
		data->d0errors ++;
		LOG(data,"<tr bgcolor='yellow'><td><font color='red'>%21.17g</font></td><td><font color='red'>%21.17g</font></td><td><font color='red'>%.4g</font></td></tr>\n", residual_rev,residual_fwd, fabs(residual_rev - residual_fwd));
	}
	else{
		LOG(data,"<tr><td>%21.17g</td><td>%21.17g</td><td>%.4g</td></tr>\n",residual_rev,residual_fwd,0.0);
	}
	LOG(data,"</table>\n");



	LOG(data,"</br> <b> Table of Values for First Derivatives </b> </br>\n");
	LOG(data,"\n<table BORDER>\n");
	for(i=0; i<num_var; i++) {

		if(data->use_yacas && data->FirstDer.nonsafeder!=NULL){
			/* Recording Reverse AD Non-Safe Derivatives to file */
			fprintf(data->FirstDer.nonsafeder,"%21.17g\n",gradients_rev[i]);
		}
		else if(data->first_yacas!=NULL){
			/* Benchmarking Non-Safe Gradient Errors against Yacas */
			if(!feof(data->first_yacas)){
				fscanf(data->first_yacas,"%g\n",&yacas_first_der);

				if(yacas_first_der!=0.0){
					err = fabs((double)yacas_first_der - gradients_rev[i]) / (double)yacas_first_der;
				}else{
					err = gradients_rev[i];
				}
				err = fabs(err);
				LOG(data,"<tr><td>Column</td><td>ASCEND(NONSAFE,REV)</td><td>YACAS</td><td>Percentage Mismatch</td></tr>\n");
				//CU_ASSERT(err <= RAD_TOL);
				if (err > RAD_TOL) {
					CU_FAIL("error exceeded tolerance");
					data->d1errors_yacas ++;
					LOG(data,"<tr bgcolor='yellow'><td><font color='red'>%lu</font></td><td><font color='red'>%21.17g</font></td><td><font color='red'>%21.17g</font></td><td><font color='red'>%.4g</font></td></tr>\n", i,gradients_rev[i],yacas_first_der, err*100);
				}
				else{
					LOG(data,"<tr><td>%lu</td><td>%21.17g</td><td>%21.17g</td><td>%.4g</td></tr>\n", i,gradients_rev[i],yacas_first_der,0.0);
				}
			}
		}

		if (gradients_fwd[i] != 0.0) {
			err = (gradients_fwd[i] - gradients_rev[i]) / gradients_fwd[i];
		} else {
			err = gradients_rev[i];  // These are totally different quantities should I make err as 1?
		}
		err = fabs(err);
		LOG(data,"<tr><td>Column</td><td>ASCEND(NONSAFE,REV)</td><td>ASCEND(NONSAFE,FWD)</td><td>Percentage Mismatch</td></tr>\n");
		//CU_ASSERT(err <= RAD_TOL);
		if (err > RAD_TOL) {
			CONSOLE_DEBUG("Failed tolerance in first deriv #%d",i);
			CU_FAIL("Error exceeded tolerance");
			data->d1errors ++;
			LOG(data,"<tr bgcolor='yellow'><td><font color='red'>%lu</font></td><td><font color='red'>%21.17g</font></td><td><font color='red'>%21.17g</font></td><td><font color='red'>%.4g</font></td></tr>\n", i,gradients_rev[i],gradients_fwd[i], err*100);
		}
		else {
			LOG(data,"<tr><td>%lu</td><td>%21.17g</td><td>%21.17g</td><td>%21.17g</td></tr>\n", i,gradients_rev[i],gradients_fwd[i],0.0);
		}
	}
	LOG(data,"</table>\n");

	/*Non Safe Second Derivative Calculations*/
	if(data->use_yacas && data->SecondDer.nonsafeder!=NULL){
		fprintf(data->SecondDer.nonsafeder,"@ Relation: %s Follows\n",rname);
	}



	LOG(data,"</br> <b> Table of Values for Second Derivatives </b> </br>\n");
	LOG(data,"\n<table BORDER>\n");
	LOG(data,"<tr><td>Row</td><td>Column</td><td>ASCEND (NON-SAFE)</td><td>Yacas</td><td>Percentage Mismatch</td></tr>\n");
	for(i=0; i<num_var; i++){
		RelationCalcSecondDeriv(inst,deriv_2nd,i);
		if(data->use_yacas && data->SecondDer.nonsafeder!=NULL){
			/** @todo log calculated values and indiceshere.*/ /*FIXME*/
			for(j=0; j<num_var; j++){
				fprintf(data->SecondDer.nonsafeder,"%21.17g\n",deriv_2nd[j]); /*FIXME*/
			}
		}
		else if (data->second_yacas!=NULL){
			/* Benchmarking Non-Safe Second Derivative Errors against Yacas */
			for(j=0; j<num_var; j++){
				if(!feof(data->second_yacas)){
					fscanf(data->second_yacas,"%g\n",&yacas_second_der);

					if(yacas_second_der!=0.0){
						err = ((double)yacas_second_der - deriv_2nd[j]) / (double)yacas_second_der; /* todo err scaling */
					}else{
						err = deriv_2nd[j];
					}
					err = fabs(err);
					//CU_ASSERT(err <= RAD_TOL);
					if(err > RAD_TOL) {
						CU_FAIL("error exceeded tolerance");
						data->d2errors_yacas ++;
						LOG(data,"<tr bgcolor='yellow'><td><font color='red'>%lu</font></td><td><font color='red'>%lu</font></td><td><font color='red'>%21.17g</font></td><td><font color='red'>%21.17g</font></td><td><font color='red'>%.4g</font></td></tr>\n", i,j,deriv_2nd[j],yacas_second_der,err*100);
					}
					else{
						LOG(data,"<tr><td>%lu</td><td>%lu</td><td>%21.17g</td><td>%21.17g</td><td>%21.17g</td></tr>\n", i,j,deriv_2nd[j],yacas_second_der,0.0);
					}
				}
			}
		}
	}

	LOG(data,"</table>\n");

	/** Testing safe routines */

	status = (int32) RelationCalcResidGradRevSafe(inst,&residual_rev,gradients_rev);
	safe_error_to_stderr( (enum safe_err *)&status );

	status = RelationCalcResidGradSafe(inst,&residual_fwd,gradients_fwd);
	safe_error_to_stderr( (enum safe_err *)&status );



	LOG(data,"</br> <b> Table of Values for Residuals </b> </br>\n");
	LOG(data,"\n<table BORDER>\n");
	LOG(data,"<tr><td>ASCEND(SAFE,REV)</td><td>ASCEND(SAFE,FWD)</td><td>Error</td></tr>\n");
	CU_ASSERT(fabs(residual_rev - residual_fwd) <= RAD_TOL);
	if ( fabs(residual_rev - residual_fwd) > RAD_TOL) {
		data->d0errors ++;
		LOG(data,"<tr bgcolor='yellow'><td><font color='red'>%21.17g</font></td><td><font color='red'>%21.17g</font></td><td><font color='red'>%.4g</font></td></tr>\n", residual_rev,residual_fwd, fabs(residual_rev - residual_fwd));
	}
	else{
		LOG(data,"<tr><td>%21.17g</td><td>%21.17g</td><td>%.4g</td></tr>\n", residual_rev,residual_fwd,0.0);
	}
	LOG(data,"</table>\n");



	LOG(data,"</br> <b> Table of Values for First Derivatives </b> </br>\n");
	LOG(data,"\n<table BORDER>\n");
	LOG(data,"<tr><td>Column</td><td>ASCEND(SAFE,REV)</td><td>ASCEND(SAFE,FWD)</td><td>Percentage Mismatch</td></tr>\n");
	for(i=0;i<num_var;i++) {

		if(data->use_yacas && data->FirstDer.safeder!=NULL){
			/* Recording Reverse AD Safe Derivatives to file */
			fprintf(data->FirstDer.safeder,"%21.17g\n",gradients_rev[i]);
		}


		if (gradients_fwd[i] != 0.0) {
			err = ( gradients_fwd[i] - gradients_rev[i] ) / gradients_fwd[i];
		} else {
			err = gradients_rev[i];
		}
		err = fabs(err);
		CU_ASSERT(err <= RAD_TOL);
		if (err > RAD_TOL) {
			data->d1errors ++;
			LOG(data,"<tr bgcolor='yellow'><td><font color='red'>%lu</font></td><td><font color='red'>%21.17g</font></td><td><font color='red'>%21.17g</font></td><td><font color='red'>%.4g</font></td></tr>\n", i,gradients_rev[i],gradients_fwd[i], err*100);
		}
		else{
			LOG(data,"<tr><td>%lu</td><td>%21.17g</td><td>%21.17g</td><td>%21.17g</td></tr>\n", i,gradients_rev[i],gradients_fwd[i],0.0);
		}
	}
	LOG(data,"</table>\n");


	/*Safe Second Derivative Calculations*/
	if(data->use_yacas && data->SecondDer.safeder!=NULL){
		fprintf(data->SecondDer.safeder,"@ Relation: %s Follows\n",rname);
	}
	for(i=0; i<num_var; i++){
		status = (int32) RelationCalcSecondDerivSafe(inst,deriv_2nd,i);
		safe_error_to_stderr( (enum safe_err *)&status );

		if(data->use_yacas && data->SecondDer.safeder!=NULL){
			/** @todo log calculated values here.*/ /*FIXME*/
			for(j=0; j<num_var; j++){
				fprintf(data->SecondDer.safeder,"%21.17g\n",deriv_2nd[j]); /*FIXME*/
			}
		}
	}

	/** End of Relation */
	LOG(data,"</br><b>@ End of Relation </b>%p{%s}</br><hr></hr>",r,rname);


	/** Freeing used memeory */
	if(gradients_rev) {
		ASC_FREE(gradients_rev);
	}

	if(gradients_fwd) {
		ASC_FREE(gradients_fwd);
	}

	if(deriv_2nd) {
		ASC_FREE(deriv_2nd);
	}


	RETURN;

#undef RETURN

}
#undef OUTENV
#undef VARFILE
#undef CASEFILE

#undef SAFEDER_2ND
#undef NONSAFEDER_2ND
#undef YACAS_2ND

#undef SAFEDER_1ST
#undef NONSAFEDER_1ST
#undef YACAS_1ST

#undef YACAS_IN_1ST
#undef YACAS_IN_2ND


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
  T(autodiff)

REGISTER_TESTS_SIMPLE(compiler_autodiff, TESTS)

