%{
  open Types
%}

(*
 * This parser builds a syntax tree for the ASCEND language. This is something that the C parser
 * does NOT do, since it does a lot of processing as it goes along. My goal is to have a syntax tree
 * that can be serialized back to an equivalent ASCEND program (modulo whitespace) and to keep the
 * parser and data structures as possible.
 *
 * I based this parser on the Bison parser from ascParse.y but changed a lot of stuff. For example:
 *
 *   - Sometimes I use menhir's standard library for lissts, optional tokens, etc
 *
 *   - Menhir does not hav ea stack-size limit so we can build lists directly,
 *     without needing the left-recursion + reversal trick.
 *      
 *   - Some rules (like the _head) ones have been inlined. Sometimes they were only separated in the
 *     Bison grammar because their semantic actions would have global side-effects but here we are
 *     just building a syntax tree, without any side-effects.
 *      
 *   - Removed some rules that were only there to give good error messages. This grammar is
 *     intended for language experimentation so simplicity is more important than error handling.
 *      
 *   - Reordered some rules (made things alphabetical, etc)
 *)


(* Basic tokens. The unused tokens are commented out, stop Menhir from complaining *)
%token ADD ALIASES AND ANY AREALIKE ARETHESAME (*ARRAY*) ASSERT ATOM
%token BEQ BNE BREAK
%token CALL CARD CASE CHOICE CHECK CHILDREN CONDITIONAL CONSTANT
%token CONTINUE CREATE
%token DATA DECREASING DEFAULT DEFINITION DER DIMENSION DERIV DERIVATIVE
%token DIMENSIONLESS DO
%token ELSE END EVENT EXPECT EXTERNAL
%token FALSE FALLTHRU FIX FOR FREE FROM
%token GLOBAL
%token IF  IGNORE IMPORT IN INPUT INCREASING (*INTERACTIVE*) INDEPENDENT
%token INTERSECTION ISA IS ISREFINEDTO
%token LIKE LINK
%token MAXIMIZE MAXINTEGER MAXREAL METHODS METHOD MINIMIZE MODEL
%token NOT NOTES
%token OF OPTION OR OTHERWISE OUTPUT
%token PATCH PRE PREVIOUS PROD PROVIDE
%token REFINES REPLACE REQUIRE RETURN RUN
%token SATISFIED SELECT (*SIZE*) SOLVE SOLVER STOP SUCHTHAT SUM SWITCH
%token THEN TRUE
%token UNION UNITS UNIVERSAL UNLINK
%token WHEN WHERE WHILE WILLBE WILLBETHESAME WILLNOTBETHESAME
%token ASSIGN CASSIGN DBLCOLON USE LEQ GEQ NEQ
%token DOTDOT WITH (*VALUE*) WITH_VALUE

(* Tokens with values *)
%token <float> REAL
%token <int> INTEGER
%token <Types.identifier> IDENTIFIER
%token <Types.bracedtext> BRACEDTEXT
%token <Types.symbol> SYMBOL
%token <Types.dquote> DQUOTE

(* The following Single-character tokens are implicit in yacc-land but
 * we have to declare them in Ocaml *)
%token LPAREN RPAREN
%token LBRACKET RBRACKET
%token COLON SEMICOLON
%token DOT COMMA
%token PLUS MINUS TIMES DIV
%token CIRCUMFLEX
%token PIPE
%token EQ LT GT

(* Menhir likes when there is a token for the end of file: *)
%token EOF


(* Associativities,types, etc *)
%left PIPE SUCHTHAT
%left BEQ BNE
%left AND OR IN
%left LT EQ GT LEQ GEQ NEQ
%left PLUS MINUS
%left DIV TIMES
%nonassoc UMINUS UPLUS
%right CIRCUMFLEX
%left NOT

%type <Types.program> start
%type <Types.definition> definition
%type <Types.model_refines option> optional_model_refines
%type <Types.atomic_constant> atomic_constant
%type <Types.dimension> dimension
%type <Types.dimexpr> dimexpr
%type <Types.fraction> fraction
%type <Types.fraction_body> fraction_body
%type <Types.fname list> fnames
%type <Types.fname> fname
%type <Types.name> name
%type <Types.note> note
%type <Types.number> number
%type <Types.number_sign> optional_number_sign
%type <Types.number_literal> number_literal
%type <Types.unit_def> unit_declaration
%type <Types.procedure> procedure
%type <Types.set> set
%type <Types.statement> statement
%type <Types.assignment> assignment
%type <Types.for_action> for_action
%type <Types.for_direction> for_direction
%type <Types.jump> jump
%type <Types.relation> relation
%type <Types.switch_block list> switch_blocks
%type <Types.switch_block> switch_block
%type <Types.type_expr> type_expr
%type <Types.expr> expr

%start start

%%

start: 
    | definitions EOF { $1 }


(*
 * TOP LEVEL DEFINITIONS
 *)

definitions:
    | (**)                   { [] }
    | definition definitions { $1 :: $2 }

definition:
    | require_file_def   { $1 }
    | provide_module_def { $1 }
    | import_def         { $1 }
    | add_method_def     { $1 }
    | replace_method_def { $1 }
    | add_notes_def      { $1 }
    | constant_def       { $1 }
    | atom_def           { $1 }
    | model_def          { $1 }
    | definition_def     { $1 }
    | patch_def          { $1 }
    | units_def          { $1 }
    | global_def         { $1 }

require_file_def: 
    | REQUIRE DQUOTE SEMICOLON { Def_require_file($2) }

provide_module_def: 
    | PROVIDE DQUOTE SEMICOLON { Def_provide_module($2) }

import_def: 
    | IMPORT IDENTIFIER FROM DQUOTE SEMICOLON { Def_import(Some($2), $4) }
    | IMPORT                 DQUOTE SEMICOLON { Def_import(None    , $2) }

add_method_def: 
    | ADD METHODS IN model_modification_location SEMICOLON procedures END METHODS SEMICOLON
      { Def_add_method($4, $6) }

replace_method_def: 
    | REPLACE METHODS IN model_modification_location SEMICOLON procedures END METHODS SEMICOLON
      { Def_replace_method($4, $6)}

add_notes_def: 
    | ADD NOTES IN IDENTIFIER optional_method SEMICOLON notes_body END NOTES SEMICOLON
      { Def_add_notes($4, $5, $7)}

constant_def: 
    | optional_universal CONSTANT IDENTIFIER REFINES IDENTIFIER
      dimension
      optional_constant_val
      optional_comment
      SEMICOLON
      { Def_constant($1, $3, $5, $6,$7, $8) }

atom_def: 
    | optional_universal ATOM IDENTIFIER REFINES IDENTIFIER
      dimension
      optional_default_val
      optional_like_children
      SEMICOLON
      statements
      optional_methods
      END IDENTIFIER SEMICOLON
      { Def_atom($1, $3, $5, $6, $7, $8, $10, $11) }

model_def: 
    | optional_universal MODEL IDENTIFIER
      optional_model_parameters
      optional_model_parameter_wheres
      optional_model_refines
      SEMICOLON
      statements
      optional_methods
      END IDENTIFIER SEMICOLON
      { Def_model($1,$3,$4,$5,$6,$8,$9) }

definition_def: 
    | DEFINITION IDENTIFIER
      statements
      optional_methods
      END IDENTIFIER SEMICOLON
      { (* TODO check $2,$6 *)
        Def_definition($2,$3,$4) }

patch_def: 
    | PATCH IDENTIFIER FOR IDENTIFIER SEMICOLON
      statements
      optional_methods
      END IDENTIFIER SEMICOLON
      { (*TODO check $2,$9 -- look are ascParse.y to make sure its not $4 *) 
        Def_patch($2,$4,$6,$7) }

units_def: 
    | units_block SEMICOLON { Def_units($1) }

global_def: 
    | GLOBAL SEMICOLON
      statements
      END GLOBAL SEMICOLON { Def_global($3) }


(* 
 * Top-level helpers
 *)


model_modification_location:
    | IDENTIFIER       { Model_var($1) }
    | DEFINITION MODEL { Model_def }

optional_constant_val:
    | (**)                    { None }
    | CASSIGN atomic_constant { Some($2) }

optional_default_val:
    | (**)                    { None }
    | DEFAULT atomic_constant { Some($2) }

optional_like_children:
    | (**)                 { None }
    | LIKE CHILDREN fnames { Some($3) }

optional_method: 
    | (**)              { None }
    | METHOD IDENTIFIER { Some($2) }

optional_methods: 
    | (**)                    { None }
    | METHODS list(procedure) { Some($2) }

optional_model_parameters: 
    | (**)                     { None }
    | LPAREN statements RPAREN { Some($2) }

optional_model_parameter_wheres: 
    | (**)                           { None }
    | WHERE LPAREN statements RPAREN { Some($3) }

optional_model_refines:
    | (**)                                        { None }
    | REFINES IDENTIFIER                          { Some(Model_Refines($2,None)) }
    | REFINES IDENTIFIER LPAREN statements RPAREN { Some(Model_Refines($2,Some($4))) }

optional_universal: 
    | (**)      { false }
    | UNIVERSAL { true  }


(*
 * ATOMIC CONSTANT
 *)

atomic_constant:
    | optional_number_sign number { Atom_number($1,$2) }
    | TRUE                        { Atom_bool(true) }
    | FALSE                       { Atom_bool(false) }
    | SYMBOL                      { Atom_symbol($1) }

(*
 * DIMENSIONS
 *)

dimension:
    | (**)              { Dim_default }
    | DIMENSION TIMES   { Dim_wildcard }
    | DIMENSION dimexpr { Dim_dimexpr($2) }
    | DIMENSIONLESS     { Dim_dimensionless }

dimexpr: 
    | IDENTIFIER                  { Dimex_var($1) }
    | INTEGER                     { Dimex_int($1) }
    | dimexpr DIV dimexpr         { Dimex_div($1,$3) }
    | dimexpr TIMES dimexpr       { Dimex_mul($1,$3) }
    | dimexpr CIRCUMFLEX fraction { Dimex_pow($1,$3) }
    | LPAREN dimexpr RPAREN       { Dimex_paren($2) }

fraction: 
    | optional_number_sign fraction_body { Fraction($1,$2) }

fraction_body: 
    | INTEGER                           { Frac_int($1) }
    | LPAREN INTEGER DIV INTEGER RPAREN { Frac_frac($2,$4) }

(*
 * NAMES
 *)

fnames:
    | fname              { [$1] }
    | fname COMMA fnames { $1 :: $3 }

fname: 
    | name optional_comment { ($1,$2) }

name: 
    | IDENTIFIER                 { Name_id($1) }
    | name DOT IDENTIFIER        { Name_dot($1,$3) }
    | name LBRACKET set RBRACKET { Name_set($1,$3) }
    | DERIV LPAREN fnames RPAREN { Name_der($3) }
    | PRE LPAREN fname RPAREN    { Name_pre($3) }

optional_comment: 
    | (**)   { None }
    | DQUOTE { Some($1) }

(*
 * NOTES
 *)

notes_body: 
    | list(note) { $1 }

note:
    | SYMBOL list(note_value) { Note($1,$2) }

note_value: 
    | fnames BRACEDTEXT { ($1,$2) }

(*
 * NUMBERS
 *)

number: 
    | number_literal option(BRACEDTEXT) { Number($1,$2) }

number_literal:
    | INTEGER { Num_integer($1) }
    | REAL    { Num_real($1) }

optional_number_sign:
    | (**)  { Num_nosign }
    | PLUS  { Num_pos }
    | MINUS { Num_neg }


(*
 * UNITS
 *)

units_block: 
    | UNITS list(unit_declaration) END UNITS { $2 }

unit_declaration: 
    | IDENTIFIER EQ BRACEDTEXT SEMICOLON { Unit($1,$3) }

(*
 * PROCEDURES
 *)

procedures:
    | (**)                 { [] }
    | procedure procedures { $1::$2 }

procedure: 
    | METHOD IDENTIFIER SEMICOLON
      statements
      END IDENTIFIER SEMICOLON
      { Method($2,$4) }

(*
 * SETS
 *)

set: 
    | (**)               { [] }
    | set_item           { [$1] }
    | set_item COMMA set { $1 :: $3 }

set_item: 
    | expr             { Set_singleton($1) }
    | expr DOTDOT expr { Set_range($1,$3) }

(*
 * STATEMENTS
 *) 

statements:
    | (**)                           { [] }
    | statement SEMICOLON statements { $1::$3}

statement: 
    | aliases_statement          { $1 }
    | arealike_statement         { $1 }
    | arethesame_statement       { $1 }
    | assert_statement           { $1 }
    | assignment_statement       { $1 }
    | blackbox_statement         { $1 }
    | call_statement             { $1 }
    | conditional_statement      { $1 }
    | der_statement              { $1 }
    | derivative_statement       { $1 }
    | event_statement            { $1 }
    | external_statement         { $1 }
    | fix_and_assign_statement   { $1 }
    | fix_statement              { $1 }
    | flow_statement             { $1 }
    | for_statement              { $1 }
    | free_statement             { $1 }
    | glassbox_statement         { $1 }
    | if_statement               { $1 }
    | independent_statement      { $1 }
    | is_statement               { $1 }
    | isa_statement              { $1 }
    | isrefinedto_statement      { $1 }
    | link_statement             { $1 }
    | notes_statement            { $1 }
    | option_statement           { $1 }
    | previous_statement         { $1 }
    | relation_statement         { $1 }
    | run_statement              { $1 }
    | select_statement           { $1 }
    | solve_statement            { $1 }
    | solver_statement           { $1 }
    | switch_statement           { $1 }
    | units_statement            { $1 }
    | unlink_statement           { $1 }
    | use_statement              { $1 }
    | when_statement             { $1 }
    | while_statement            { $1 }
    | willbe_statement           { $1 }
    | willbethesame_statement    { $1 }
    | willnotbethesame_statement { $1 }

aliases_statement: 
    | fnames ALIASES fname
      { Stat_aliases_one($1,$3) }
    | fnames ALIASES LPAREN fnames RPAREN
      WHERE fnames ISA IDENTIFIER OF IDENTIFIER optional_set_values
      { Stat_aliases_many($1,$4,$7,$9,$11,$12) }

arealike_statement: 
    | fnames AREALIKE { Stat_arealike($1) }

arethesame_statement: 
    | fnames ARETHESAME { Stat_arethesame($1) }

assert_statement: 
    | ASSERT expr { Stat_assert($2) }

assignment_statement: 
    | assignment { Stat_assign($1) }

blackbox_statement: 
    | label IDENTIFIER LPAREN input_args SEMICOLON output_args optional_data_arg RPAREN
      { Stat_blackbox($1,$2,$4,$6,$7) }

call_statement: 
    | CALL IDENTIFIER                   { Stat_call($2,None) }
    | CALL IDENTIFIER LPAREN set RPAREN { Stat_call($2,Some($4)) }

conditional_statement: 
    | CONDITIONAL statements END CONDITIONAL { Stat_conditional($2) }

der_statement: 
    | DER LPAREN fnames RPAREN { Stat_der($3) }

derivative_statement: 
    | DERIVATIVE OF fnames            { Stat_derivative($3, None) }
    | DERIVATIVE OF fnames WITH fname { Stat_derivative($3, Some($5)) }

event_statement: 
    | optional_label EVENT LPAREN fnames RPAREN switch_blocks END EVENT
      { Stat_event($1, $4, $6) }

external_statement: 
    | EXTERNAL IDENTIFIER LPAREN fnames RPAREN
      { Stat_external($2,$4) }

fix_and_assign_statement: 
    | FIX assignment { Stat_fix_and_assign($2) }

fix_statement: 
    | FIX fnames { Stat_fix($2) }

flow_statement: 
    | jump { Stat_flow($1) }

for_statement: 
    | FOR IDENTIFIER IN expr option(for_direction) for_action
      statements
      END FOR
      { Stat_for($2,$4,$5,$6,$7) }

free_statement: 
    | FREE fnames { Stat_free($2) }

glassbox_statement: 
    | label IDENTIFIER LPAREN fnames SEMICOLON INTEGER RPAREN optional_scope
    { Stat_glassbox ($1,$2,$4,$6,$8) }

if_statement: 
    | IF expr THEN statements END IF                 { Stat_if($2,$4) }
    | IF expr THEN statements ELSE statements END IF { Stat_if_else($2,$4,$6) }

independent_statement: 
    | INDEPENDENT fnames { Stat_independent($2) }

is_statement: 
    | fnames IS IDENTIFIER optional_of
      { Stat_is($1,$3,$4) }

isa_statement:
    | fnames ISA type_expr optional_of optional_with_value
      { Stat_isa($1,$3,$4,$5) }

isrefinedto_statement: 
    | fnames ISREFINEDTO type_expr { Stat_isrefinedto($1,$3) }

link_statement: 
    | LINK LPAREN IGNORE COMMA SYMBOL COMMA fnames RPAREN { Stat_link_ignore($5,$7) }
    | LINK LPAREN              SYMBOL COMMA fnames RPAREN { Stat_link_symbol($3,$5) }
    | LINK LPAREN              fname  COMMA fnames RPAREN { Stat_link_name($3,$5) }

notes_statement: 
    | NOTES notes_body END NOTES { Stat_notes($2) }

option_statement: 
    | OPTION IDENTIFIER expr { Stat_option($2,$3) }

previous_statement: 
    | PREVIOUS fnames { Stat_previous($2) }

relation_statement: 
    | optional_label relation { Stat_relation($1,$2) }

run_statement: 
    | RUN run_target { Stat_run($2) }

select_statement: 
    | SELECT LPAREN fnames RPAREN switch_blocks END SELECT { Stat_select($3,$5) }

solve_statement: 
    | SOLVE { Stat_solve }

solver_statement: 
    | SOLVER IDENTIFIER { Stat_solver($2) }

switch_statement: 
    | SWITCH LPAREN fnames RPAREN switch_blocks END SWITCH { Stat_switch($3,$5) }

units_statement: 
    | units_block { Stat_units($1) }

unlink_statement: 
    | UNLINK LPAREN SYMBOL COMMA fnames RPAREN { Stat_unlink_symbol($3,$5) }
    | UNLINK LPAREN fname  COMMA fnames RPAREN { Stat_unlink_name($3,$5) }

use_statement: 
    | USE fname { Stat_use($2) }

when_statement: 
    | optional_label WHEN LPAREN fnames RPAREN switch_blocks END WHEN { Stat_when($1,$4,$6) }

while_statement: 
    | WHILE expr DO statements END WHILE { Stat_while($2,$4) }

willbe_statement: 
    | fnames WILLBE type_expr optional_of optional_with_value { Stat_willbe($1,$3,$4,$5) }

willbethesame_statement: 
    | fnames WILLBETHESAME { Stat_willbethesame($1) }

willnotbethesame_statement: 
    | fnames WILLNOTBETHESAME { Stat_willnotbethesame($1) }

(*
 * Statement helpers
 *)

label:
    | fname COLON { $1 }

%inline
optional_label:
    | (**)  { None }
    | label { Some($1) }

assignment:
    | fname ASSIGN  expr { Assign_var($1,$3) }
    | fname CASSIGN expr { Assign_const($1,$3) }

for_action: 
    | CHECK  { For_check }
    | CREATE { For_create }
    | DO     { For_do }
    | EXPECT { For_expect }

for_direction:
    | INCREASING { For_increasing }
    | DECREASING { For_decreasing }

input_args: 
    | fnames COLON INPUT { $1 }

jump:
    | BREAK                   { Jump_break    }
    | CONTINUE                { Jump_continue }
    | FALLTHRU                { Jump_fallthru }
    | RETURN                  { Jump_return   }
    | STOP option(BRACEDTEXT) { Jump_stop($2) }

relation: 
    | expr          { Rel_expr($1) }
    | MAXIMIZE expr { Rel_max($2) }
    | MINIMIZE expr { Rel_min($2) }

run_target:
    | fname                { Run_public($1) }
    | fname DBLCOLON fname { Run_hidden($1,$3) }

switch_blocks:
    | (**)                       { [] }
    | switch_block switch_blocks { $1 :: $2 }

switch_block:
    | CASE set COLON statements  { Switch_case($2,$4) }
    | OTHERWISE COLON statements { Switch_otherwise($3) }

optional_data_arg: 
    | (**)                       { None }
    | SEMICOLON fname COLON DATA { Some($2) }

optional_of: 
    | (**)          { None }
    | OF IDENTIFIER { Some($2) }

optional_scope: 
    | (**)     { None }
    | IN fname { Some($2) }

optional_set_values: 
    | (**)                         { None }
    | WITH_VALUE LPAREN set RPAREN { Some($3) }

optional_with_value: 
    | (**)            { None }
    | WITH_VALUE expr { Some($2) }

output_args: 
    | fnames COLON OUTPUT {$1}

type_expr: 
    | IDENTIFIER                   { TE_var($1) }
    | IDENTIFIER LPAREN set RPAREN { TE_call($1,$3) }


(*
 * EXPRESSIONS
 *)


expr: 
    | number                                     { E_number($1) }
    | MAXINTEGER                                 { E_maxinteger }
    | MAXREAL                                    { E_maxreal }
    | TRUE                                       { E_boolean(true) }
    | FALSE                                      { E_boolean(false) }
    | ANY                                        { E_any }
    | SYMBOL                                     { E_symbol($1) }
    | fname                                      { E_var($1) }
    | LBRACKET set RBRACKET                      { E_bracket($2) }
    | expr PLUS expr                             { E_binop(Bin_add, $1, $3) }
    | expr MINUS expr                            { E_binop(Bin_sub, $1, $3) }
    | expr TIMES expr                            { E_binop(Bin_mul, $1, $3) }
    | expr DIV expr                              { E_binop(Bin_div, $1, $3) }
    | expr CIRCUMFLEX expr                       { E_binop(Bin_pow, $1, $3) }
    | expr AND expr                              { E_binop(Bin_and, $1, $3) }
    | expr OR expr                               { E_binop(Bin_or, $1, $3) }
    | NOT expr                                   { E_unop(Un_not, $2) }
    | expr relop expr    %prec NEQ               { E_binop($2,$1,$3) }
    | expr logrelop expr %prec BEQ               { E_binop($2,$1,$3) }
    | expr IN expr                               { E_binop(Bin_in, $1, $3) }
    | expr PIPE expr                             { E_binop(Bin_pipe, $1, $3) }
    | expr SUCHTHAT expr                         { E_binop(Bin_suchthat, $1, $3) }
    | PLUS expr %prec UPLUS                      { E_unop(Un_plus, $2) }
    | MINUS expr %prec UMINUS                    { E_unop(Un_minus, $2) }
    | SATISFIED LPAREN fname COMMA number RPAREN { E_satisfied($3, Some($5)) }
    | SATISFIED LPAREN fname RPAREN              { E_satisfied($3, None) }
    | SUM LBRACKET set RBRACKET                  { E_sum($3) }
    | PROD LBRACKET set RBRACKET                 { E_prod($3) }
    | UNION LBRACKET set RBRACKET                { E_union($3) }
    | INTERSECTION LBRACKET set RBRACKET         { E_intersection($3) }
    | CARD LBRACKET set RBRACKET                 { E_card($3) }
    | CHOICE LBRACKET set RBRACKET               { E_choice($3) }
    | IDENTIFIER LPAREN expr RPAREN              { E_call($1,$3) }
    | LPAREN expr RPAREN                         { E_paren($2) }

relop: 
    | EQ  { Bin_eq }
    | LT  { Bin_lt }
    | GT  { Bin_gt }
    | LEQ { Bin_leq }
    | GEQ { Bin_geq }
    | NEQ { Bin_neq }

logrelop: 
    | BEQ { Bin_beq }
    | BNE { Bin_bne }

%%

let foo = "Hello World"
