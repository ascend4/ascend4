# parameterized flash and column test script
# by  Ben Allan, March 30, 1997.

#  The ``lib'' and ``ex'' variables tell ASCEND where to find files.
#  If you set it to the empty string, ASCEND uses the ASCENDLIBRARY
#  environment variable to find files (this is the recommended value).
#  If you set it to a directory, include the trailing slash in the
#  directory name.
#  If you are running under Windows, use the FORWARD slash / where DOS
#  would normally expect a BACKWARD slash \.
#
#set lib $env(ASCENDDIST)/models/libraries/
#set ex  $env(ASCENDDIST)/models/examples/
#set lib /usr/local/lib/ascend/models/libraries/
#set ex  /usr/local/lib/ascend/models/examples/
#set lib /usr0/ballan/new/ascend4/models/libraries/
#set ex  /usr0/ballan/new/ascend4/models/examples/
set lib "";
set ex  "";

DELETE TYPES;
READ FILE "${lib}system.lib";
READ FILE "${lib}atoms.lib";
READ FILE "${lib}components.par.lib";
READ FILE "${lib}propertyoptions.par.lib";
READ FILE "${lib}HGthermo.par.lib";
READ FILE "${lib}stream.par.lib";
READ FILE "${lib}flash.par.lib";
READ FILE "${lib}column.par.lib";
READ FILE "${lib}plot.par.lib";
READ FILE "${ex}plotcol.par.asc"; 
#READ FILE "${ex}backend.par.asc";

# C5/C6/C7 flash. Liquid feed, vapor and liquid products
COMPILE tvlf OF test_vapor_liquid_flash;
BROWSE {tvlf};
RUN {tvlf.values};
ASSIGN tvlf.equilibrated FALSE;
RUN {tvlf.reset};
SOLVE {tvlf} WITH QRSlv;
DELETE SYSTEM;
ASSIGN tvlf.equilibrated TRUE;
RUN {tvlf.reset};
SOLVE {tvlf} WITH QRSlv;

# Acetone-Benzene-Chloroform flash
COMPILE thvlf OF test_hard_vapor_liquid_flash;
BROWSE {thvlf};
RUN {thvlf.values};
ASSIGN thvlf.equilibrated FALSE;
RUN {thvlf.reset};
SOLVE {thvlf} WITH QRSlv;
DELETE SYSTEM;
ASSIGN thvlf.equilibrated TRUE;
RUN {thvlf.reset};
SOLVE {thvlf} WITH QRSlv;

# Internal Tray
COMPILE tt OF test_tray;
BROWSE {tt};
RUN {tt.values};
ASSIGN tt.equilibrated FALSE;
RUN {tt.reset};
SOLVE {tt} WITH QRSlv;
DELETE SYSTEM
ASSIGN tt.equilibrated TRUE;
RUN {tt.reset};
SOLVE {tt} WITH QRSlv;
RUN tt.Tray.reset_to_adiabatic;
SOLVE {tt} WITH QRSlv;

# "Simple" feed tray with 1 V/L feed.
COMPILE tft OF test_feed_tray;
BROWSE {tft};
RUN {tft.values};
ASSIGN tft.equilibrated FALSE;
RUN {tft.reset}; 
SOLVE {tft} WITH QRSlv;
DELETE SYSTEM;
ASSIGN tft.equilibrated TRUE;
RUN {tft.reset};
SOLVE {tft} WITH QRSlv;
RUN {tft.Feed_tray.reset_to_adiabatic};
SOLVE {tft} WITH QRSlv;

# "Simple" Condenser with 1 liquid product
COMPILE tc OF test_condenser;
BROWSE {tc};
RUN {tc.values};
RUN {tc.reset};
SOLVE {tc} WITH QRSlv;

# "Simple" Reboiler with 1 liquid product
COMPILE tr OF test_reboiler;
BROWSE {tr};
RUN {tr.values};
ASSIGN tr.equilibrated FALSE;
RUN {tr.reset};
SOLVE {tr} WITH QRSlv;
DELETE SYSTEM;
ASSIGN tr.equilibrated TRUE;
RUN {tr.reset};
SOLVE {tr} WITH QRSlv;

# Internal Tray Stack Model
COMPILE tts OF test_tray_stack;
BROWSE {tts};
ASSIGN {tts.equilibrated} FALSE;
RUN {tts.reset};
SOLVE {tts} WITH QRSlv;
DELETE SYSTEM;
ASSIGN {tts.equilibrated} TRUE;
RUN {tts.reset};
SOLVE {tts} WITH QRSlv;
RUN {tts.reset_to_adiabatic};
SOLVE {tts} WITH QRSlv;

# "Simple" Column Model
# Note that some of the solves in the following model
# may require a few more iterations than the solver
# gives by default, depending on the solver scaling
# and factorization and step control options.
# Just hit the Solve button again.

COMPILE {tsc} OF {test_simple_column};
BROWSE {tsc};
RUN {tsc.values};

# Configure to solve mass balances first
ASSIGN tsc.Equilibrated FALSE;
RUN {tsc.Column.reset};
SOLVE tsc;

# Configure to solve mass and energy balances
# with slack Qin on each tray, fixed RR and BU.
# It might be easier to free BU (reboil_ratio) and
# specify the distillate.Ftot flow.
DELETE SYSTEM;
ASSIGN tsc.Equilibrated TRUE;
RUN {tsc.Column.reset};
SOLVE {tsc};

# Configure to solve energy balanced column
# with fixed RR and BU and saturated liquid feed.
RUN {tsc.Column.reset_to_adiabatic};
SOLVE {tsc};

COMPILE tmwdc OF mw_demo_column;
BROWSE {tmwdc};
RUN {tmwdc.mw_column.values};
RUN {tmwdc.mw_column.Column.reset_to_massbal};
SOLVE {tmwdc} WITH QRSlv;
RUN {tmwdc.mw_column.Column.reset_to_fullthermo};
SOLVE {tmwdc} WITH QRSlv;
RUN {tmwdc.mw_column.Column.reset_to_adiabatic};
SOLVE {tmwdc} WITH QRSlv;

COMPILE tabcdc OF abc_demo_column;
BROWSE {tabcdc};
RUN tabcdc.abc_column.values;
RUN {tabcdc.abc_column.Column.reset_to_massbal};
SOLVE {tabcdc.abc_column.Column} WITH QRSlv;
RUN {tabcdc.abc_column.Column.reset_to_fullthermo};
SOLVE {tabcdc.abc_column.Column} WITH QRSlv;
RUN {tabcdc.abc_column.Column.reset_to_adiabatic};
SOLVE {tabcdc.abc_column.Column} WITH QRSlv;

COMPILE tc567dc OF c567_demo_column;
BROWSE {tc567dc};
RUN {tc567dc.c567_column.values};
RUN {tc567dc.c567_column.Column.reset_to_massbal};
SOLVE {tc567dc} WITH QRSlv;
RUN tc567dc.c567_column.scale;
RUN {tc567dc.c567_column.Column.reset_to_fullthermo};
SOLVE {tc567dc} WITH QRSlv;
RUN tc567dc.c567_column.scale;
RUN {tc567dc.c567_column.Column.reset_to_adiabatic};
SOLVE {tc567dc} WITH QRSlv;

COMPILE mwdpc OF mw_demo_plot_column;
BROWSE {mwdpc};
RUN {mwdpc.mw_column.values};
RUN {mwdpc.mw_column.Column.reset_to_massbal};
SOLVE {mwdpc} WITH QRSlv;
RUN {mwdpc.mw_column.Column.reset_to_fullthermo};
SOLVE {mwdpc} WITH QRSlv;
RUN {mwdpc.mw_column.Column.reset_to_adiabatic};
SOLVE {mwdpc} WITH QRSlv;

PLOT {mwdpc.mw_column.Plot_K} ;
SHOW LAST;
PLOT {mwdpc.mw_column.Plot_x} ;
SHOW LAST;
PLOT {mwdpc.mw_column.Plot_y} ;
SHOW LAST;
PLOT {mwdpc.mw_column.Plot_P} ;
SHOW LAST;
PLOT {mwdpc.mw_column.Plot_T} ;
SHOW LAST;


COMPILE c567dpc OF c567_demo_plot_column;
BROWSE {c567dpc};
RUN {c567dpc.c567_column.values};
RUN {c567dpc.c567_column.Column.reset_to_massbal};
SOLVE {c567dpc} WITH QRSlv;
RUN {c567dpc.c567_column.Column.reset_to_fullthermo};
SOLVE {c567dpc} WITH QRSlv;
SOLVE {c567dpc} WITH QRSlv;
RUN {c567dpc.c567_column.Column.reset_to_adiabatic};
SOLVE {c567dpc} WITH QRSlv;
PLOT {c567dpc.c567_column.Plot_K} ;
SHOW LAST;
PLOT {c567dpc.c567_column.Plot_T} ;
SHOW LAST;
PLOT {c567dpc.c567_column.Plot_x} ;
SHOW LAST;
PLOT {c567dpc.c567_column.Plot_y} ;
SHOW LAST;
PLOT {c567dpc.c567_column.Plot_P} ;
SHOW LAST;




