$! This is a command file to run MINOS on DEC VAX/VMS systems.
$
$  if P1 .eqs. "?" then goto HELP
$  if P1 .eqs. ""  then goto HELP
$  on  control_Y   then goto DONE
$  set noon
$
$  MPSDIR = "[-]"   ! Say here where to find MPS files.
$  SPECS  = P1
$  if P2 .nes. "" then SPECS = P2
$  write sys$output "Defining ''P1' files..."
$  write sys$output "Will use ''SPECS'.spc"
$
$  close/nolog  for004
$  close/nolog  for010
$  open/read	for004 'SPECS'.spc
$  define/nolog for006  sys$output
$  define/nolog	for009 'P1'.lis
$  open/read	for010  'MPSDIR''P1'.mps
$  define/nolog	for011 'P1'.basis
$  define/nolog	for012 'P1'.newbasis
$
$  dir 'P1' /size=all /date
$  if SPECS .nes. P1 then dir 'SPECS' /size=all /date 
$
$  define/user_mode sys$input sys$command
$  run 'P3' MINOS
$
$ DONE:
$  close/nolog  for004
$  close/nolog  for010
$  set on
$  exit
$
$ HELP:
$  write sys$output "Say @minos t4manne     to use t4manne.mps, t4manne.spc"
$  write sys$output "or  @minos t4manne nlp to use t4manne.mps, nlp.spc"
$  write sys$output "or  @minos t4manne nlp /nodebug to run without debug"
