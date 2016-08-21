(* This test program reads an a4c ASCEND file from stdin
 * and outputs a stream of tokens, one per line.
 *
 * To output directly from the lexer:
 *
 *   ./test_tokens.byte lexer < input_file.a4c
 *
 * To parse the program to an AST and then convert it back to tokens:
 *
 *   ./test_tokens.byte parser < input_file.a4c
 *
 *)

open Containers

let () =
  let tokens =
    match Sys.argv with
    | [| _; "lexer" |] ->
        Parser.tokenize stdin
    | [| _; "parser" |] ->
        Tokenize_tree.tokenize_ast (Parser.parse stdin)
    | _ ->
        Printf.fprintf stderr "Bad command line\n";
        exit 1
  in
  Parser.print_tokens stdout tokens
