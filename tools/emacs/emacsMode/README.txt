The two files in this directory provide an ASCEND mode to the emacs
editor.  The .elc file is a "compiled" version of the .el file.  Place
them into the lisp or elisp directory for your emacs installation.
These work on unix, linux and windows.

This mode automatically turns on for any file having the ASCEND4
extensions a4c and a4l.

Among other things, the ASCEND mode will automatically translate any
of the following aliases into the first shown (e.g., the mode replaces
"ats" with ARE_THE_SAME or "add" with ADD).  In addition it will
attempt to indent lines properly when you complete them with a
carriage return.

---

Aliases this mode translates - 

     ("ADD" "add")
     ("ALIASES" "aliases" "alii" "al")
     ("AND" "and")
     ("ANY" "any")
     ("ARE_ALIKE" "are_alike" "arealike" "aa")
     ("ARE_THE_SAME" "are_the_same" "arethesame" "ats")
     ("ARRAY" "array")
     ("ATOM" "atom")
     ("BREAK" "break")
     ("CALL" "call")
     ("CARD" "card")
     ("CASE" "case")
     ("CHOICE" "choice")
     ("CONDITIONAL" "conditional")
     ("CONSTANT" "constant" "const")
     ("CONTINUE" "continue")
     ("CREATE" "create")
     ("DATA" "data")
     ("DECREASING" "descreasing")
     ("DEFAULT" "default")
     ("DEFINITION" "definition")
     ("DIMENSION" "dimension")
     ("DIMENSIONLESS" "dimensionless")
     ("DO" "do")
     ("ELSE" "else")
     ("END" "end")
     ("EXTERNAL" "external")
     ("FALSE" "false")
     ("FALL_THROUGH" "fall_through" "fallthrough" "fall")
     ("FOR" "for")
     ("FROM" "from")
     ("GLOBAL" "global")
     ("IF" "if")
     ("IMPORT" "import")
     ("IN" "in")
     ("INCREASING" "increasing")
     ("INPUT" "input")
     ("INTERACTIVE" "interactive")
     ("INTERSECTION" "intersection")
     ("IS_A" "is_a" "isa")
     ("IS_REFINED_TO" "is_refined_to" "isrefinedto" "irt")
     ("MAXIMIZE" "maximize" "max")
     ("MAX_INTEGER" "max_integer" "maxinteger" "maxint")
     ("MAX_REAL" "max_real" "maxreal")
     ("METHOD" "method")
     ("METHODS" "methods")
     ("MINIMIZE" "minimize" "min")
     ("MODEL" "model")
     ("NOT" "not")
     ("NOTES" "notes")
     ("OF" "of")
     ("OR" "or")
     ("OTHERWISE" "otherwise")
     ("OUTPUT" "output")
     ("PROD" "prod")
     ("PROVIDE" "provide")
     ("REFINES" "refines")
     ("REPLACE" "replace")
     ("REQUIRE" "require")
     ("RETURN" "return")
     ("RUN" "run")
     ("SATISFIED" "satisfied")
     ("SELECT" "select")
     ("SIZE" "size")
     ("STOP" "stop")
     ("SUCH_THAT" "such_that" "suchthat")
     ("SUM" "sum")
     ("SWITCH" "switch")
     ("THEN" "then")
     ("TRUE" "true")
     ("UNION" "union")
     ("UNITS" "units")
     ("UNIVERSAL" "universal")
     ("USE" "use")
     ("VALUE" "value")
     ("WHEN" "when")
     ("WHERE" "where")
     ("WHILE" "while")
     ("WILL_BE" "will_be" "willbe" "wb")
     ("WILL_BE_THE_SAME" "willbethesame" "wbts")
     ("WILL_NOT_BE_THE_SAME" "willnotbethesame" "wnbts")
     ("WITH" "with")
     ("WITH_VALUE" "with_value" "withvalue" "wv")

					 
