#!/usr/local/bin/perl

# Created 12/96 by Chad Farschman, Mark Thomas
# the input to the program comes from the command line
# the first argument after the executable is the name of the 
# file you want to parse. This is a file generated from 
# ASCEND4 using the dbg_write_slv0_sys filename command.
# Here filename is the name of the ascend output. 
#
# The name of the gams output is obtained by munging the
# name of the input.
#
# $Revision: 1.4 $
# $Author: ballan $
# $Date: 1997/07/05 14:11:59 $
#

$filename = shift(@ARGV);

# this may not be very robust (n != 1 '.' in a name may mess it up)
# split the list filename into the base and its extension
($base,$extension) = split(/\./,$filename);

# concatenate the base with the extension .con
$outfile = $base."\.con";

# concatenate the base with the extension .lk
$lookup = $base."\.lk";

open(OUTPUT, ">$outfile");
open(INPUT, "$filename");

# the following are GAMS reserved syntax words in equations
$reserved{"abs"} = 1;
$reserved{"exp"} = 1;
$reserved{"log"} = 1;
$reserved{"sqrt"} = 1;
$reserved{"sqr"} = 1;
$reserved{"if"} = 1;
$reserved{"na"} = 1;
$reserved{"ne"} = 1;
$reserved{"no"} = 1;

# the following words are also reserved in gams input, though
# we don't know if we need to treat them specially yet.
#
# These are reserved statement starters:
# ABORT ASSIGN EQUATION[i] INTEGER OPTION[i] SET[i] SOS2
# ACRONYM[i] BINARY FREE LOOP PARAMETER[i] SOLVE TABLE
# ALIAS DISPLAY MODEL[i] POSITIVE SOS1 VARIABLE[i]
# NEGATIVE SCALAR[i]
#
# Comma is also reserved in gams in some contexts.
#
# Other words gams reserves:
# ACRONYMS ALL AND CARD DISPLAY EPS EQ GE GT INF LE LT MAXIMIZING
# MINIMIZING MODELS NOT OR ORD PARAMETERS PROD SETS SMAX SMIN
# USING VARIABLES XOR YES
# =L= .. =G= =E= =N= -- ++ **


# the following are direct conversion
$name{"ln"} = "log";
$name{"="} = ")=e=(";

# initialize the following variables (counters)
$solve = 0;
$variables = 0;
$parameters = 0;
$objective = 0;
$bounds = 0;
$relation = 0;
$linenum = 0;
$parameternum = 0;
$equationnum = 0;
$negcount = 0;
$poscount = 0;
$count = 0;
$fix = 0;
$paracount = 0;
$numvariables = 0;
$relnomcount = 0;
$previousterm = "";

# the following is header information for the gams input file
print OUTPUT "\$TITLE	\n";
print OUTPUT "\$inlinecom /* */\n";
print OUTPUT "\n";
print OUTPUT "/* Turn off the listing of the input file */\n";
print OUTPUT "\$offlisting\n";
print OUTPUT "\n";
print OUTPUT "/* Turn off the listing and cross-reference of the symbols used */\n";
print OUTPUT "\$offsymxref offsymlist\n";
print OUTPUT "\n";
print OUTPUT "option\n";
print OUTPUT "	limrow = 0,		/* equations listed per block */\n";
print OUTPUT "	limcol = 0,		/* variables listed per block */\n";
print OUTPUT "	solprint = on,		/* solver's solution output printed */\n";
print OUTPUT "	sysout = off;		/* solver's system output printed */\n";
print OUTPUT "\n";

# begin counting variables at the letter a
$n = "a";

while (<INPUT>) {
	$linenum = $linenum + 1;

# set some switches so you know where you are in the file you are parsing
	if (/^Solver: (.*)/){
		$solve = 1;
	}
	if (/^Variables: (.*)/){
		$solve = 0;
		$variables = 1;
	}
	if (/^Parameters: (.*)/){
		$variables = 0;
		$parameters = 1;
	}
	if (/^Objective: (.*)/){
		$parameters = 0;
		$objective = 1;
	}
	if (/^Boundaries: (.*)/){
		$objective = 0;
		$bounds = 1;
	}
	if (/^Relations: (.*)/){
		$bounds = 0;
		$relation = 1;
	}

# split up the table of information given and output that information in the standard gams format
# this information will go at the end of a gams file
# should only do this for information between Variables and Objective headers

	if(($variables == 1) && ($linenum >= 5)){
	    $numvariables = $variables + 1;
		if(length($_) > 1){
			($variable, $value, $nominal, $lower, $upper, $fixed) = split(' ', $_);

# the following line deletes the character " and replaces it with nothing
			$variable =~ s/\"//g;

			$name{$variable} = $n++;
			if($reserved{$n}){
			    $n++;
			}

			if($upper >= 1e+20){
			    $upper = 1e+20;
			}
			if($lower <= -1e+20){
			    $lower = -1e+20;
			}

			if($fixed == 0){

# if the lower bound of the variable is 0, then the variable is positive
				if($lower == 0){
					$poscount = $poscount + 1;
					@posvararray = (@posvararray, $name{$variable});
					@posvalarray = (@posvalarray, $value);
					@posloarray = (@posloarray, $lower);
					@posuparray = (@posuparray, $upper);

# if the upper bound of the variable is 0, then the variable is negative
				}elsif($upper == 0){
					$negcount = $negcount + 1;
					@negvararray = (@negvararray, $name{$variable});
					@negvalarray = (@negvalarray, $value);
					@negloarray = (@negloarray, $lower);
					@neguparray = (@neguparray, $upper);

# if the above two don't apply, the variable is neither
				}else{
					$count = $count + 1;
					@vararray = (@vararray, $name{$variable});
					@valarray = (@valarray, $value);
					@loarray = (@loarray, $lower);
					@uparray = (@uparray, $upper);
				}
			}else{

# the variable is fixed
				$fix = $fix + 1;
				@fixarray = (@fixarray, $name{$variable});
				@fixvalarray = (@fixvalarray, $value);
			}
		}
	}
# if the keyword ITERATIONS is found, we are at the end of the file
# (essentially). Turn the variable RELATION off.  Thus, the rest of 
# the file will be ignored.
	if(/^Iterations: (.*)/){
		$relation = 0;
	}

# if we are in the parameters subsection
	if($parameters == 1){
	    if($parameternum <= 1){
		$parameternum = $parameternum + 1;
	    }else{
		if(length($_) > 1){
		    ($parameter, $value) = split(' ', $_);

# the following line deletes the character " and replaces it with nothing
		    $parameter =~ s/\"//g;

		    $name{$parameter} = $n++;
		    if($reserved{$n}){
			$n++;
		    }
		    $paracount = $paracount + 1;
		    @paraarray = (@paraarray, $name{$parameter});
		    @paravalarray = (@paravalarray, $value);
		}
	    }
	}

# if we are in the relations subsection
	if($relation == 1){
	    if($equationnum == 0){
		$equationnum = $equationnum + 1;
	    }else{

# if the NOMINAL keyword is found
		if(/^.*Nominal: (.*)/){
		    chop($_);
		    ($extra1,$extra2,$relnom) = split(' ',$_);
		    $relnomcount = $relnomcount + 1;
		    @relnomarray = (@relnomarray, $relnom);
		}else{


# if the CONDITIONS keyword is found, don't print anything.
		if(/^.*Conditions: (.*)/){

# if the CONDITIONS keyword isn't found, we are at an equation.
		}else{


# following line removes the return character from the line
			chop($_);

# the following line removes the " from the equation
			$_ =~ s/\"//g;

# the following line replaces variables such as -d.flash.x
# with -1 * d.flash.x. The variables is not recognizable
# otherwise.
			$_ =~ s/-([a-zA-Z])/-1 \* $1/g;

# the following code replaces abs with sqrt(sqr)
			while(/\A(.*\b(abs)\s*)\((.*)\Z/s){
			    $_ = $1 . 'sqrt(sqr(';
			    $operator = $2;
			    $balance_me = $3;
			    $balance = 1;

			    foreach  $c ( split( /([\(\)])/, $balance_me ) ) {
				if ( $balance == 0 ) {
				    $_ .= $c;
				}
				elsif ( $c eq '(' ) {
				    $balance++;
				    $_ .= $c;
				}
				elsif (( $c eq ')' ) && ( --$balance == 0 )) {
				    $_ .= '))';
				}
				else {
				    $_ .= $c;
				}
			    }
			$_ =~ s/abssqrt\(sqr\(/sqrt\(sqr\(/;
			}

# end the code to replace abs with sqrt(sqr)

# this breaks up an equation into its respective parts and 
# evaluates each part (replacing and deleting as necessary).

			@term = split(/([\s\(\)])/,$_);
			$termcount = 0;
			foreach $term (@term){
			    $termcount = $termcount + 1;

# the following line replaces ^ with ** for power functions
			    $term =~ s/\^/\**/g;

# the following line replaces terms such as -1 with (-1).
# this prevents problems with double operators such as
# flash.x + -flash.y = 1
			    $term =~ s/\s-\w/\s\(-\w\)/g;
			    
			    if($name{$term} ne ""){
				$term = $name{$term};
			    }
			}

# before the terms are joined back together, determine the string
# length and join so that the equation printed to each line is no
# more than 100 characters.
			$totlen = 0;
			for($i=1; $i <= $termcount; $i++){

# if the length of the string is zero, we are at the beginning of equation
			    if($totlen == 0){
				$totlen = length($term[$i - 1]);
				if($term[$i - 1] < 0){
				    $_ = "(".$term[$i - 1].")";
				}else{
				    $_ = $term[$i - 1];
				}
			    }else{

# if the length of the current string plus the new string is greater than 100
# we will not add the two string directly.  Instead we will add a carriage
# return between the strings and reset the length counter to the length of
# the present string
				$totlen = $totlen + length($term[$i-1]);
				if($totlen < 100){
				    if($term[$i - 1] < 0){
					$_ = $_."(".$term[$i - 1].")";
				    }else{
					$_ = $_.$term[$i-1];
				    }
				}else{
				    if($term[$i - 1] < 0){
					$_ = $_."\n\t"."(".$term[$i - 1].")";
				    }else{
					$_ = $_."\n\t".$term[$i-1];
				    }
				    $totlen = length($term[$i-1]);
				}
			    }
			}
		       	@eqnarray = (@eqnarray, $_);
# increment the equation counter
			$equationnum = $equationnum + 1;

		    }
	    }
	    }
	}
    }

# print the output in gams format
if($paracount > 0){
print OUTPUT "\nparameters\n";
for($i=1; $i <= $paracount; $i++){
	if($i == $paracount){
		print OUTPUT $paraarray[$i - 1],";\n";
	}else{
		print OUTPUT $paraarray[$i - 1],"\n";
	}
}
}
if($poscount > 0){
print OUTPUT "\npositive variables\n";
for($i=1; $i <= $poscount; $i++){
	if($i == $poscount){
		print OUTPUT $posvararray[$i - 1],";\n";
	}else{
		print OUTPUT $posvararray[$i - 1],"\n";
	}
}
}
if($negcount > 0){
print OUTPUT "\nnegative variables\n";
for($i=1; $i <= $negcount; $i++){
	if($i == $negcount){
		print OUTPUT $negvararray[$i - 1],";\n";
	}else{
		print OUTPUT $negvararray[$i - 1],"\n";
	}
}
}
if($count > 0){
print OUTPUT "\nvariables\n";
for($i=1; $i <= $count; $i++){
	print OUTPUT $vararray[$i - 1],"\n";
}
}
if($fix > 0){
for($i=1; $i <= $fix; $i++){
	print OUTPUT $fixarray[$i - 1],"\n";
}
}
print OUTPUT "z1;\n";

print OUTPUT "\nequations\n";
for($i = 1; $i<$equationnum; $i++){
	print OUTPUT "eqn",$i,"	\n";
}

print OUTPUT "obj1	;\n\n";

for($i = 0; $i<$equationnum-1; $i++){ 
	print OUTPUT "eqn",$i+1,"..	","1/",$relnomarray[$i],"*(",$eqnarray[$i],")/",$relnomarray[$i],";\n\n";
}


print OUTPUT "obj1..	z1 =e= ;\n";
print OUTPUT "\n";
print OUTPUT "Model unitop /all/;\n";
print OUTPUT "\n";

print OUTPUT "unitop.SCALEOPT = 1;\n";
print OUTPUT "unitop.OPTFILE = 1;\n";
print OUTPUT "\n";

for($i = 0; $i< $poscount; $i++){
#	print OUTPUT $posvararray[$i], ".lo=",$posloarray[$i],";\n";
	print OUTPUT $posvararray[$i], ".up=",$posuparray[$i],";\n";
	print OUTPUT $posvararray[$i], ".l=",$posvalarray[$i],";\n";
}

for($i = 0; $i< $negcount; $i++){
	print OUTPUT $negvararray[$i], ".lo=",$negloarray[$i],";\n";
#	print OUTPUT $negvararray[$i], ".up=",$neguparray[$i],";\n";
	print OUTPUT $negvararray[$i], ".l=",$negvalarray[$i],";\n";
}

for($i = 0; $i< $count; $i++){
	print OUTPUT $vararray[$i], ".lo=",$loarray[$i],";\n";
	print OUTPUT $vararray[$i], ".up=",$uparray[$i],";\n";
	print OUTPUT $vararray[$i], ".l=",$valarray[$i],";\n";
}

for($i = 0; $i< $fix; $i++){
	print OUTPUT $fixarray[$i], ".fx=",$fixvalarray[$i],";\n";
}

for($i = 0; $i< $paracount; $i++){
	print OUTPUT $paraarray[$i], "=",$paravalarray[$i],";\n";
}

print OUTPUT "\nSolve unitop using nlp minimizing z1;\n";

# need to produce a look-up table for the variables
open(TABLE, ">$lookup");
foreach $name(%name){
    print TABLE $name,"\n";
}
