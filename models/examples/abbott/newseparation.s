# ASCEND IV script to test PPP splitter alias recovery backend.
# by ben allan, 5/17/96.
#
DELETE TYPES;

#  The following variables tell ASCEND where to find files.
#  If you set it to the empty string, ASCEND uses the ASCENDLIBRARY
#  environment variable to find files (this is the recommended value).
#  If you set it to a directory, include the trailing slash in the
#  directory name.
#  If you are running under Windows, use the FORWARD slash / where DOS
#  would normally expect a BACKWARD slash \.
#
#set top "$env(ASCENDDIST)/models/";
#set top /usr/local/lib/ascend/models/;
#
#set lib "${top}libraries/";
#set ex  "${top}examples/";

set top "";
set lib "";
set ex  "";

set kirklib "${lib}abbott/";
set kirkex  "${ex}abbott/";

READ FILE "${lib}system.lib";
READ FILE "${lib}atoms.lib";
READ FILE "${lib}components.lib";
READ FILE "${lib}H_G_thermodynamics.lib";
READ FILE "${lib}stream.lib";
READ FILE "${lib}newintegration.lib";
READ FILE "${lib}plot.lib";
READ FILE "${kirklib}newflowsheet.lib";
READ FILE "${kirklib}newethylene_flash.lib";
READ FILE "${kirklib}newethylene_column.lib"; 
READ FILE "${kirklib}newethylene_ppp_flash.lib";
READ FILE "${kirklib}newethylene_ppp_column.lib";
READ FILE "${kirkex}separation.asc";
READ FILE "${kirkex}tube_reactor.asc";
READ FILE "${kirkex}furnace.asc";
READ FILE "${kirkex}plant.asc";

# the following sets of statements generate prototype models
# which we can then use to copy rather than reinterpretting
# dozens of times in building a flowsheet.
COMPILE ppp_tray td_simple_tray_PPP;
BROWSE ppp_tray;
PROTOTYPE ppp_tray;

COMPILE t1 OF td_simple_tray;
BROWSE t1;
PROTOTYPE t1;

COMPILE k1 OF kinetics_2;
BROWSE k1;
PROTOTYPE k1;

# If you run this statement you get a 17.5K equations instance
# in about 12M and 12 seconds. Don't do this.
# COMPILE rb recovery_backend;

# Now we will build an 79554 equations instance
# in about 62M and 85 seconds. Watch the dust.
# The above cost estimates are for unoptimized ascend4 code
# on tika.ndim.edrc.cmu.edu. Elsewhere, particularly
# on alphas, your mileage may vary.
COMPILE p1 OF plant;
