rem Build the Ascend base libraries
cd ..\..\base\jam
jam -f ..\..\jam\Jambase libs

rem Build the Ascend executable
cd ..\..\tcltk98\jam
jam -f ..\..\jam\Jambase ascend

