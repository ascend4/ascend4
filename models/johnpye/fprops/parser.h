/*
 * parser.h
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

#ifndef PARSER_H
#define PARSER_H

#ifdef STANDALONE
#include <stdio.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/relaxng.h>
#include <libxml/xmlreader.h>
#include <string.h>

#include "dataStructures.h"

int readFluidFile(char* filename, IdealData *iDat, HelmholtzData *hDat);
int getCubicData(char* filename, CubicData *c);
int getData(xmlTextReaderPtr reader, IdealData *i, HelmholtzData *h);


#endif
