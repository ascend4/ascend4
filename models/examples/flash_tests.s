# This file is part of the Ascend modeling library to
# demonstrate flash_tests.asc and is released under the GNU
# Public License as noted at the beginning of flash_tests.asc.

# $Id: flash_tests.s,v 1.9 1997/08/28 14:42:58 ballan Exp $
#
#
#	flash_tests.s by Robert S. Huss
#
#	This file is a script to run the flash examples found in
#	flash_tests.asc. 
#
#	The script can be run straight through, but I recommend 
#	running it line by line so you can observe what ASCEND is
#	doing

DELETE TYPES;
set libraries $env(ASCENDDIST)/models/libraries;
set examples  $env(ASCENDDIST)/models/examples;
READ FILE $libraries/system.lib;
READ FILE $libraries/measures.lib;
READ FILE $libraries/atoms.lib;
READ FILE $libraries/components.lib;
READ FILE $libraries/H_G_thermodynamics.lib;
READ FILE $libraries/stream.lib;
READ FILE $libraries/flash.lib;
READ FILE $examples/flash_tests.asc;

# test_flash n-alkanes
COMPILE {ft} OF {test_flash};
BROWSE {ft};
RUN {ft.values};
RUN {ft.flash.reset};
RUN {ft.flash.scale};
PROBE current ft.flash.alpha {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {ft.flash.input['feed'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current ft.flash.VLE.phi {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {ft.flash.liqout['liquid'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {ft.flash.vapout['vapor'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {ft.flash.VLE.T} {};
SOLVE {ft};

# use these three statements, or REFINE {a, b, and c} TO {your choice}
# click on 'components' in the left half of the Library window for choices
REFINE {ft.flash.data['a']} TO {n_pentane};
REFINE {ft.flash.data['b']} TO {n_hexane};
REFINE {ft.flash.data['c']} TO {n_heptane};
REFINE {ft.flash} TO {td_VLE_flash};
PROBE current {ft.flash.Qin} {};
REFINE {ft.flash.input['feed']} TO {liquid_stream};
RESUME;
RUN {ft.flash.reset};
RUN {ft.flash.scale};
SOLVE {ft};

REFINE {ft.flash.VLE} TO {equilibrium_mixture};
RUN {ft.flash.reset};
RUN {ft.flash.scale};
SOLVE {ft};

DELETE ft;
# everything in the probe becomes uncertain, but we want it later.

# test_flash azeotropic. same name ft.
COMPILE {ft} OF {test_flash};
# note that the probe became less uncertain.
BROWSE {ft};
RUN {ft.values};
RUN {ft.flash.reset};
RUN {ft.flash.scale};
SOLVE {ft};

# use these three statements, or REFINE {a, b, and c} TO {your choice}
REFINE {ft.flash.data['a']} TO {acetone};
REFINE {ft.flash.data['b']} TO {benzene};
REFINE {ft.flash.data['c']} TO {chloroform};
REFINE {ft.flash} TO {td_VLE_flash};
REFINE {ft.flash.input['feed']} TO {liquid_stream};
RESUME;
RUN {ft.flash.reset};
RUN {ft.flash.scale};
SOLVE {ft};
REFINE {ft.flash.VLE} TO {equilibrium_mixture};
RUN {ft.flash.reset};
RUN {ft.flash.scale};
SOLVE {ft};



# simple_tray_test 
COMPILE {ts} OF {td_test_simple_tray};
BROWSE {ts};
RUN {ts.values};
RUN {ts.flash.reset};
RUN {ts.flash.scale};
SOLVE {ts};
REFINE {ts.flash.VLE} TO {equilibrium_mixture};
RUN {ts.flash.reset};
PROBE new {ts.flash.alpha} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {ts.flash.input['liquid'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {ts.flash.input['liquid'].state.T} {};
PROBE current {ts.flash.input['vapor'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {ts.flash.input['vapor'].state.T} {};
PROBE current {ts.flash.VLE.phi} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {ts.flash.cmo_ratio} {};
PROBE current {ts.flash.Qin} {};
PROBE current {ts.flash.liqout['liquid'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {ts.flash.vapout['vapor'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {ts.flash.VLE.T} {};
SOLVE {ts};

RUN {ts.flash.heat_balance};
RUN {ts.zero_Q};
SOLVE {ts};

# simple_feed_tray test 
COMPILE {tft} OF {td_test_simple_feed_tray};
BROWSE {tft};
RUN {tft.values};
RUN {tft.flash.reset};
RUN {tft.flash.scale};
SOLVE {tft};
PROBE new {tft.flash.alpha} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {tft.flash.input['feed'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {tft.flash.input['feed'].state.T} {};
PROBE current {tft.flash.input['feed'].state.phi} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {tft.flash.input['liquid'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {tft.flash.input['liquid'].state.T} {};
PROBE current {tft.flash.input['vapor'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {tft.flash.input['vapor'].state.T} {};
PROBE current {tft.flash.VLE.phi} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {tft.flash.q} {};
PROBE current {tft.flash.Qin} {};
PROBE current {tft.flash.liqout['liquid'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {tft.flash.vapout['vapor'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {tft.flash.VLE.T} {};
REFINE {tft.flash.VLE} TO {equilibrium_mixture};
RUN {tft.flash.reset};
SOLVE {tft};
RUN {tft.flash.heat_balance};
RUN {tft.zero_Q};
SOLVE {tft};

# condenser_test 
COMPILE {tc} OF {td_test_condenser};
BROWSE {tc};
RUN {tc.values};
RUN {tc.flash.reset};
RUN {tc.flash.scale};
SOLVE {tc};
REFINE {tc.flash.VLE} TO {equilibrium_mixture};
RUN {tc.flash.reset};
PROBE new {tc.flash.alpha} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};;
PROBE current {tc.flash.input['vapor'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};;
PROBE current {tc.flash.input['vapor'].state.T} {};
PROBE current {tc.flash.VLE.phi} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};;
PROBE current {tc.flash.reflux_ratio} {};
PROBE current {tc.flash.Qin} {};
PROBE current {tc.flash.liqout['liquid'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};;
PROBE current {tc.flash.vapout['vapor_product'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};;
PROBE current {tc.flash.liqout['distillate'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};;
PROBE current {tc.flash.VLE.T} {};
SOLVE {tc};

# reboiler_test 
COMPILE {tr} OF {td_test_reboiler};
BROWSE {tr};
RUN {tr.values};
RUN {tr.flash.reset};
RUN {tr.flash.scale};
SOLVE {tr};
REFINE {tr.flash.VLE} TO {equilibrium_mixture};
RUN {tr.flash.reset};
PROBE current {tr.flash.alpha} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};;
PROBE current {tr.flash.input['liquid'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};;
PROBE current {tr.flash.input['liquid'].state.T} {};
PROBE current {tr.flash.VLE.phi} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};;
PROBE current {tr.flash.reboil_ratio} {};
PROBE current {tr.flash.Qin} {};
PROBE current {tr.flash.vapout['vapor'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {tr.flash.vapout['vapor_product'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {tr.flash.liqout['bottoms'].f} {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0};
PROBE current {tr.flash.VLE.T} {};
SOLVE {tr};

