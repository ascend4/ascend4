(*
 * ast_mapper allows us to implement AST transformation using open recursion.
 * A typical mapper would be based on default_mapper, a trivial mapper.
 *
 * This design is inspired by the one used for in Ocaml 4.03 compiler.
 * *)

open Containers
open Types

type mapper = {
  program: mapper -> program -> program;
  definition: mapper -> definition -> definition ;
  model_refines: mapper -> model_refines -> model_refines;
  dimension: mapper -> dimension -> dimension;
  dimexpr: mapper -> dimexpr -> dimexpr;
  fname: mapper -> fname -> fname;
  name: mapper -> name -> name;
  note: mapper -> note -> note;
  unit_def: mapper -> unit_def -> unit_def;
  procedure: mapper -> procedure -> procedure;
  set: mapper -> set -> set;
  set_item: mapper -> set_item -> set_item;
  statement: mapper -> statement -> statement;
  assignment: mapper -> assignment -> assignment;
  relation: mapper -> relation -> relation;
  run_target: mapper -> run_target -> run_target;
  switch_block: mapper -> switch_block -> switch_block;
  type_expr: mapper -> type_expr -> type_expr;
  expr: mapper -> expr -> expr;
}

let default_mapper = {
  program = (fun self -> function
    | defs -> List.map (self.definition self) defs
  );

  definition = (fun self -> function
    | Def_require_file(d) ->
        Def_require_file(d)

    | Def_provide_module(d) ->
        Def_provide_module(d)

    | Def_import(idopt,d) ->
        Def_import(idopt,d)

    | Def_add_method(mml,procs) ->
        let procs' = procs |> List.map (self.procedure self) in
        Def_add_method(mml, procs')

    | Def_replace_method(mml, procs) ->
        let procs' = procs |> List.map (self.procedure self) in
        Def_replace_method(mml, procs')

    | Def_add_notes(id, method_opt, notes_body) ->
        let notes_body' = notes_body |> List.map (self.note self) in
        Def_add_notes(id, method_opt, notes_body')

    | Def_constant(isuni, id1, id2, dim, atom_opt, comment_opt) ->
        let dim' = dim |> (self.dimension self); in
        Def_constant(isuni, id1, id2, dim', atom_opt, comment_opt)

    | Def_atom(isuni, id1, id2, dim, atom_opt,names_opt, stats, methods_opt) ->
        let dim' = dim |> (self.dimension self); in
        let names_opt' = names_opt |> Option.map (List.map (self.fname self)); in
        let stats' = stats |> List.map (self.statement self); in
        let methods_opt' = methods_opt |> Option.map (List.map (self.procedure self)) in
        Def_atom(isuni, id1, id2, dim', atom_opt, names_opt', stats', methods_opt')

    | Def_model(isuni, id, params_opt, wheres_opt, refines_opt, stats, methods_opt) ->
        let params_opt' = params_opt |> Option.map (List.map (self.statement self)); in
        let wheres_opt' = wheres_opt |> Option.map (List.map (self.statement self)); in
        let refines_opt' = refines_opt |> Option.map (self.model_refines self); in
        let stats' = stats |> List.map (self.statement self); in
        let methods_opt' = methods_opt |> Option.map (List.map (self.procedure self)) in
        Def_model(isuni, id, params_opt', wheres_opt', refines_opt', stats', methods_opt')

    | Def_definition(id, stats, methods_opt) ->
        let stats' = stats |> List.map (self.statement self); in
        let methods_opt' = methods_opt |> Option.map (List.map (self.procedure self)) in
        Def_definition(id, stats', methods_opt')

    | Def_patch(id1, id2, stats, methods_opt) ->
        let stats' = stats |> List.map (self.statement self); in
        let methods_opt' = methods_opt |> Option.map (List.map (self.procedure self)) in
        Def_patch(id1, id2, stats', methods_opt')

    | Def_units(units) ->
        let units' = units |> List.map (self.unit_def self) in
        Def_units(units')

    | Def_global(stats) ->
        let stats' = stats |> List.map (self.statement self) in
        Def_global(stats')
  );

  model_refines = (fun self -> function
    | Model_Refines(id, stats_opt) ->
        let stats_opt' = stats_opt |> Option.map (List.map (self.statement self)) in
        Model_Refines(id, stats_opt')
  );

  dimension = (fun self -> function
    | Dim_default       -> Dim_default
    | Dim_wildcard      -> Dim_wildcard
    | Dim_dimexpr(dim)  -> let dim' = dim |> (self.dimexpr self) in Dim_dimexpr(dim')
    | Dim_dimensionless -> Dim_dimensionless
  );

  dimexpr = (fun self -> function
    | Dimex_var(id) ->
        Dimex_var(id)

    | Dimex_int(i) ->
        Dimex_int(i)

    | Dimex_div(dim1,dim2) ->
        let dim1' = dim1 |> (self.dimexpr self); in
        let dim2' = dim2 |> (self.dimexpr self) in
        Dimex_div(dim1', dim2')

    | Dimex_mul(dim1,dim2) ->
        let dim1' = dim1 |> (self.dimexpr self); in
        let dim2' = dim2 |> (self.dimexpr self) in
        Dimex_mul(dim1', dim2')

    | Dimex_pow(dim1,frac) ->
        let dim1' = dim1 |> (self.dimexpr self) in
        Dimex_pow(dim1',frac)

    | Dimex_paren(dim) ->
        let dim' = dim |> (self.dimexpr self) in
        Dimex_paren(dim')
  );

  fname = (fun self -> function
    | (name, comment_opt) ->
        let name' = name |> (self.name self) in
        (name', comment_opt)
  );

  name = (fun self -> function
    | Name_id(id) ->
        Name_id(id)

    | Name_dot(name, id)  ->
        let name' = name |> (self.name self) in
        Name_dot(name', id)

    | Name_set(name, set) ->
        let name' = name |> (self.name self); in
        let set' = set |> (self.set self) in
        Name_set(name', set')

    | Name_der(fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Name_der(fnames')

    | Name_pre(fname) ->
        let fname' = fname |> (self.fname self) in
        Name_pre(fname')

  );

  note = (fun self -> function
    | Note(symbol, notes) ->
        let notes' = notes |> List.map (fun (fnames, braced_text) ->
          let fnames' = fnames |> List.map (self.fname self) in
          (fnames', braced_text)
          ) in
        Note(symbol, notes')
  );

  unit_def = (fun self -> function
    | Unit(id, braced) -> Unit(id, braced)
  );

  procedure = (fun self -> function
    | Method(id, stats) ->
        let stats' = stats |> List.map (self.statement self) in
        Method(id, stats')
  );

  set = (fun self -> function
    | set_items -> List.map (self.set_item self) set_items
  );

  set_item = (fun self -> function
    | Set_singleton(expr) ->
        let expr' = expr |> (self.expr self) in
        Set_singleton(expr')

    | Set_range(expr1, expr2) ->
        let expr1' = expr1 |> (self.expr self); in
        let expr2' = expr2 |> (self.expr self) in
        Set_range(expr1', expr2')
  );

  statement = (fun self -> function
    | Stat_aliases_one(fnames, fname) ->
        let fnames' = fnames |> List.map (self.fname self); in
        let fname' = fname |> (self.fname self) in
        Stat_aliases_one(fnames', fname')

    | Stat_aliases_many(fnames1, fnames2, fnames3, id1, id2, set_opt) ->
        let fnames1' = fnames1 |> List.map (self.fname self); in
        let fnames2' = fnames2 |> List.map (self.fname self); in
        let fnames3' = fnames3 |> List.map (self.fname self); in
        let set_opt' = set_opt |> Option.map (self.set self) in
        Stat_aliases_many(fnames1', fnames2', fnames3', id1, id2, set_opt')

    | Stat_arealike(fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_arealike(fnames')

    | Stat_arethesame(fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_arethesame(fnames')

    | Stat_assert(expr) ->
        let expr' = expr |> (self.expr self) in
        Stat_assert(expr')

    | Stat_assign(assignment) ->
        let assignment' = assignment |> (self.assignment self) in
        Stat_assign(assignment')

    | Stat_blackbox(label, id, input_args, output_args, data_arg_opt) ->
        let label' = label |> (self.fname self); in
        let input_args' = input_args |> List.map (self.fname self); in
        let output_args' = output_args |> List.map (self.fname self); in
        let data_arg_opt' = data_arg_opt |> Option.map (self.fname self) in
        Stat_blackbox(label', id, input_args', output_args', data_arg_opt')

    | Stat_call(id, set_opt) ->
        let set_opt' = set_opt |> Option.map (self.set self) in
        Stat_call(id, set_opt')

    | Stat_conditional(stats) ->
        let stats' = stats |> List.map (self.statement self) in
        Stat_conditional(stats')

    | Stat_der(fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_der(fnames')

    | Stat_derivative(fnames, fname_opt) ->
        let fnames' = fnames |> List.map (self.fname self); in
        let fname_opt' = fname_opt |> Option.map (self.fname self) in
        Stat_derivative(fnames', fname_opt')

    | Stat_event(label_opt, fnames, switch_blocks) ->
        let label_opt' = label_opt |> Option.map (self.fname self); in
        let fnames' = fnames |> List.map (self.fname self); in
        let switch_blocks' = switch_blocks |> List.map (self.switch_block self) in
        Stat_event(label_opt', fnames', switch_blocks')

    | Stat_external(id, fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_external(id, fnames')

    | Stat_fix_and_assign(assignment) ->
        let assignment' = assignment |> (self.assignment self) in
        Stat_fix_and_assign(assignment')

    | Stat_fix(fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_fix(fnames')

    | Stat_flow(jump) ->
        Stat_flow(jump)

    | Stat_for(id, expr, for_direction_opt, for_action, stats) ->
        let expr' = expr |> (self.expr self); in
        let stats' = stats |> List.map (self.statement self) in
        Stat_for(id, expr', for_direction_opt, for_action, stats')

    | Stat_free(fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_free(fnames')

    | Stat_glassbox(label, id, fnames, n, scope_opt) ->
        let label' = label |> (self.fname self); in
        let fnames' = fnames |> List.map (self.fname self); in
        let scope_opt' = scope_opt |> Option.map (self.fname self) in
        Stat_glassbox(label', id, fnames', n, scope_opt')

    | Stat_if(expr, stats) ->
        let expr' = expr |> (self.expr self); in
        let stats' = stats |> List.map (self.statement self) in
        Stat_if(expr', stats')

    | Stat_if_else(expr, stats1, stats2) ->
        let expr' = expr |> (self.expr self); in
        let stats1' = stats1 |> List.map (self.statement self); in
        let stats2' = stats2 |> List.map (self.statement self) in
        Stat_if_else(expr', stats1', stats2')

    | Stat_independent(fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_independent(fnames')

    | Stat_is(fnames, id, id_opt) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_is(fnames', id, id_opt)

    | Stat_isa(fnames, type_expr, id_opt, value_opt) ->
        let fnames' = fnames |> List.map (self.fname self); in
        let type_expr' = type_expr |> (self.type_expr self); in
        let value_opt' = value_opt |> Option.map (self.expr self) in
        Stat_isa(fnames', type_expr', id_opt, value_opt')

    | Stat_isrefinedto(fnames, type_expr) ->
        let fnames' = fnames |> List.map (self.fname self); in
        let type_expr' = type_expr |> (self.type_expr self) in
        Stat_isrefinedto(fnames', type_expr')

    | Stat_link_ignore(symbol, fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_link_ignore(symbol, fnames')

    | Stat_link_symbol(symbol, fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_link_symbol(symbol, fnames')

    | Stat_link_name(fname,fnames) ->
        let fname' = fname |> (self.fname self); in
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_link_name(fname', fnames')

    | Stat_notes(notes) ->
        let notes' = notes |> List.map (self.note self) in
        Stat_notes(notes')

    | Stat_option(id, expr) ->
        let expr' = expr |> (self.expr self) in
        Stat_option(id, expr')

    | Stat_previous(fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_previous(fnames')
    
    | Stat_relation(label_opt, relation) ->
        let label_opt' = label_opt |> Option.map (self.fname self); in
        let relation' = relation |> (self.relation self) in
        Stat_relation(label_opt', relation')

    | Stat_run(run_target) ->
        let run_target' = run_target |> (self.run_target self) in
        Stat_run(run_target')

    | Stat_select(fnames, switch_blocks) ->
        let fnames' = fnames |> List.map (self.fname self); in
        let switch_blocks' = switch_blocks |> List.map (self.switch_block self) in
        Stat_select(fnames', switch_blocks')

    | Stat_solve ->
        Stat_solve

    | Stat_solver(id) ->
        Stat_solver(id)

    | Stat_switch(fnames, switch_blocks) ->
        let fnames' = fnames |> List.map (self.fname self); in
        let switch_blocks' = switch_blocks |> List.map (self.switch_block self) in
        Stat_switch(fnames', switch_blocks')

    | Stat_units(unit_defs) ->
        let unit_defs' = unit_defs |> List.map (self.unit_def self) in
        Stat_units(unit_defs')

    | Stat_unlink_symbol(symbol, fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_unlink_symbol(symbol, fnames')

    | Stat_unlink_name(fname,fnames) ->
        let fname' = fname |> (self.fname self); in
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_unlink_name(fname', fnames')

    | Stat_use(fname) ->
        let fname' = fname |> (self.fname self) in
        Stat_use(fname')

    | Stat_when(label_opt,fnames, switch_blocks) ->
        let label_opt' = label_opt |> Option.map (self.fname self); in
        let fnames' = fnames |> List.map (self.fname self); in
        let switch_blocks' = switch_blocks |> List.map (self.switch_block self) in
        Stat_when(label_opt', fnames', switch_blocks')

    | Stat_while(expr,stats) ->
        let expr' = expr |> (self.expr self); in
        let stats' = stats |> List.map (self.statement self) in
        Stat_while(expr', stats')

    | Stat_willbe(fnames, type_expr, id_opt, expr_opt) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_willbe(fnames', type_expr, id_opt, expr_opt)

    | Stat_willbethesame(fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_willbethesame(fnames')

    | Stat_willnotbethesame(fnames) ->
        let fnames' = fnames |> List.map (self.fname self) in
        Stat_willnotbethesame(fnames')
  );

  assignment = (fun self -> function
    | Assign_var(fname, expr) ->
        let fname' = fname |> (self.fname self); in
        let expr' = expr |> (self.expr self) in
        Assign_var(fname', expr')

    | Assign_const(fname, expr) ->
        let fname' = fname |> (self.fname self); in
        let expr' = expr |> (self.expr self) in
        Assign_const(fname', expr')
  );

  relation = (fun self -> function
    | Rel_expr(expr) -> let expr' = expr |> (self.expr self) in Rel_expr(expr')
    | Rel_max(expr)  -> let expr' = expr |> (self.expr self) in Rel_max(expr')
    | Rel_min(expr)  -> let expr' = expr |> (self.expr self) in Rel_min(expr')
  );

  run_target = (fun self -> function
    | Run_public(fname) ->
        let fname' = fname |> (self.fname self) in
        Run_public(fname')

    | Run_hidden(fname1,fname2) ->
        let fname1' = fname1 |> (self.fname self); in
        let fname2' = fname2 |> (self.fname self) in
        Run_hidden(fname1', fname2')

  );

  switch_block = (fun self -> function
    | Switch_case(set, stats) ->
        let set' = set |> (self.set self); in
        let stats' = stats |> List.map (self.statement self) in
        Switch_case(set', stats')

    | Switch_otherwise(stats) ->
        let stats' = stats |> List.map (self.statement self) in
        Switch_otherwise(stats')

  );

  type_expr = (fun self -> function
    | TE_var(id) ->
        TE_var(id)

    | TE_call(id, set) ->
        let set' = set |> (self.set self) in
        TE_call(id, set')
  );

  expr = (fun self -> function
    | E_number(n) -> E_number(n)
    | E_maxinteger -> E_maxinteger
    | E_maxreal -> E_maxreal
    | E_boolean(b) -> E_boolean(b)
    | E_any -> E_any
    | E_symbol(symbol) -> E_symbol(symbol)
    | E_var(fname) ->
        let fname' = fname |> (self.fname self) in
        E_var(fname')
    | E_bracket(set) ->
        let set' = set |> (self.set self) in
        E_bracket(set')
    | E_binop(binop,expr1,expr2) ->
        let expr1' = expr1 |> (self.expr self); in
        let expr2' = expr2 |> (self.expr self) in
        E_binop(binop, expr1', expr2')
    | E_unop(unop,expr) ->
        let expr' = expr |> (self.expr self) in
        E_unop(unop, expr')
    | E_satisfied(fname, n_opt) ->
        let fname' = fname |> (self.fname self) in
        E_satisfied(fname', n_opt)
    | E_sum(set) ->
        let set' = set |> (self.set self) in
        E_sum(set')
    | E_prod(set) ->
        let set' = set |> (self.set self) in
        E_prod(set')
    | E_union(set) ->
        let set' = set |> (self.set self) in
        E_union(set')
    | E_intersection(set) ->
        let set' = set |> (self.set self) in
        E_intersection(set')
    | E_card(set) ->
        let set' = set |> (self.set self) in
        E_card(set')
    | E_choice(set) ->
        let set' = set |> (self.set self) in
        E_choice(set')
    | E_call(id, expr) ->
        let expr' = expr |> (self.expr self) in
        E_call(id, expr')
    | E_paren(expr) ->
        let expr' = expr |> (self.expr self) in
        E_paren(expr')
  );
}
