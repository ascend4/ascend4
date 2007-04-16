<?php

require_once("ashighlight.class.php");

$text = "MODEL test; END test;";
$lang = "a4c";

$p = new ASHighlight();

$p->parse_code($text,$lang);

if($p->error){
	print "parse_code returns error:";
	die($p->errmsg);
}

print "output...\n";
print $p->out;
print "...output\n";

?>
