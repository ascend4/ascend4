(*
 * ast_iterator allows us to implement AST inspection using open recursion.
 * A typical iterator woul be based on default_iterator, a trivial iterator.
 *
 * This design is inspired by the one used for in Ocaml 4.03 compiler.
 * *)

open Containers
open Types

type iterator = {
  program: iterator -> program -> unit;
  definition: iterator -> definition -> unit ;
  model_refines: iterator -> model_refines -> unit;
  dimension: iterator -> dimension -> unit;
  dimexpr: iterator -> dimexpr -> unit;
  fname: iterator -> fname -> unit;
  name: iterator -> name -> unit;
  note: iterator -> note -> unit;
  unit_def: iterator -> unit_def -> unit;
  procedure: iterator -> procedure -> unit;
  set: iterator -> set -> unit;
  set_item: iterator -> set_item -> unit;
  statement: iterator -> statement -> unit;
  assignment: iterator -> assignment -> unit;
  relation: iterator -> relation -> unit;
  run_target: iterator -> run_target -> unit;
  switch_block: iterator -> switch_block -> unit;
  type_expr: iterator -> type_expr -> unit;
  expr: iterator -> expr -> unit;
}

let default_iterator = {
  program = (fun it -> function
    | defs -> defs |> List.iter (it.definition it)
  );

  definition = (fun it -> function
    | Def_require_file(d) -> ()
    | Def_provide_module(d) -> ()
    | Def_import(idopt,d) -> ()
    | Def_add_method(mml,procs) ->
        procs |> List.iter (it.procedure it)
    | Def_replace_method(mml, procs) ->
        procs |> List.iter (it.procedure it)
    | Def_add_notes(id, method_opt, notes_body) ->
        notes_body |> List.iter (it.note it)
    | Def_constant(isuni, id1, id2, dim, atom_opt, comment_opt) ->
        dim |> (it.dimension it);
    | Def_atom(isuni, id1, id2, dim, atom_opt,
               names_opt, statements, methods_opt) ->
        dim |> (it.dimension it);
        names_opt |> Option.iter (List.iter (it.fname it));
        statements |> List.iter (it.statement it);
        methods_opt |> Option.iter (List.iter (it.procedure it))
    | Def_model(isuni, id, params_opt, wheres_opt,
                refines_opt, stats, methods_opt) ->
        params_opt |> Option.iter (List.iter (it.statement it));
        wheres_opt |> Option.iter (List.iter (it.statement it));
        refines_opt |> Option.iter (it.model_refines it);
        stats |> List.iter (it.statement it);
        methods_opt |> Option.iter (List.iter (it.procedure it))
    | Def_definition(id, stats, methods_opt) ->
        stats |> List.iter (it.statement it);
        methods_opt |> Option.iter (List.iter (it.procedure it))
    | Def_patch(id1, id2, stats, methods_opt) ->
        stats |> List.iter (it.statement it);
        methods_opt |> Option.iter (List.iter (it.procedure it))
    | Def_units(units) ->
        units |> List.iter (it.unit_def it)
    | Def_global(stats) ->
        stats |> List.iter (it.statement it)
  );

  model_refines = (fun it -> function
    | Model_Refines(id, stats_opt) ->
        stats_opt |> Option.iter (List.iter (it.statement it))
  );

  dimension = (fun it -> function
    | Dim_default       -> ()
    | Dim_wildcard      -> ()
    | Dim_dimexpr(dim)  -> dim |> (it.dimexpr it)
    | Dim_dimensionless -> ()
  );

  dimexpr = (fun it -> function
    | Dimex_var(id)        -> ()
    | Dimex_int(i)         -> ()
    | Dimex_div(dim1,dim2) ->
        dim1 |> (it.dimexpr it);
        dim2 |> (it.dimexpr it)
    | Dimex_mul(dim1,dim2) ->
        dim1 |> (it.dimexpr it);
        dim2 |> (it.dimexpr it)
    | Dimex_pow(dim1,frac) ->
        dim1 |> (it.dimexpr it)
    | Dimex_paren(dim) ->
        dim |> (it.dimexpr it)
  );

  fname = (fun it -> function
    | (name, comment_opt) ->
        name |> (it.name it)
  );

  name = (fun it -> function
    | Name_id(id)        -> ()
    | Name_dot(name,id)  ->
        name |> (it.name it)
    | Name_set(name,set) ->
        name |> (it.name it);
        set |> (it.set it)
    | Name_der(fnames) ->
        fnames |> List.iter (it.fname it)
    | Name_pre(fname) ->
        fname |> (it.fname it)
  );

  note = (fun it -> function
    | Note(symbol, notes) ->
        notes |> List.iter (fun (fnames, braced_text) ->
          fnames |> List.iter (it.fname it)
          )
  );

  unit_def = (fun it -> function
    | Unit(id,braced) -> ()
  );

  procedure = (fun it -> function
    | Method(id,stats) ->
        stats |> List.iter (it.statement it)
  );

  set = (fun it -> function
    | set_items -> List.iter (it.set_item it) set_items
  );

  set_item = (fun it -> function
    | Set_singleton(expr) ->
        expr |> (it.expr it)
    | Set_range(expr1,expr2) ->
        expr1 |> (it.expr it);
        expr2 |> (it.expr it)
  );

  statement = (fun it -> function
    | Stat_aliases_one(fnames,fname) ->
        fnames |> List.iter (it.fname it);
        fname |> (it.fname it)
    | Stat_aliases_many(fnames1,fnames2,fnames3,id1,id2,set_opt) ->
        fnames1 |> List.iter (it.fname it);
        fnames2 |> List.iter (it.fname it);
        fnames3 |> List.iter (it.fname it);
        set_opt |> Option.iter (it.set it)
    | Stat_arealike(fnames) ->
        fnames |> List.iter (it.fname it)
    | Stat_arethesame(fnames) ->
        fnames |> List.iter (it.fname it)
    | Stat_assert(expr) ->
        expr |> (it.expr it)
    | Stat_assign(assignment) ->
        assignment |> (it.assignment it)
    | Stat_blackbox(label, id, input_args, output_args, data_arg_opt) ->
        label |> (it.fname it);
        input_args |> List.iter (it.fname it);
        output_args |> List.iter (it.fname it);
        data_arg_opt |> Option.iter (it.fname it)
    | Stat_call(id,set_opt) ->
        set_opt |> Option.iter (it.set it)
    | Stat_conditional(stats) ->
        stats |> List.iter (it.statement it)
    | Stat_der(fnames) ->
        fnames |> List.iter (it.fname it)
    | Stat_derivative(fnames,fname_opt) ->
        fnames |> List.iter (it.fname it);
        fname_opt |> Option.iter (it.fname it)
    | Stat_event(label_opt,fnames,switch_blocks) ->
        label_opt |> Option.iter (it.fname it);
        fnames |> List.iter (it.fname it);
        switch_blocks |> List.iter (it.switch_block it)
    | Stat_external(id,fnames) ->
        fnames |> List.iter (it.fname it)
    | Stat_fix_and_assign(assignment) ->
        assignment |> (it.assignment it)
    | Stat_fix(fnames) ->
        fnames |> List.iter (it.fname it)
    | Stat_flow(jump) -> ()
    | Stat_for(id,expr,for_direction_opt,for_action,stats) ->
        expr |> (it.expr it);
        stats |> List.iter (it.statement it)
    | Stat_free(fnames) ->
        fnames |> List.iter (it.fname it)
    | Stat_glassbox(label, id, fnames, n, scope_opt) ->
        label |> (it.fname it);
        fnames |> List.iter (it.fname it);
        scope_opt |> Option.iter (it.fname it)
    | Stat_if(expr,stats) ->
        expr |> (it.expr it);
        stats |> List.iter (it.statement it)
    | Stat_if_else(expr,stats1,stats2) ->
        expr |> (it.expr it);
        stats1 |> List.iter (it.statement it);
        stats2 |> List.iter (it.statement it)
    | Stat_independent(fnames) ->
        fnames |> List.iter (it.fname it)
    | Stat_is(fnames,id,id_opt) ->
        fnames |> List.iter (it.fname it)
    | Stat_isa(fnames, type_expr, id_opt, value_opt) ->
        fnames |> List.iter (it.fname it);
        type_expr |> (it.type_expr it);
        value_opt |> Option.iter (it.expr it)
    | Stat_isrefinedto(fnames, type_expr) ->
        fnames |> List.iter (it.fname it);
        type_expr |> (it.type_expr it)
    | Stat_link_ignore(symbol,fnames) ->
        fnames |> List.iter (it.fname it)
    | Stat_link_symbol(symbol,fnames) ->
        fnames |> List.iter (it.fname it)
    | Stat_link_name(fname,fnames) ->
        fname |> (it.fname it);
        fnames |> List.iter (it.fname it)
    | Stat_notes(notes) ->
        notes |> List.iter (it.note it)
    | Stat_option(id,expr) ->
        expr |> (it.expr it)
    | Stat_previous(fnames) ->
        fnames |> List.iter (it.fname it)
    | Stat_relation(label_opt,relation) ->
        label_opt |> Option.iter (it.fname it);
      relation |> (it.relation it)
    | Stat_run(run_target) ->
        run_target |> (it.run_target it)
    | Stat_select(fnames,switch_blocks) ->
        fnames |> List.iter (it.fname it);
        switch_blocks |> List.iter (it.switch_block it)
    | Stat_solve -> ()
    | Stat_solver(id) -> ()
    | Stat_switch(fnames,switch_blocks) ->
        fnames |> List.iter (it.fname it);
        switch_blocks |> List.iter (it.switch_block it)
    | Stat_units(unit_defs) ->
        unit_defs |> List.iter (it.unit_def it)
    | Stat_unlink_symbol(symbol, fnames) ->
        fnames |> List.iter (it.fname it)
    | Stat_unlink_name(fname,fnames) ->
        fname |> (it.fname it);
        fnames |> List.iter (it.fname it)
    | Stat_use(fname) ->
        fname |> (it.fname it)
    | Stat_when(label_opt,fnames, switch_blocks) ->
        label_opt |> Option.iter (it.fname it);
        fnames |> List.iter (it.fname it);
        switch_blocks |> List.iter (it.switch_block it)
    | Stat_while(expr,stats) ->
        expr |> (it.expr it);
        stats |> List.iter (it.statement it)
    | Stat_willbe(fnames,type_expr,id_opt,expr_opt) ->
        fnames |> List.iter (it.fname it)
    | Stat_willbethesame(fnames) ->
        fnames |> List.iter (it.fname it)
    | Stat_willnotbethesame(fnames ) ->
        fnames |> List.iter (it.fname it)
  );

  assignment = (fun it -> function
    | Assign_var(fname, expr) ->
        fname |> (it.fname it);
        expr |> (it.expr it)
    | Assign_const(fname, expr) ->
        fname |> (it.fname it);
        expr |> (it.expr it)
  );

  relation = (fun it -> function
    | Rel_expr(expr) -> expr |> (it.expr it)
    | Rel_max(expr)  -> expr |> (it.expr it)
    | Rel_min(expr)  -> expr |> (it.expr it)
  );

  run_target = (fun it -> function
    | Run_public(fname) ->
        fname |> (it.fname it)
    | Run_hidden(fname1,fname2) ->
        fname1 |> (it.fname it);
        fname2 |> (it.fname it)
  );

  switch_block = (fun it -> function
    | Switch_case(set,stats) ->
        set |> (it.set it);
        stats |> List.iter (it.statement it)
    | Switch_otherwise(stats) ->
        stats |> List.iter (it.statement it)
  );

  type_expr = (fun it -> function
    | TE_var(id)       -> ()
    | TE_call(id, set) -> set |> (it.set it)
  );

  expr = (fun it -> function
    | E_number(n) -> ()
    | E_maxinteger -> ()
    | E_maxreal -> ()
    | E_boolean(b) -> ()
    | E_any -> ()
    | E_symbol(symbol) -> ()
    | E_var(fname) ->
        fname |> (it.fname it)
    | E_bracket(set) ->
        set |> (it.set it)
    | E_binop(binop,expr1,expr2) ->
        expr1 |> (it.expr it);
        expr2 |> (it.expr it)
    | E_unop(unop,expr) ->
        expr |> (it.expr it)      
    | E_satisfied(fname, n_opt) ->
        fname |> (it.fname it)
    | E_sum(set) ->
        set |> (it.set it)
    | E_prod(set) ->
        set |> (it.set it)
    | E_union(set) ->
        set |> (it.set it)
    | E_intersection(set) ->
        set |> (it.set it)
    | E_card(set) ->
        set |> (it.set it)
    | E_choice(set) ->
        set |> (it.set it)
    | E_call(id,expr) ->
        expr |> (it.expr it)
    | E_paren(expr) ->
        expr |> (it.expr it)
  );
}
