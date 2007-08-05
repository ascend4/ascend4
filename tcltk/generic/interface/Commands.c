/*
 *  Commands.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.62 $
 *  Version control file: $RCSfile: Commands.c,v $
 *  Date last modified: $Date: 1998/04/25 13:10:16 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

#define KILLBDAG 0
#define REIMPLEMENT 0

#include <tcl.h>
#include <tk.h>

#include <utilities/config.h>
#include <utilities/ascConfig.h>
#include <general/list.h>

#include <compiler/instance_enum.h>
#include <compiler/instance_name.h>
#include <compiler/units.h>

#include <linear/mtx.h>

#include <system/slv_client.h>

#include "HelpProc.h"
#include "Commands.h"
#include "LibraryProc.h"
#include "DisplayProc.h"

#include "Qlfdid.h"
#ifndef ASCTK_QLFDID_H
# error "ASCTK_QLFDID_H not defined in Commands.c???"
#endif

#include "SimsProc.h"
#include "BrowserProc.h"
#include "BrowserMethod.h"
#include "BrowserQuery.h"
#if KILLBDAG
# include "interface/BrowserDag.h"
#endif
#include "BrowserRel_io.h"
#include "BrowLogRel_io.h"
#include "BrowWhen_io.h"
#include "ProbeProc.h"
#include "UserData.h"
#include "SolverProc.h"
#include "DebugProc.h"
#include "MtxProc.h"
#include "SlvProc.h"
#include "EnvVarProc.h"
#if REIMPLEMENT
# include "interface/CodeGen.h"
#endif
#include "UnitsProc.h"
#include "ScriptProc.h"
#include "Integrators.h"
#include "Sensitivity.h"		/* only for Asc_MtxNormsCmd */
#include "typelex.h"
#include "Driver.h"

void Asc_AddCommand(Tcl_Interp *interp, char *cmdName, Tcl_CmdProc *proc,
                    ClientData cdata, Tcl_CmdDeleteProc *deleteProc,
                    CONST char *group, CONST char *usage,
                    CONST char *shorth, HLFunc longh)
{
  Tcl_CreateCommand(interp,cmdName,proc,cdata,deleteProc);
  Asc_HelpDefine(cmdName,group,usage,shorth,longh);
}

/*
 * Any new commands that need to be registered should be added here.
 */
void Asc_CreateCommands(Tcl_Interp *interp)
{
  /*Tcl_Interp *interp = g_interp;  */
  /* create a local pointer to the global Tcl Interpreter (from GUIinit) */

  Asc_HelpDefineGroup("system",
    "Explanation: functions that do various system call type activities.");

  /* All commands should eventually look like this one */
  ASCADDCOM(interp,Asc_HelpCmdHN,Asc_HelpCmd,
   "system", Asc_HelpCmdHU, Asc_HelpCmdHS, Asc_HelpCmdHLF);

  ASCADDCOM(interp,Asc_EnvVarCmdHN,Asc_EnvVarCmd,
   "system", Asc_EnvVarCmdHU, Asc_EnvVarCmdHS, Asc_EnvVarCmdHLF);

  ASCADDCOM(interp,"asc_clock", Asc_TimeCmd,
    "system",
    "asc_clock <string> [n]",
    "returns clock info from evaluating 'string' n times in Tcl global scope",
    Asc_TimeCmdHLF
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"script_eval", Asc_ScriptEvalCmd,
    "system",
    "script_eval <string>",
    "Evaluates 'string' in Tcl global scope",
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  ASCADDCOM(interp,"fastraise", Asc_FastRaiseCmd,
    "system",
    "fastraise windowname",
    "calls XRaiseWindow on the toplevel given, rather than going through tk."
    "\nThis function exists because tk/X can be very very slow. It's not \
     highly portable and may be lost in a tk8 port.",
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  Asc_HelpDefineGroup("miscellaneous",
    "Explanation: functions that do unclassifiable things.");

  ASCADDCOM(interp,"stringcompact", Asc_StringCompact,
    "miscellaneous",
    "stringcompact <string>",
    "returns a string constructed from the input by removing extra whitespace"
    "\nThis function reduces all extra tabs, newlines and blanks to a space, \
     and ascplot depends on it heavily to insure sanity",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  Asc_HelpDefineGroup("library",
    "Explanation: functions that operate on the ascend type library");

  ASCADDCOM(interp, Asc_LibrQueryTypeCmdHN, Asc_LibrQueryTypeCmd,
    "library", Asc_LibrQueryTypeCmdHU, Asc_LibrQueryTypeCmdHS,
    Asc_LibrQueryTypeCmdHLF);

  ASCADDCOM(interp, Asc_LibrOptionsCmdHN, Asc_LibrOptionsCmd,
    "library", Asc_LibrOptionsCmdHU, Asc_LibrOptionsCmdHS,
    Asc_LibrOptionsCmdHLF);

  ASCADDCOM(interp, Asc_LibrParseCmdHN, Asc_LibrParseCmd,
    "library", Asc_LibrParseCmdHU, Asc_LibrParseCmdHS,
    Asc_LibrParseCmdHLF);

  ASCADDCOM(interp, Asc_LibrReadCmdHN, Asc_LibrReadCmd,
    "library", Asc_LibrReadCmdHU, Asc_LibrReadCmdHS,
    Asc_LibrReadCmdHLF);

  ASCADDCOM(interp, Asc_LibrModuleInfoCmdHN, Asc_LibrModuleInfoCmd,
    "library", Asc_LibrModuleInfoCmdHU, Asc_LibrModuleInfoCmdHS,
    Asc_LibrModuleInfoCmdHLF);

  ASCADDCOM(interp, Asc_LibrDestroyTypesCmdHN, Asc_LibrDestroyTypesCmd,
    "library",  Asc_LibrDestroyTypesCmdHU,  Asc_LibrDestroyTypesCmdHS,
     Asc_LibrDestroyTypesCmdHLF);

  ASCADDCOM(interp, Asc_LibrHideTypeCmdHN, Asc_LibrHideTypeCmd,
    "library", Asc_LibrHideTypeCmdHU, Asc_LibrHideTypeCmdHS,
    Asc_LibrHideTypeCmdHLF);

  ASCADDCOM(interp, Asc_LibrUnHideTypeCmdHN, Asc_LibrUnHideTypeCmd,
    "library", Asc_LibrUnHideTypeCmdHU, Asc_LibrUnHideTypeCmdHS,
    Asc_LibrUnHideTypeCmdHLF);

  ASCADDCOM(interp, Asc_LibrTypeIsShownCmdHN, Asc_LibrTypeIsShownCmd,
    "library", Asc_LibrTypeIsShownCmdHU, Asc_LibrTypeIsShownCmdHS,
    Asc_LibrTypeIsShownCmdHLF);

  ASCADDCOM(interp, Asc_LibrTypeListCmdHN, Asc_LibrTypeListCmd,
    "library", Asc_LibrTypeListCmdHU, Asc_LibrTypeListCmdHS,
    Asc_LibrTypeListCmdHLF);

  ASCADDCOM(interp, Asc_ExtractTypeHN, Asc_ExtractType,
    "library", Asc_ExtractTypeHU,  Asc_ExtractTypeHS,  Asc_ExtractTypeHLF);


#ifndef ASCTK_QLFDID_H
# error "ASCTK_QLFDID_H not yet defined???"
#endif

  /* Browser Routines */
  ASCADDCOM(interp,"qlfdid",Asc_BrowQlfdidSearchCmd,
    "miscellaneous",
    NULL,
    "qlfdid     -- searches for the inst of a qualified name",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  Asc_HelpDefineGroup("fileio",
    "Explanation: functions that read/write files");
  ASCADDCOM(interp,"bwritevalues", Asc_BrowWriteValues,
    "fileio",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__brow_find_type", Asc_BrowFindTypeCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__brow_reln_relop", Asc_BrowRelationRelopCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  /*   Future work
  ASCADDCOM(interp,"__brow_lrel_relop", BrowLogRelRelopCmd,
                    (ClientData) NULL,(Tcl_CmdDeleteProc *) NULL);  */

  ASCADDCOM(interp,"btransfer", Asc_BrowTransferCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"rootinit", Asc_BrowRootInitCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"root", Asc_BrowRootCmd,
    "browser",
    NULL,
    "root       -- sets the root instance for queries",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"oldinst", Asc_BrowRootBackupCmd,
    "browser",
    NULL,
    "oldinst    -- backup to parent instance",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"rootn", Asc_BrowRootNCmd,
    "browser",
    NULL,
    "rootn      -- backup to ancestor instance",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slist", Asc_BrowSimListCmd,
    "simulations",
    NULL,
    "slist      -- prints the list of all simulations",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"simtype", Asc_BrowSimTypeCmd,
    "simulations",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"bstatistics",Asc_BrowInstStatCmd,
    "browser",
    NULL,
    "bstatistics - inst tree stats",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"ilist",Asc_BrowInstListCmd,
    "miscellaneous",
    NULL,
    "ilist      -- prints the list of all working instances",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"is_type_refined",Asc_BrowRefinesMeCmd,
    "miscellaneous",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  ASCADDCOM(interp,"bprint", Asc_BrowPrintCmd,
    "miscellaneous",
    NULL,
    "bprint     -- prints the contents of an instance",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"inst", Asc_BrowInstQueryCmd,
    "browser",
    NULL,
    "inst       -- general inst query routines",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"bgetproc", Asc_BrowWriteProcedure,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp, Asc_BrowInitializeCmdHN, Asc_BrowInitializeCmd,
    "browser", Asc_BrowInitializeCmdHU, Asc_BrowInitializeCmdHS,
    Asc_BrowInitializeCmdHLF
  );

  ASCADDCOM(interp,"brow_assign", Asc_BrowRunAssignmentCmd,
    "browser",
    NULL,
    "assigns a value to the current or search atomic instance",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"qassgn2", Asc_BrowRunAssignQlfdidCmd2,
    "miscellaneous",
    NULL,
    "qassgn     -- assigns a value to a qlfdid atomic instance",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"qassgn3", Asc_BrowRunAssignQlfdidCmd3,
    "miscellaneous",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  ASCADDCOM(interp,"__brow_iname", Asc_BrowWriteInstanceNameCmd,
    "browser",
    NULL,
    "iname      -- returns the name of the working instance",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__brow_isrelation", Asc_BrowIsRelationCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__brow_ismodel", Asc_BrowIsModelCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"bgetrels", Asc_BrowWriteRelListCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"bgetrelspf", Asc_BrowWriteRelListPostfixCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__brow_relsforatom", Asc_BrowWriteRelsForAtomCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  ASCADDCOM(interp,"__brow_islogrel", Asc_BrowIsLogRelCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"bgetlogrels", Asc_BrowWriteLogRelListCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"bgetlogrelspf", Asc_BrowWriteLogRelListPostfixCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__brow_logrelsforatom", Asc_BrowWriteLogRelsForAtomCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  ASCADDCOM(interp,"bgetcondrels", Asc_BrowWriteCondRelListCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"bgetcondlogrels", Asc_BrowWriteCondLogRelListCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  ASCADDCOM(interp,"__brow_iswhen", Asc_BrowIsWhenCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__brow_isinstanceinwhen",
                    Asc_BrowIsInstanceInWhenCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"bgetwhens", Asc_BrowWriteWhenListCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__brow_whensforinstance",
                    Asc_BrowWriteWhensForInstanceCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"bwritependings", Asc_BrowWritePendingsSTDOUT,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"bnumpendings", Asc_BrowShowPendings,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"count_names", Asc_BrowCountNamesCmd,
    "browser",
    NULL,
    "count_names-- counts things slowly. see output for details",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"aliases", Asc_BrowWriteAliasesCmd,
    "browser",
    NULL,
   "aliases    -- returns the aliases of an instance",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"isas", Asc_BrowWriteISAsCmd,
    "browser",
    NULL,
   "returns the constructions of an instance",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"cliques", Asc_BrowWriteCliqueCmd,
    "browser",
    NULL,
    "returns all the members in the same ARE_ALIKE clique",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,Asc_BrowWriteInstanceCmdHN, Asc_BrowWriteInstanceCmd,
    "browser",
    Asc_BrowWriteInstanceCmdHU,
    "returns formatted info for the Browser",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"bmerge", Asc_BrowInstanceMergeCmd,
    "browser",
    NULL,
    "merge current and search instances",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"brefine",Asc_BrowInstanceRefineCmd,
    "browser",
    NULL,
    "refine current or search inst to type given",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"bmakealike",Asc_BrowMakeAlikeCmd,
    "browser",
    NULL,
    "ARE_ALIKE current and search instances",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"b_isplottable", Asc_BrowIsPlotAllowedCmd,
    "browser",
    NULL,
    "return boolean value TRUE if instance is a plot",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"b_prepplotfile", Asc_BrowPreparePlotFileCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"free_all_vars",Asc_BrowClearVarsCmd,
    "browser",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  /* Simulation Routines */
  ASCADDCOM(interp, Asc_SimBinTokenSetOptionsHN, Asc_SimBinTokenSetOptions,
    "library", Asc_SimBinTokenSetOptionsHU, Asc_SimBinTokenSetOptionsHS,
    Asc_SimBinTokenSetOptionsHLF);

  ASCADDCOM(interp,"sims", Asc_SimsQueryCmd,
    "simulations",
    NULL,
    "return list of simulation names in the instance universe",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"sim_instantiate",Asc_SimsCreateInstanceCmd,
    "simulations",
    NULL,
    "create a simulation in the instance universe",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"sim_unique",Asc_SimsUniqueNameCmd,
    "simulations",
    NULL,
    "boolean check for potential simulation name not currently in use",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"sim_reinstantiate",Asc_SimsResumeInstantiateCmd,
    "simulations",
    NULL,
    "sim_reinstantiate - resume compiling a simulation specified",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__sims_copy",Asc_SimsCopyInstanceCmd,
    "broken",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__sims_proto",Asc_SimsProtoTypeInstanceCmd,
    "broken",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__sims_saveinst",Asc_SimsSaveInstanceCmd,
    "broken",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__sims_update",Asc_SimsUpdateInstanceCmd,
    "broken",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"sim_destroy",Asc_SimsDestroySimulationCmd,
    "simulations",
    NULL,
    "sim_destroy   -- destroy a simulation",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"simlistpending",Asc_SimListPending,
    "simulations",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  /* Display and Type query */
  ASCADDCOM(interp,"ddefine", Asc_DispDefineCmd,
    "library",
    NULL,
    "ddefine    -- list all the types, or details of a type",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"ddiffdefine", Asc_DispDiffDefineCmd,
    "library",
    NULL,
    "ddiffdefine -- list the details of a type differing from parent",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"disp", Asc_DispQueryCmd,
    "miscellaneous",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"hier", Asc_DispHierarchyCmd,
    "library",
    NULL,
    "hier       -- list the hierarchy of a given type",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"file_by_type", Asc_DispFileByTypeCmd,
    "fileio",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dchild", Asc_DispChildOneCmd,
    "miscellaneous",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"drefines_me", Asc_DispRefinesMeCmd,
    "library",
    NULL,
    "drefines_me - list the immediate refinements of a given type",
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  Asc_AddCommand(interp,"drefines_meall", Asc_DispRefinesMeCmd,
    (ClientData) 1, (Tcl_CmdDeleteProc *)NULL,
    "library",
    NULL,
    "drefines_meall - list all refinements of a given type",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"drefinement_tree", Asc_DispRefinesMeTreeCmd,
    "library",
    NULL,
    "drefinement_tree - list the refinement hierarchy based on type",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dgetparts", Asc_DispTypePartsCmd,
    "library",
    NULL,
    "dgetparts  -- list the types of parts IN a type.",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"disroot_type", Asc_DispIsRootTypeCmd,
    "library",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  /* Probe Routine */
  ASCADDCOM(interp,Asc_ProbeCmdHN, Asc_ProbeCmd,
    "probe-list",Asc_ProbeCmdHU,Asc_ProbeCmdHS,Asc_ProbeCmdHLF);

  /* slv query routines */
  ASCADDCOM(interp,"__var_analyze", Asc_VarAnalyzeCmd,
    "miscellaneous",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__rel_analyze", Asc_RelAnalyzeCmd,
    "miscellaneous",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  /* UserData Routines */
  ASCADDCOM(interp,"__userdata_init", Asc_UserDataInitializeCmd,
    "user-list",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__userdata_create", Asc_UserDataCreateCmd,
    "user-list",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__userdata_destroy", Asc_UserDataDestroyCmd,
    "user-list",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__userdata_save", Asc_UserDataSaveValuesCmd,
    "user-list",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__userdata_restore", Asc_UserDataRestoreValuesCmd,
    "user-list",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__userdata_query", Asc_UserDataQueryCmd,
    "user-list",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"__userdata_print", Asc_UserDataPrintLibrary,
    "user-list",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  /* Solver Routines */
    /* Solve_* are routines calling the commandline solver */
    /* Solv_[SOLVERNAME_]<cfunction> are commands for the windowed solvers */

#if DELETEME
  ASCADDCOM(interp,"bexp_s",Asc_Brow2Solve,
    "miscellaneous",
    NULL,
    "bexp_s     -- export browser instance to solver context",
    NULL
    /* the stuff above should be replaced with header macros. */
  );
#endif /* DELETEME */

  ASCADDCOM(interp,"get_model_children",Asc_SolvGetModKids,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

#if DELETEME
  ASCADDCOM(interp,"slv_import_sim",Asc_Sims2Solve,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"ssolve",Asc_SolvSimInst,
    "solver",
    NULL,
    "ssolve     -- fire up commandline solver <n> on <name>. DON'T.",
    NULL
    /* the stuff above should be replaced with header macros. */
  );
#endif /* DELETEME */

#if 0 
  ASCADDCOM(interp,"solve",Asc_SolvCurInst,
    "solver",
    NULL,
    "solve      -- fire up commandline solver <n>. on g_curinst. DON'T.",
    NULL
    /* the stuff above should be replaced with header macros. */
  );
#endif

#ifdef ASC_SIGNAL_TRAPS
  ASCADDCOM(interp,"slv_trapfp",Asc_SolvTrapFP,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_untrapfp",Asc_SolvUnTrapFP,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_trapint",Asc_SolvTrapINT,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_untrapint",Asc_SolvUnTrapINT,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );
#endif

  ASCADDCOM(interp,"slv_checksim",Asc_SolvIncompleteSim,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_checksys",Asc_SolvCheckSys,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_get_obj_list",Asc_SolvGetObjList,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_set_obj_by_num",Asc_SolvSetObjByNum,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,Asc_SolvGetObjNumCmdHN,Asc_SolvGetObjNumCmd,
    "solver",Asc_SolvGetObjNumCmdHU,Asc_SolvGetObjNumCmdHS,
    Asc_SolvGetObjNumCmdHLF);

  ASCADDCOM(interp,"slv_get_parms",Asc_SolvGetSlvParms,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"set_slv_parms",Asc_SolvSetSlvParms,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"set_slv_parmsnew",Asc_SolvSetSlvParmsNew,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_get_parmsnew",Asc_SolvGetSlvParmsNew,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

/* what's up with this? */
/*  Tcl_CreateCommand(interp,"slv_get_bool_parms",Asc_SolvGetSlvBoolParms,
 *                   (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 *Tcl_CreateCommand(interp,"slv_get_real_parms",Asc_SolvGetSlvRealParms,
 *                   (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 */

  ASCADDCOM(interp,"slv_get_insttype",Asc_SolvGetInstType,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_get_cost_page",Asc_SolvGetSlvCostPage,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_get_stat_page",Asc_SolvGetSlvStatPage,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_get_objval",Asc_SolvGetObjectiveVal,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_get_instname",Asc_SolvGetInstName,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_get_pathname",Asc_SolvGetPathName,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_get_vr",Asc_SolvGetVRCounts,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slvdump",Asc_SolvSlvDumpInt,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_presolve",Asc_SolvSlvPresolve,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_reanalyze",Asc_SolvReanalyze,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_check_and_reanalyze",Asc_SolvCheckAndReanalyze,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_set_independent",Asc_SolvMakeIndependent,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_resolve",Asc_SolvSlvResolve,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_solve",Asc_SolvSlvSolve,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_iterate",Asc_SolvSlvIterate,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_available",Asc_SolvAvailSolver,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_number",Asc_SolvSolverNum,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_name",Asc_SolvSolverName,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_linsol_names",Asc_SolvLinsolNames,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_eligible_solver",Asc_SolvEligSolver,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_select_solver",Asc_SolvSelectSolver,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_get_solver",Asc_SolvGetSelectedSolver,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_flush_solver",Asc_SolvFlushSolver,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_import_qlfdid",Asc_SolvImportQlfdid,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_lnmget",Asc_SolvGetLnmEpsilon,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_lnmset",Asc_SolvSetLnmEpsilon,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_set_haltflag",Asc_SolvSetCHaltFlag,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slvhelp",Asc_SolvHelpList,
    "solver",
    NULL,
    "slvhelp    -- give tcl/[s]hort/[l]ong solver primitive list",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_near_bounds",Asc_SolvNearBounds,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"slv_far_from_nominals",Asc_SolvFarFromNominal,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,Asc_SolveMonitorCmdHN,Asc_SolveMonitorCmd,"solver",
    Asc_SolveMonitorCmdHU, Asc_SolveMonitorCmdHS, Asc_SolveMonitorCmdHLF);




  /* solver Debugger commands */
  ASCADDCOM(interp,"dbg_get_blk_of_var",Asc_DebuGetBlkOfVar,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_get_blk_of_eqn",Asc_DebuGetBlkOfEqn,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_get_blk_coords",Asc_DebuGetBlkCoords,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_get_eqn_of_var",Asc_DebuGetEqnOfVar,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_get_varpartition",Asc_DebuGetVarPartition,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_get_eqnpartition",Asc_DebuGetEqnPartition,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_list_rels",Asc_DebuListRels,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_list_vars",Asc_DebuListVars,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_write_var",Asc_DebuWriteVar,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_write_unattvar",Asc_DebuWriteUnattachedVar,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  Asc_AddCommand(interp,"brow_write_var",Asc_DebuWriteVar,
                 (ClientData) 1, (Tcl_CmdDeleteProc *) NULL,
    "miscellaneous",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  ASCADDCOM(interp,"dbg_write_rel",Asc_DebuWriteRel,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  /* do we need this function? */
  Asc_AddCommand(interp,"brow_write_rel",Asc_DebuWriteRel,
                 (ClientData) 1, (Tcl_CmdDeleteProc *) NULL,
    "miscellaneous",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_write_obj",Asc_DebuWriteObj,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  /* do we need this function? */
  Asc_AddCommand(interp,"brow_write_obj",Asc_DebuWriteObj,
                 (ClientData) 1, (Tcl_CmdDeleteProc *) NULL,
    "miscellaneous",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_write_varattr",Asc_DebuWriteVarAttr,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  Asc_AddCommand(interp,"dbg_write_qlfattr",Asc_DebuWriteVarAttr,
                     (ClientData)1, (Tcl_CmdDeleteProc *) NULL,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_rel_included",Asc_DebuRelIncluded,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_var_fixed",Asc_DebuVarFixed,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_get_incidence",Asc_DebuGetIncidence,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_get_order",Asc_DebuGetOrder,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_write_incidence",Asc_DebuWriteIncidence,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_find_eligible",Asc_DebuFindEligible,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"brow_find_eligible",Asc_DebuInstEligible,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_consistency_analysis",Asc_DebuConsistencyAnalysis,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_global_eligible",Asc_DebuFindGlobalEligible,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  ASCADDCOM(interp,"dbg_find_activerels",Asc_DebuFindActive,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"brow_find_activerels",Asc_DebuInstActive,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_struct_singular",Asc_DebuStructSing,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_num_block_singular",Asc_DebuNumBlockSing,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"var_free2nom",Asc_DebuVarFree2Nom,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"var_nom2free",Asc_DebuVarNom2Free,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_calc_relnoms",Asc_DebuCalcRelNominals,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_check_rels",Asc_DebuCheckRelFp,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbg_write_slv0_xsys",Asc_DebuWriteSystem,
    "fileio",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  Asc_AddCommand(interp,"dbg_write_slv0_sys",Asc_DebuWriteSystem,
                 (ClientData) 1, (Tcl_CmdDeleteProc *) NULL,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  Asc_AddCommand(interp,"dbg_mtxwriteplot",Asc_DebuMtxWritePlotCmd,
                 (ClientData) 1, (Tcl_CmdDeleteProc *) NULL,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  Asc_AddCommand(interp,"dbg_calc_jacobian",Asc_DebuMtxCalcJacobianCmd,
                     (ClientData) 1, (Tcl_CmdDeleteProc *) NULL,
    "debugger",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"dbghelp",Asc_DebuHelpList,
    "debugger",
    NULL,
    "dbghelp    -- give tcl/[s]hort/[l]ong debugger primitives list",
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  /* ivp solver commands */
  ASCADDCOM(interp,"integrate_setup", Asc_IntegSetupCmd,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"integrate_cleanup", Asc_IntegCleanupCmd,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"integrate_set_y_file", Asc_IntegSetYFileCmd,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"integrate_set_obs_file", Asc_IntegSetObsFileCmd,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"integrate_logunits", Asc_IntegSetFileUnitsCmd,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"integrate_logformat", Asc_IntegSetFileFormatCmd,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"integrate_get_samples", Asc_IntegGetXSamplesCmd,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"integrate_set_samples", Asc_IntegSetXSamplesCmd,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"integrate_able", Asc_IntegInstIntegrableCmd,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  /* This code is in Sensitivity.[ch] but needs a home */
  /* Also need a different name as the Mtx name space is already taken. */
  ASCADDCOM(interp,"__mtx_norms", Asc_MtxNormsCmd,
    "solver",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  /* Working very hard to avoid direct X code, here we make a command
     set for handling incidence matrices via Tcl/tk without the
     amazing overhead of a tcl interpreter getting in the way.
     Avoids a _lot_ of list operations.
  */
  /* mtx incidence commands */
  ASCADDCOM(interp,"mtx_gui_plot_incidence",Asc_MtxGUIPlotIncidence,
    "mtxplot",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"mtxhelp",Asc_MtxHelpList,
    "mtxplot",
    NULL,
    "mtxhelp    -- give tcl/[s]hort/[l]ong incid matrix primitives.",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  /* Units window stuff */
  ASCADDCOM(interp,"u_destroy_units",Asc_UnitDestroyDisplayList,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_setSIdef",Asc_UnitDefaultBaseUnits,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_getbasedef",Asc_UnitGetBaseUnits,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_dump",Asc_UnitDump,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_dims",Asc_DimenDump,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_dim_setverify",Asc_DimenRelCheck,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_dim2num",Asc_UnitBaseDimToNum,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_num2dim",Asc_UnitNumToBaseDim,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_frombasedim",Asc_UnitMatchBaseDim,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_fromatomdim",Asc_UnitMatchAtomDim,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_getdimatoms",Asc_UnitGetAtomList,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_change_baseunit",Asc_UnitChangeBaseUnit,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_getprec",Asc_UnitGetPrec,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_setprec",Asc_UnitSetPrec,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_get_atoms",Asc_UnitGetAtomsForUnit,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );


  ASCADDCOM(interp,"u_get_units",Asc_UnitGetUnits,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_set_user",Asc_UnitSetUser,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_get_user",Asc_UnitGetUser,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_get_list",Asc_UnitGetList,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_clear_user",Asc_UnitClearUser,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_getval",Asc_UnitGetVal,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_browgetval",Asc_UnitBrowGetVal,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_slvgetrelval",Asc_UnitSlvGetRelVal,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_slvgetvarval",Asc_UnitSlvGetVarVal,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"u_slvgetobjval",Asc_UnitSlvGetObjVal,
    "units",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"uhelp",Asc_UnitHelpList,
    "units",
    NULL,
    "uhelp      -- give tcl/[s]hort/[l]ong units primitives list.",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  /*Script callbacks */
  ASCADDCOM(interp,"srefine", Asc_ScriptRefineCmd,
    "scripting",
    NULL,
    "srefine    -- refine qlfdid to type given",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"smerge", Asc_ScriptMergeCmd,
    "scripting",
    NULL,
    "smerge     -- merge 2 qlfdids",
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  /* Auxiliaries and Window Management */
  ASCADDCOM(interp,"ascloadwin",Asc_LoadWin,
    "miscellaneous",
    NULL,
    NULL,
    NULL
    /* the stuff above should be replaced with header macros. */
  );

  ASCADDCOM(interp,"gnutext",Asc_GNUTextCmd,
    "miscellaneous",
    NULL,
    "gnutext    -- give compiler license [l] or warranty [w]",
    NULL
    /* the stuff above should be replaced with header macros. */
  );


#if REIMPLEMENT
  Tcl_CreateCommand(interp,"dbg_write_kirk_xsys",Asc_DebuWriteKirkSystem,
                    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp,"dbg_write_gams_xsys",Asc_DebuWriteGAMSSystem,
                    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
#endif
#if REIMPLEMENT
  /* code generation commands */
  Tcl_CreateCommand(interp,"__codegen_parsedata",Asc_CodeGenParseDataCmd,
                    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp,"__codegen_general",Asc_CodeGenGeneralCmd,
                    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp,"__codegen_c",Asc_CodeGenCCmd,
                    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp,"__codegen_gams",Asc_CodeGenGamsCmd,
                    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp,"__codegen_write",Asc_CodeGenWriteCmd,
                    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp,"__codegen_read",Asc_CodeGenReadCmd,
                    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
#endif
  /* some experimental stuff */
#if REIMPLEMENT
  Tcl_CreateCommand(interp,"__codegen_types",Asc_CodeGenTypesCmd,
                    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
#endif
  /* Dag Routines */
#if KILLBDAG
  Tcl_CreateCommand(interp,"__brow_tree_list", Asc_BrowTreeListCmd,
                    (ClientData) NULL,(Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp,"__dag_write_instdag", Asc_DagWriteInstDagCmd,
                    (ClientData) NULL,(Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp,"__dag_write_modeldag", Asc_DagWriteModelDagCmd,
                    (ClientData) NULL,(Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp,"__dag_partition", Asc_DagPartitionCmd,
                    (ClientData) NULL,(Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp,"__dag_countrelns", Asc_DagCountRelnsCmd,
                    (ClientData) NULL,(Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp,"__dag_coupling_relns", Asc_DagCouplingRelnsCmd,
                    (ClientData) NULL,(Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp,"__dag_model_relns", Asc_DagModelRelnsCmd,
                    (ClientData) NULL,(Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp,"__dag_build", Asc_DagBuildDagCmd,
                    (ClientData) NULL,(Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp,"__dag_prepare", Asc_DagPrepareCmd,
                    (ClientData) NULL,(Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp,"__dag_shutdown", Asc_DagShutdownCmd,
                    (ClientData) NULL,(Tcl_CmdDeleteProc *) NULL);
#endif /* killbdag */
#ifdef DEBUG_MALLOC
  /* See GUIinit.[ch] for definition of Asc_DebugMallocCmd */
  Tcl_CreateCommand(interp,"__dbmalloc",Asc_DebugMallocCmd,
                    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
#endif

#ifdef BALLAN /* testing crap not checked in */

  ASCADDCOM(interp, Asc_LibrAnonTypesCmdHN, Asc_LibrAnonTypesCmd,
    "library", Asc_LibrAnonTypesCmdHU,  Asc_LibrAnonTypesCmdHS,
    Asc_LibrAnonTypesCmdHLF);

  ASCADDCOM(interp, Asc_DotCmdHN, Asc_DotCmd, "browser",
    Asc_DotCmdHU,  Asc_DotCmdHS, Asc_DotCmdHLF);

#endif /* ballan */

}

