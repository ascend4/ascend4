(* This is a proof of concept for an ASCEND AST transformer.
 * It inserts a DERIVATIVE OF declaration corresponding to what derivatives 
 * are actually being used in expressions throughout the model.
 *
 * usage: inverDerivatives.byte < model.a4c > transformed_model.a4c
 *
 * *)

open Containers
open Fun

open Types
open Ast_iterator
open Ast_mapper

let find_derivatives statements = 
  let derived = ref [] in
  let find_der = {
    default_iterator with
    name = (fun it -> function
      | Name_der([fname]) ->
          if not (List.mem fname (!derived)) then
            derived := (fname :: !derived)
      | Name_der([]) ->
          assert false
      | Name_der(_::_::_) -> 
          Printf.fprintf stderr "I don't know what do do with multiparam derivatives\n";
          exit 1
      | name -> default_iterator.name it name
    );
  } in
  List.iter (find_der.statement find_der) statements;
  !derived

let is_var_decl stat =
  match stat with
    | Stat_isa _ -> true
    | _          -> false

let is_derivative_decl stat = 
  match stat with
    | Stat_derivative _ -> true
    | _                 -> false

let infer_derivatives ast = 
  let mapper = {
    default_mapper with
    definition = (fun self -> function
      | Def_model(isuni, id, params_opt, wheres_opt, refines_opt, stats, methods_opt) ->
          (* get rid of existing DERIVARIVE OF declarations and
           * hoist variable declarations to the top. *)
          let stats' = List.filter (not % is_derivative_decl) stats in 
          let (decl_stats, other_stats) = List.partition is_var_decl stats' in

          (* Infer the derivatives that we use and
           * insert a new DERIVATIVE OF declaration *)
          let der_stats =
            match find_derivatives other_stats with
            | []           -> []  (* No derivatives found *)
            | derived_vars -> [ Stat_derivative(derived_vars, None) ]
          in
          let stats''  = decl_stats @ der_stats @ other_stats in
          Def_model(isuni, id, params_opt, wheres_opt, refines_opt, stats'', methods_opt)

      | def ->
          default_mapper.definition self def
    );
  } in
  mapper.program mapper ast


let () =
  let ast = Parser.parse stdin in
  let ast' = infer_derivatives ast in
  Parser.print_tokens stdout (Tokenize_tree.tokenize_ast ast')
