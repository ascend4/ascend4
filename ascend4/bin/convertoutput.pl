#!/usr/local/bin/perl

# This program opens a file called lookup and uses
# it to convert the gams output with substitute
# variables into gams output with real variables.
# The command line for this program is as follows:
# convertoutput <filename> <simulation_name>
#
# $Revision: 1.2 $
# $Author: ballan $
# $Date: 1997/07/05 14:12:08 $
#

$filename = shift(@ARGV);
$sim = shift(@ARGV);

# split the list filename into the base and its extension
($base,$extension) = split(/\./,$filename);

# concatenate the base with the extension .con
$outfile = $base."\.con";

open(OUTPUT, ">$outfile");
open(INPUT, "$filename");

# need to open the look-up table for the variables

# concatenate the base the the extensions .gms and .lk
# this is the format of the lookup file name as set in
# ascend2gms.pl
$lookup = $base."\.gms\.lk";


while (<INPUT>) {

# set some switches so you know where you are in the file you are parsing
	if (/^.*VAR /){
		$convert = 1;
		$display = 0;
	    }else{
		$convert = 0;
	    }

        if (/^.*VARIABLE /){
                $convert = 0;
                $display = 1;
            }else{
                $display = 0;
            }

# if we are in the variables section
	if($convert == 1){

# the lines being read are of the form
# ---- VAR x 0 .5 1 .23
# these lines are split and the parts are saved in the appropriate variables
	    ($spacer, $type, $dummy, $lower, $level, $upper, $marginal) = split(' ', $_);

	    if($dummy == Z1) {
# since z1 is the actual variable, don't change it
		$actual = $dummy;
	    }

	    $outerlinenum = 0;

# concatenate the dummy variable with a return
	    $dummy = $dummy."\n";

# open the lookup table
	    open(TABLE, "$lookup");
	    while (<TABLE>) {
		$outerlinenum = $outerlinenum + 1;
		$innerlinenum = 0;

# count the number of lines in the lookup table until we find the dummy variable
		if(/^$dummy/i){
# open the lookup table again and go to the line number before the dummy variable
		    open(TABLE, "$lookup");
		    while(<TABLE>) {
			$innerlinenum = $innerlinenum + 1;
			if($innerlinenum == $outerlinenum - 1){

# set the actual variable to that at the corresponding line
			    $actual = $_;
			}
		    }
		}
	    }

# remove the return character from the variable
	    chop($actual);

# format the output so it is easy to read
            format OUTPUT =
@>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> @>>>>>>>>>> @>>>>>>>>>> @>>>>>>>>>> @>>>>>>>>>>
$actual,$lower,$level,$upper,$marginal
.
    write OUTPUT;
        }elsif($display == 1){
            ($type, $dummyplusextension, $equalsign, $level) = split(' ', $_);
            $outerlinenum = 0;
            ($dummy,$extension) = split(/\./,$dummyplusextension);
            $dummy = $dummy."\n";
            open(TABLE, "$lookup");
            while (<TABLE>) {
                $outerlinenum = $outerlinenum + 1;
                $innerlinenum = 0;
                if(/^$dummy/i){
                    open(TABLE, "$lookup");
                    while(<TABLE>) {
                        $innerlinenum = $innerlinenum + 1;
                        if($innerlinenum == $outerlinenum - 1){
                            $actual = $_;
                        }
                    }
                }
            }
            chop($actual);
# with the format of the output, you can directly cut and paste into the script window
# this currently assumes that your simulation was called t1
            print OUTPUT "ASSIGN {",$sim,".",$actual,"}"," ",$level,";\n";
        }else{
            print OUTPUT $_;
        }
    }

