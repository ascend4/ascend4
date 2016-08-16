(* This test program reads an a4c ASCEND file from stdin
 * and outputs a stream of tokens, one per line.
 *
 * To output directly from the lexer:
 *
 *   ./testTokens.byte lexer
 *
 * To parse the program to an AST and then convert it back to tokens:
 *
 *   ./testTokens.byte parser
 *)


let exit_with_error s =
  Printf.fprintf stderr "%s\n" s;
  exit 1

let token_sequence_from_lexer lexbuf = 
  Sequence.from_fun (fun () ->
    match Lexer.initial lexbuf with
    | Parser.EOF -> None
    | tok        -> Some(tok)
  )

let token_sequence_from_parser lexbuf =
  TokenizeTree.tokenize_ast (Parser.start Lexer.initial lexbuf)

let () =
  try
    let lexbuf = Lexing.from_channel stdin in
    let tokens = match Sys.argv with
      | [| _; "lexer"  |] -> token_sequence_from_lexer lexbuf
      | [| _; "parser" |] -> token_sequence_from_parser lexbuf
      | _                 -> exit_with_error "Bad command line"
    in
    tokens |> Sequence.iter (fun tok ->
      Printf.printf "%s\n" (Lexer.string_of_token tok))
  with
  | Types.LexerError s  -> exit_with_error ("Lexer error: " ^ s)
  | Types.SyntaxError s -> exit_with_error ("Parser error: " ^ s)
  | Types.NotImpl n     -> exit_with_error ("Not implemented yet: " ^ string_of_int n)
