#!/bin/sh
#
# Patch for xf re-generated sources.
#
exit

overwrite library.tcl sed 's/# Window manager configurations/# Ascend patched window manager configurations \
  wm iconify .library/' library.tcl

overwrite sims.tcl sed 's/# Window manager configurations/# Ascend patched window manager configurations \
  wm iconify .sims/' sims.tcl

overwrite browser.tcl sed 's/# Window manager configurations/# Ascend patched window manager configurations \
  wm iconify .browser/' browser.tcl

overwrite solver.tcl sed 's/# Window manager configurations/# Ascend patched window manager configurations \
  wm iconify .solver/' solver.tcl

overwrite probe.tcl sed 's/# Window manager configurations/# Ascend patched window manager configurations \
  wm iconify .probe/' probe.tcl

overwrite display.tcl sed 's/# Window manager configurations/# Ascend patched window manager configurations \
  wm iconify .display/' display.tcl

overwrite script.tcl sed 's/# Window manager configurations/# Ascend patched window manager configurations \
  wm iconify .script/' script.tcl

overwrite units.tcl sed 's/# Window manager configurations/# Ascend patched window manager configurations \
  wm iconify .units/' units.tcl

overwrite toolbox.tcl sed 's/# Window manager configurations/# Ascend patched window manager configurations \
  wm iconify .toolbox/' toolbox.tcl


