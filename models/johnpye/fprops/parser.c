/*
 * parser.c
 * This file is part of ASCEND
 *
 * Copyright (C) 2011 - Carnegie Mellon University
 *
 * ASCEND is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ASCEND is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASCEND; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
 
 //Compile with gcc `xml2-config --cflags --libs` -DSTANDALONE -Wall parserMkII.c -o parserMkII

#include "parser.h"

#ifdef STANDALONE
int main(int argc, char* argv[]) {
	char *filename;
	IdealData i; HelmholtzData h;
	if (argc <= 1) {
		printf("Usage: %s docname\n", argv[0]);
		return(0);
	}
	else filename = argv[1];
	
	readFluidFile(filename,&i,&h);
	
	//File read successfully:
	int count;
	printf(
	"﻿<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	"<fluid name=\"%s\">\n"
	"\t<idealData>\n"
	"\t\t<molarMass value=\"%f\" />\n"
	"\t\t<specificGasConstant value=\"%f\" />\n"
	"\t\t<normalizationTemperature value=\"%f\" />\n"
	"\t\t<constantTerm value=\"%f\" />\n"
	"\t\t<linearTerm value=\"%f\" />\n"
	"\t\t<idealPowerTerms>\n",h.name,h.M,h.R,h.T_star,i.c,i.m);
	for(count=0; count<i.np; count++) {
		printf("\t\t\t<idealPowerTerm c=\"%f\" t=\"%f\" />\n",i.pt[count].c,i.pt[count].t);
	}
	printf("\t\t</idealPowerTerms>\n"
	"\t\t<exponentialTerms>\n");
	for(count=0; count<i.ne; count++) {
		printf("\t\t\t<exponentialTerm b=\"%f\" beta=\"%f\" />\n",i.et[count].b,i.et[count].beta);
	}
	printf("\t\t</exponentialTerms>\n"
	"\t</idealData>\n"
	"\t<helmholtzData>\n"
	"\t\t<rhoStar value=\"%f\" />\n"
	"\t\t<rhoC value=\"%f\" />\n"
	"\t\t<triplePointTemp value=\"%f\" />\n"
	"\t\t<acentricFactor value=\"%f\" />\n"
    "\t\t<helmholtzPowerTerms>\n",h.rho_star, h.rho_c, h.T_t, h.omega);
    for(count=0; count<h.np; count++) {
		printf("\t\t\t<powerTerm a=\"%f\" t=\"%f\" d=\"%d\" l=\"%d\" />\n",h.pt[count].a,h.pt[count].t,h.pt[count].d,h.pt[count].l);
	}
	printf("\t\t</helmholtzPowerTerms>\n"
	"\t\t<criticalTermsFirstKind>\n");
	for(count=0; count<h.ng; count++) {
		printf("\t\t\t<criticalTerm n=\"%f\" t=\"%f\" d=\"%f\" alpha=\"%f\" beta=\"%f\" gamma=\"%f\" epsilon=\"%f\" />\n",
		h.gt[count].n,h.gt[count].t,h.gt[count].d,h.gt[count].alpha, h.gt[count].beta, h.gt[count].gamma, h.gt[count].epsilon);
	}
	printf("\t\t</criticalTermsFirstKind>\n"
	"\t\t<criticalTermsSecondKind>\n");
	for(count=0; count<h.nc; count++) {
		printf("\t\t\t<criticalTerm n=\"%f\" a=\"%f\" b=\"%f\" beta=\"%f\" A=\"%f\" B=\"%f\" C=\"%f\" D=\"%f\" />\n",
		h.ct[count].n,h.ct[count].a,h.ct[count].b,h.ct[count].beta,h.ct[count].A,h.ct[count].B,h.ct[count].C,h.ct[count].D);
	}
	printf("\t\t</criticalTermsSecondKind>\n"
	"\t</helmholtzData>\n"
	"</fluid>\n");
	
	return 0;
}
#endif

int readFluidFile(char* filename, IdealData *i, HelmholtzData *h) {
	//Load the RelaxNG Schema:
	xmlRelaxNGPtr relaxngschemas = NULL;
	xmlRelaxNGParserCtxtPtr ctxt;
	xmlDocPtr doc;
	
	ctxt = xmlRelaxNGNewParserCtxt("fluidSchema.rng");
	
	xmlRelaxNGSetParserErrors(ctxt,
		(xmlRelaxNGValidityErrorFunc)fprintf,
		(xmlRelaxNGValidityWarningFunc)fprintf,
		stderr);
	
	relaxngschemas = xmlRelaxNGParse(ctxt);
	xmlRelaxNGFreeParserCtxt(ctxt);
	
	//Load the xml document to be parsed:
	doc = xmlParseFile(filename);
	
	//Validate:
	xmlRelaxNGValidCtxtPtr validCtxt;
	validCtxt = xmlRelaxNGNewValidCtxt(relaxngschemas);
	xmlRelaxNGSetValidErrors(validCtxt,
		(xmlRelaxNGValidityErrorFunc) fprintf,
		(xmlRelaxNGValidityWarningFunc) fprintf,
		stderr);
	xmlRelaxNGValidateDoc(validCtxt, doc);
	
	//Read data:
	xmlTextReaderPtr reader = xmlReaderWalker(doc);
	int ret = xmlTextReaderRead(reader);
    while (ret == 1) {
    	getData(reader,i,h);
    	ret=xmlTextReaderRead(reader);
    }
    xmlFreeTextReader(reader);

	return 0;
}

int getCubicData(char* filename, CubicData *c) {
    return 0;
}

int getData(xmlTextReaderPtr reader, IdealData *i, HelmholtzData *h) {
	xmlChar *elementName;
	xmlNodePtr node; int term;
	elementName=xmlTextReaderName(reader);
	if(elementName!=NULL && elementName[0]!='#') {
		if(!xmlStrcmp(elementName,xmlCharStrdup("fluid"))) {
			h->name=malloc((strlen((char*)xmlTextReaderGetAttribute(reader,xmlCharStrdup("name")))+1)*sizeof(char));
			strcpy(h->name,(char*)xmlTextReaderGetAttribute(reader,xmlCharStrdup("name")));
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("molarMass"))) {
			h->M=atof((const char*)xmlTextReaderGetAttribute(reader,xmlCharStrdup("value")));
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("specificGasConstant"))) {
			h->R=atof((const char*)xmlTextReaderGetAttribute(reader,xmlCharStrdup("value")));
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("normalizationTemperature"))) {
			h->T_star=atof((const char*)xmlTextReaderGetAttribute(reader,xmlCharStrdup("value")));
			h->T_c=h->T_star; //FIXME: Is this always true!?
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("constantTerm"))) {
			i->c=atof((const char*)xmlTextReaderGetAttribute(reader,xmlCharStrdup("value")));
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("linearTerm"))) {
			i->m=atof((const char*)xmlTextReaderGetAttribute(reader,xmlCharStrdup("value")));
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("rhoStar"))) {
			h->rho_star=atof((const char*)xmlTextReaderGetAttribute(reader,xmlCharStrdup("value")));
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("rhoC"))) {
			h->rho_c=atof((const char*)xmlTextReaderGetAttribute(reader,xmlCharStrdup("value")));
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("triplePointTemp"))) {
			h->T_t=atof((const char*)xmlTextReaderGetAttribute(reader,xmlCharStrdup("value")));
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("acentricFactor"))) {
			h->omega=atof((const char*)xmlTextReaderGetAttribute(reader,xmlCharStrdup("value")));
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("idealPowerTerms"))) {
			node=xmlTextReaderExpand(reader);
			if(node!=NULL && node->xmlChildrenNode!=NULL) node=node->xmlChildrenNode;
			for(term=0; node->next!=NULL; node=node->next) {
				if(!xmlStrcmp(node->name, (const xmlChar *)"idealPowerTerm")) term++;
			}
			i->np=term;
			i->pt=malloc(i->np*sizeof(IdealPowTerm));
			node=xmlTextReaderExpand(reader);
			if(node!=NULL && node->xmlChildrenNode!=NULL) node=node->xmlChildrenNode;
			for(term=0; node->next!=NULL; node=node->next) {
				if(!xmlStrcmp(node->name, (const xmlChar *)"idealPowerTerm")) {
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("c") ),"%lf",&(i->pt[term].c));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("t") ),"%lf",&(i->pt[term].t));
					term++;
				}
			}
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("exponentialTerms"))) {
			node=xmlTextReaderExpand(reader);
			if(node!=NULL && node->xmlChildrenNode!=NULL) node=node->xmlChildrenNode;
			for(term=0; node->next!=NULL; node=node->next) {
				if(!xmlStrcmp(node->name, (const xmlChar *)"exponentialTerm")) term++;
			}
			i->ne=term;
			i->et=malloc(i->ne*sizeof(IdealExpTerm));
			node=xmlTextReaderExpand(reader);
			if(node!=NULL && node->xmlChildrenNode!=NULL) node=node->xmlChildrenNode;
			for(term=0; node->next!=NULL; node=node->next) {
				if(!xmlStrcmp(node->name, (const xmlChar *)"exponentialTerm")) {
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("b") ),"%lf",&(i->et[term].b));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("beta") ),"%lf",&(i->et[term].beta));
					term++;
				}
			}
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("helmholtzPowerTerms"))) {
			node=xmlTextReaderExpand(reader);
			if(node!=NULL && node->xmlChildrenNode!=NULL) node=node->xmlChildrenNode;
			for(term=0; node->next!=NULL; node=node->next) {
				if(!xmlStrcmp(node->name, (const xmlChar *)"powerTerm")) term++;
			}
			h->np=term;
			h->pt=malloc(h->np*sizeof(HelmholtzPowTerm));
			node=xmlTextReaderExpand(reader);
			if(node!=NULL && node->xmlChildrenNode!=NULL) node=node->xmlChildrenNode;
			for(term=0; node->next!=NULL; node=node->next) {
				if(!xmlStrcmp(node->name, (const xmlChar *)"powerTerm")) {
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("a") ),"%lf",&(h->pt[term].a));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("t") ),"%lf",&(h->pt[term].t));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("d") ),"%d",&(h->pt[term].d));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("l") ),"%d",&(h->pt[term].l));
					term++;
				}
			}
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("criticalTermsFirstKind"))) {
			node=xmlTextReaderExpand(reader);
			if(node!=NULL && node->xmlChildrenNode!=NULL) node=node->xmlChildrenNode;
			for(term=0; node->next!=NULL; node=node->next) {
				if(!xmlStrcmp(node->name, (const xmlChar *)"criticalTerm")) term++;
			}
			h->ng=term;
			h->gt=malloc(h->ng*sizeof(HelmholtzGausTerm));
			node=xmlTextReaderExpand(reader);
			if(node!=NULL && node->xmlChildrenNode!=NULL) node=node->xmlChildrenNode;
			for(term=0; node->next!=NULL; node=node->next) {
				if(!xmlStrcmp(node->name, (const xmlChar *)"criticalTerm")) {
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("n") ),"%lf",&(h->gt[term].n));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("t") ),"%lf",&(h->gt[term].t));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("d") ),"%lf",&(h->gt[term].d));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("alpha") ),"%lf",&(h->gt[term].alpha));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("beta") ),"%lf",&(h->gt[term].beta));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("gamma") ),"%lf",&(h->gt[term].gamma));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("epsilon") ),"%lf",&(h->gt[term].epsilon));
					term++;
				}
			}
		}
		else if(!xmlStrcmp(elementName,xmlCharStrdup("criticalTermsSecondKind"))) {
			node=xmlTextReaderExpand(reader);
			if(node!=NULL && node->xmlChildrenNode!=NULL) node=node->xmlChildrenNode;
			for(term=0; node->next!=NULL; node=node->next) {
				if(!xmlStrcmp(node->name, (const xmlChar *)"criticalTerm")) term++;
			}
			h->nc=term;
			h->ct=malloc(h->nc*sizeof(HelmholtzGausTerm));
			node=xmlTextReaderExpand(reader);
			if(node!=NULL && node->xmlChildrenNode!=NULL) node=node->xmlChildrenNode;
			for(term=0; node->next!=NULL; node=node->next) {
				if(!xmlStrcmp(node->name, (const xmlChar *)"criticalTerm")) {
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("n") ),"%lf",&(h->ct[term].n));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("a") ),"%lf",&(h->ct[term].a));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("b") ),"%lf",&(h->ct[term].b));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("beta") ),"%lf",&(h->ct[term].beta));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("A") ),"%lf",&(h->ct[term].A));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("B") ),"%lf",&(h->ct[term].B));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("B") ),"%lf",&(h->ct[term].C));
					sscanf((const char*)xmlGetProp(node, xmlCharStrdup("B") ),"%lf",&(h->ct[term].D));
					term++;
				}
			}
		}
	}
	return 0;
}
