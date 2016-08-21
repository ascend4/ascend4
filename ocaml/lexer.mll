{
open Grammar
open Types

open Lexing
open Containers

let update_line_number lexbuf = 
   Lexing.lexeme lexbuf |> String.iter (fun c ->
      if c = '\n' then Lexing.new_line lexbuf
    )
}

let blank =      ['\x0C' '\x0D' '\x09' '\x0B' '\x20']  (*[\f\r\t\v ]*)
let digit =      ['0'-'9']
let letter =     ['a'-'z' 'A'-'Z']

let exp =        (['e''E']['-''+']? digit+)
let real =       ((((digit+ "." digit*)|("." digit+)) exp?) | (digit+ exp))
let integer =    digit+
let idChar =     letter (integer|letter|'_')*

rule
initial = parse
  | eof      { EOF }

  (* Multichar operators *)
  | "<="     { LEQ }
  | ">="     { GEQ }
  | "<>"     { NEQ }
  | ".."     { DOTDOT }
  | "::"     { DBLCOLON }
  | ":="     { ASSIGN }
  | ":=="    { CASSIGN }
  | "=="     { BEQ  }
  | "!="     { BNE  }
  (* Single-char operators and tokens *)
  | "="      { EQ }
  | ">"      { GT }
  | "<"      { LT }
  | ","      { COMMA }
  | "."      { DOT }
  | ";"      { SEMICOLON }
  | ":"      { COLON }
  | "["      { LBRACKET }
  | "]"      { RBRACKET }
  | "("      { LPAREN }
  | ")"      { RPAREN }
  | "+"      { PLUS }
  | "-"      { MINUS }
  | "*"      { TIMES }
  | "/"      { DIV }
  | "^"      { CIRCUMFLEX }
  | "|"      { PIPE }
  | "'ignore'"  { IGNORE }
  (* Reserverd Words *)
  | idChar as name {
    match name with
    | "ADD"           -> ADD 
    | "ALIASES"       -> ALIASES 
    | "AND"           -> AND 
    | "ANY"           -> ANY 
    | "ARE_ALIKE"     -> AREALIKE 
    | "ARE_THE_SAME"  -> ARETHESAME 
    (*| "ARRAY"         -> ARRAY *)
    | "ASSERT"        -> ASSERT 
    | "ATOM"          -> ATOM 
    | "BREAK"         -> BREAK 
    | "CALL"          -> CALL 
    | "CARD"          -> CARD 
    | "CASE"          -> CASE 
    | "CHECK"         -> CHECK 
    | "CHILDREN"      -> CHILDREN 
    | "CHOICE"        -> CHOICE 
    | "CONDITIONAL"   -> CONDITIONAL 
    | "CONSTANT"      -> CONSTANT 
    | "CONTINUE"      -> CONTINUE 
    | "CREATE"        -> CREATE 
    | "DATA"          -> DATA
    | "DECREASING"    -> DECREASING 
    | "DEFAULT"       -> DEFAULT 
    | "DEFINITION"    -> DEFINITION 
    | "DER"           -> DER 
    | "der"           -> DERIV 
    | "DERIVATIVE"    -> DERIVATIVE 
    | "DIMENSION"     -> DIMENSION 
    | "DIMENSIONLESS" -> DIMENSIONLESS 
    | "DO"            -> DO 
    | "ELSE"          -> ELSE 
    | "END"           -> END 
    | "EVENT"         -> EVENT 
    | "EXPECT"        -> EXPECT 
    | "EXTERNAL"      -> EXTERNAL 
    | "FALSE"         -> FALSE 
    | "FALL_THROUGH"  -> FALLTHRU 
    | "FIX"           -> FIX 
    | "FOR"           -> FOR 
    | "FREE"          -> FREE 
    | "FROM"          -> FROM 
    | "GLOBAL"        -> GLOBAL 
    | "IF"            -> IF 
    | "IMPORT"        -> IMPORT 
    | "IN"            -> IN 
    | "INCREASING"    -> INCREASING 
    | "INDEPENDENT"   -> INDEPENDENT 
    | "INPUT"         -> INPUT
    | "INTERSECTION"  -> INTERSECTION 
    | "IS_A"          -> ISA 
    | "IS_REFINED_TO" -> ISREFINEDTO 
    | "LIKE"          -> LIKE 
    | "LINK"          -> LINK  
    | "MAXIMIZE"      -> MAXIMIZE 
    | "MAX_INTEGER"   -> MAXINTEGER 
    | "MAX_REAL"      -> MAXREAL 
    | "METHOD"        -> METHOD 
    | "METHODS"       -> METHODS 
    | "MINIMIZE"      -> MINIMIZE 
    | "MODEL"         -> MODEL 
    | "NOT"           -> NOT 
    | "NOTES"         -> NOTES 
    | "OF"            -> OF 
    | "OPTION"        -> OPTION 
    | "OR"            -> OR 
    | "OTHERWISE"     -> OTHERWISE 
    | "OUTPUT"        -> OUTPUT
    | "pre"           -> PRE 
    | "PREVIOUS"      -> PREVIOUS 
    | "PROD"          -> PROD 
    | "PROVIDE"       -> PROVIDE 
    | "REFINES"       -> REFINES 
    | "REPLACE"       -> REPLACE 
    | "REQUIRE"       -> REQUIRE 
    | "RETURN"        -> RETURN 
    | "RUN"           -> RUN 
    | "SATISFIED"     -> SATISFIED 
    | "SELECT"        -> SELECT 
    (*| "SIZE"          -> SIZE *)
    | "SOLVE"         -> SOLVE 
    | "SOLVER"        -> SOLVER 
    | "STOP"          -> STOP 
    | "SUCH_THAT"     -> SUCHTHAT 
    | "SUM"           -> SUM 
    | "SWITCH"        -> SWITCH 
    | "THEN"          -> THEN 
    | "TRUE"          -> TRUE 
    | "UNION"         -> UNION 
    | "UNITS"         -> UNITS 
    | "UNIVERSAL"     -> UNIVERSAL 
    | "UNLINK"        -> UNLINK  
    | "USE"           -> USE 
    (*| "VALUE"         -> VALUE *)
    | "WHEN"          -> WHEN 
    | "WHERE"         -> WHERE 
    | "WHILE"         -> WHILE 
    | "WILL_BE"       -> WILLBE 
    | "WILL_BE_THE_SAME"      -> WILLBETHESAME 
    | "WILL_NOT_BE_THE_SAME"  -> WILLNOTBETHESAME 
    | "WITH"          -> WITH 
    | "WITH_VALUE"    -> WITH_VALUE 

    | _               -> (IDENTIFIER (Identifier name))
  }

  | "(*"            { comment 1 lexbuf; initial lexbuf }
  | "\'"            { symbol      (Buffer.create 20) lexbuf }
  | "\""            { doublequote (Buffer.create 20) lexbuf }

  | "{" (blank* '\n')? {
          update_line_number lexbuf;
          bracedtext 1 (Buffer.create 20) lexbuf
        }

  (* We do not want 1..2 to parse as 1. followed by .2 
   * Se we use a big hack to pretend ocamllex has lookahead *)
  | (integer as nstr) ".." {
          lexbuf.lex_curr_pos <- lexbuf.lex_curr_pos - 2;
          (INTEGER (int_of_string nstr))
        }

  | integer as nstr { (INTEGER (int_of_string nstr)) }
  | real    as rstr { (REAL (float_of_string rstr)) }

  | blank* { update_line_number lexbuf; initial lexbuf }
  | '\n'   { update_line_number lexbuf; initial lexbuf }

  | _ as c {
          let msg = Printf.sprintf "Unexpected char '%c'" c in
          raise (Types.LexerError msg)
        }

and
comment nesting = parse
  | eof    { raise (Types.LexerError "Unfinished comment") } 
  | "(*"   { comment (nesting + 1) lexbuf }
  | "*)"   { if nesting == 1 then () else comment (nesting - 1) lexbuf }
  | _      {
    update_line_number lexbuf;
    comment nesting lexbuf
  }

and
symbol strbuf = parse
  | eof  { raise (Types.LexerError "Unfinished symbol") }
  | '\n' { raise (Types.LexerError "Unfinished symbol") }
  | '\'' { (SYMBOL (Symbol (Buffer.contents strbuf))) }
  | _ as c {
    update_line_number lexbuf;
    Buffer.add_char strbuf c;
    symbol strbuf lexbuf
  }

and 
doublequote strbuf = parse
  | eof  { raise (Types.LexerError "Unfinished string") }
  | '"'  { (DQUOTE (DQuote (Buffer.contents strbuf))) }
  | '\\' {
    Buffer.add_string strbuf (backslash lexbuf);
    doublequote strbuf lexbuf
  }
  | _ as c {
    update_line_number lexbuf;
    Buffer.add_char strbuf c;
    doublequote strbuf lexbuf
  }

and
bracedtext nesting strbuf = parse
  | eof  { raise (Types.LexerError "Unfinished braced text") }
  | '{' as c {
      Buffer.add_char strbuf c;
      bracedtext (nesting + 1) strbuf lexbuf
    }
  | '}' as c {
      if nesting == 1 then begin
        (BRACEDTEXT (BracedText (Buffer.contents strbuf)))
      end else begin
        Buffer.add_char strbuf c;
        bracedtext (nesting - 1) strbuf lexbuf
      end
    }
  | '\\' {
      Buffer.add_string strbuf (backslash lexbuf);
      bracedtext nesting strbuf lexbuf
    }
  | _ as c {
      update_line_number lexbuf;
      Buffer.add_char strbuf c;
      bracedtext nesting strbuf lexbuf
    }


and
backslash = parse
  | 'a' { "\x07" }
  | 'b' { "\x08" }
  | 'f' { "\x0C" }
  | 'n' { "\x0A" }
  | 'r' { "\x0D" }
  | 't' { "\x09" }
  | 'v' { "\x0B" }
  | 'x'       { raise (Types.LexerError "\\xhh escapes not implemented yet") }
  | ['0'-'7'] { raise (Types.LexerError "Octal escapes not implemented yet") } 
  | '\n' {
      update_line_number lexbuf;
      "\n"
    }
  | _ as c { String.make 1 c }


{

let ascend_quote ldelim rdelim specialset =
  let re = Str.regexp "[{}]" in
  fun str -> ldelim ^ Str.global_replace re "\\\\\\0" str ^ rdelim
  
let quote_ascend_bracedtext = ascend_quote "{"  "}"  "[{}]"
let quote_ascend_symbol     = ascend_quote "\'" "\'" "[\']"
let quote_ascend_dquote     = ascend_quote "\"" "\"" "[\"]"


(* Convert token back to how it looked before we lexed it (as much as we can)
 * This conversion is not guaranteed to roundtrip 100% because:
 *  - Some things may be represented multiple ways (ex.: 100.0 vs  1.0e2),
 *  - Lexing throws aways some things (comments, leading space in braced text, etc)
 *  - I'm using Ocaml's string_of_XXX functions instead of functions tuned for ASCEND
 *  - and maybe more... *)
let string_of_token = function
  | EOF              -> ""
  | LEQ              -> "<="
  | GEQ              -> ">="
  | NEQ              -> "<>"
  | DOTDOT           -> ".."
  | DBLCOLON         -> "::"
  | ASSIGN           -> ":="
  | CASSIGN          -> ":=="
  | BEQ              -> "=="
  | BNE              -> "!="

  | EQ               -> "="
  | LT               -> ">"
  | GT               -> "<"
  | COMMA            -> ","
  | DOT              -> "."
  | SEMICOLON        -> ";"
  | COLON            -> ":"
  | LBRACKET         -> "["
  | RBRACKET         -> "]"
  | LPAREN           -> "("
  | RPAREN           -> ")"
  | PLUS             -> "+"
  | MINUS            -> "-"
  | TIMES            -> "*"
  | DIV              -> "/"
  | CIRCUMFLEX       -> "^"
  | PIPE             -> "|"

  | ADD              -> "ADD"
  | ALIASES          -> "ALIASES"
  | AND              -> "AND"
  | ANY              -> "ANY"
  | AREALIKE         -> "ARE_ALIKE"
  | ARETHESAME       -> "ARE_THE_SAME"
  (*| ARRAY            -> "ARRAY"*)
  | ASSERT           -> "ASSERT"
  | ATOM             -> "ATOM"
  | BREAK            -> "BREAK"
  | CALL             -> "CALL"
  | CARD             -> "CARD"
  | CASE             -> "CASE"
  | CHOICE           -> "CHOICE"
  | CHECK            -> "CHECK"
  | CHILDREN         -> "CHILDREN"
  | CONDITIONAL      -> "CONDITIONAL"
  | CONSTANT         -> "CONSTANT"
  | CONTINUE         -> "CONTINUE"
  | CREATE           -> "CREATE"
  | DATA              -> "DATA"
  | DECREASING       -> "DECREASING"
  | DEFAULT          -> "DEFAULT"
  | DEFINITION       -> "DEFINITION"
  | DER              -> "DER"
  | DERIV            -> "der"
  | DERIVATIVE       -> "DERIVATIVE"
  | DIMENSION        -> "DIMENSION"
  | DIMENSIONLESS    -> "DIMENSIONLESS"
  | DO               -> "DO"
  | ELSE             -> "ELSE"
  | END              -> "END"
  | EVENT            -> "EVENT"
  | EXPECT           -> "EXPECT"
  | EXTERNAL         -> "EXTERNAL"
  | FALSE            -> "FALSE"
  | FALLTHRU         -> "FALL_THROUGH"
  | FIX              -> "FIX"
  | FOR              -> "FOR"
  | FREE             -> "FREE"
  | FROM             -> "FROM"
  | GLOBAL           -> "GLOBAL"
  | IF               -> "IF"
  | IGNORE           -> "'ignore'"
  | IMPORT           -> "IMPORT"
  | IN               -> "IN"
  | INCREASING       -> "INCREASING"
  | INDEPENDENT      -> "INDEPENDENT"
  | INPUT            -> "INPUT"
  | INTERSECTION     -> "INTERSECTION"
  | IS               -> "IS"
  | ISA              -> "IS_A"
  | ISREFINEDTO      -> "IS_REFINED_TO"
  | LIKE             -> "LIKE"
  | LINK             -> "LINK"
  | MAXIMIZE         -> "MAXIMIZE"
  | MAXINTEGER       -> "MAX_INTEGER"
  | MAXREAL          -> "MAX_REAL"
  | METHOD           -> "METHOD"
  | METHODS          -> "METHODS"
  | MINIMIZE         -> "MINIMIZE"
  | MODEL            -> "MODEL"
  | NOT              -> "NOT"
  | NOTES            -> "NOTES"
  | OF               -> "OF"
  | OPTION           -> "OPTION"
  | OR               -> "OR"
  | OTHERWISE        -> "OTHERWISE"
  | OUTPUT           -> "OUTPUT"
  | PATCH            -> "PATCH"
  | PRE              -> "pre"
  | PREVIOUS         -> "PREVIOUS"
  | PROD             -> "PROD"
  | PROVIDE          -> "PROVIDE"
  | REFINES          -> "REFINES"
  | REPLACE          -> "REPLACE"
  | REQUIRE          -> "REQUIRE"
  | RETURN           -> "RETURN"
  | RUN              -> "RUN"
  | SATISFIED        -> "SATISFIED"
  | SELECT           -> "SELECT"
  (*| SIZE             -> "SIZE"*)
  | SOLVE            -> "SOLVE"
  | SOLVER           -> "SOLVER"
  | STOP             -> "STOP"
  | SUCHTHAT         -> "SUCHTHAT"
  | SUM              -> "SUM"
  | SWITCH           -> "SWITCH"
  | THEN             -> "THEN"
  | TRUE             -> "TRUE"
  | UNION            -> "UNION"
  | UNITS            -> "UNITS"
  | UNIVERSAL        -> "UNIVERSAL"
  | UNLINK           -> "UNLINK"
  | USE              -> "USE"
  (*| VALUE            -> "VALUE"*)
  | WHEN             -> "WHEN"
  | WHERE            -> "WHERE"
  | WHILE            -> "WHILE"
  | WILLBE           -> "WILL_BE"
  | WILLBETHESAME    -> "WILL_BE_THE_SAME"
  | WILLNOTBETHESAME -> "WILL_NOT_BE_THE_SAME"
  | WITH             -> "WITH"
  | WITH_VALUE       -> "WITH_VALUE"
  | REAL x            -> string_of_float x
  | INTEGER x         -> string_of_int x
  | IDENTIFIER (Identifier x) -> x
  | BRACEDTEXT (BracedText x) -> quote_ascend_bracedtext x
  | SYMBOL (Symbol x)         -> quote_ascend_symbol x
  | DQUOTE (DQuote x)         -> quote_ascend_dquote x



(* Stringify token in the format used by "menhir --interpret" *)
(* This can be helpful if you need to find out why the grammar is giving syntax errors *)
let debug_string_of_token = function
  | EOF              -> "EOF"
  | LEQ              -> "LEQ"
  | GEQ              -> "GEQ"
  | NEQ              -> "NEQ"
  | DOTDOT           -> "DOTDOT"
  | DBLCOLON         -> "DBLCOLON"
  | ASSIGN           -> "ASSIGN"
  | CASSIGN          -> "CASSIGN"
  | BEQ              -> "BEQ"
  | BNE              -> "BNE"

  | EQ               -> "EQ"
  | LT               -> "LT"
  | GT               -> "GT"
  | COMMA            -> "COMMA"
  | DOT              -> "DOT"
  | SEMICOLON        -> "SEMICOLON"
  | COLON            -> "COLON"
  | LBRACKET         -> "LBRACKET"
  | RBRACKET         -> "RBRACKET"
  | LPAREN           -> "LPAREN"
  | RPAREN           -> "RPAREN"
  | PLUS             -> "PLUS"
  | MINUS            -> "MINUS"
  | TIMES            -> "TIMES"
  | DIV              -> "DIV"
  | CIRCUMFLEX       -> "CIRCUMFLEX"
  | PIPE             -> "PIPE"

  | AREALIKE         -> "AREALIKE"
  | ARETHESAME       -> "ARETHESAME"
  | DERIV            -> "DERIV"
  | FALLTHRU         -> "FALLTHRU"
  | IGNORE           -> "IGNORE"
  | ISA              -> "ISA"
  | ISREFINEDTO      -> "ISREFINEDTO"
  | MAXINTEGER       -> "MAXINTEGER"
  | PRE              -> "PRE"
  | WILLBE           -> "WILLBE"
  | WILLBETHESAME    -> "WILLBETHESAME"
  | WILLNOTBETHESAME -> "WILLNOTBETHESAME"
  | WITH             -> "WITH"
  | WITH_VALUE       -> "WITH_VALUE"

  | REAL _            -> "REAL"
  | INTEGER _         -> "INTEGER"
  | IDENTIFIER _      -> "IDENTIFIER"
  | BRACEDTEXT _      -> "BRACEDTEXT"
  | SYMBOL _          -> "SYMBOL"
  | DQUOTE _          -> "DQUOTE"

  | tok -> string_of_token tok
}
