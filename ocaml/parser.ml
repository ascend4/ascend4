(* Helper functions for lexing and parsing files *)

open Containers


let exit_with_error lexbuf message =
  let open Lexing in
  let pos = lexbuf.lex_curr_p in
  let line_num = pos.pos_lnum in
  let col_num  = pos.pos_cnum - pos.pos_bol in
  Printf.fprintf stderr "Line %d, col %d: %s\n" line_num col_num message;
  exit 1

let format_LexerError s     = Printf.sprintf "Lexer error: %s" s
let format_SyntaxError s    = Printf.sprintf "Syntax error: %s" s
let format_GrammarError     = Printf.sprintf "Syntax error"
let format_NotImplemented n = Printf.sprintf "Not implemented yet: %d" n

let tokenize in_channel = 
  let lexbuf = Lexing.from_channel in_channel in
  Sequence.from_fun (fun () ->
    try
      match Lexer.initial lexbuf with
      | Grammar.EOF -> None
      | tok        -> Some(tok)
    with
    | Types.LexerError s     -> exit_with_error lexbuf (format_LexerError s)
    | Types.NotImplemented n -> exit_with_error lexbuf (format_NotImplemented n)
  )

let parse in_channel =
  let lexbuf = Lexing.from_channel in_channel in
  try
    Grammar.start Lexer.initial lexbuf
  with
  | Types.LexerError s     -> exit_with_error lexbuf (format_LexerError s)
  | Types.SyntaxError s    -> exit_with_error lexbuf (format_SyntaxError s)
  | Grammar.Error          -> exit_with_error lexbuf (format_GrammarError)
  | Types.NotImplemented n -> exit_with_error lexbuf (format_NotImplemented n)


let print_tokens out_chan tokens = 
  tokens |> Sequence.iter (fun tok ->
    Printf.fprintf out_chan "%s\n" (Lexer.string_of_token tok))


