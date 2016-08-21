open Sequence
open Types
open Grammar

let ss = Sequence.singleton
let sd = Sequence.doubleton
let sl = Sequence.of_list


let rec do_program = function
  | defs -> do_definitions defs

and do_definitions = function
  | defs -> flat_map do_definition (sl defs)

and do_definition = function
  | Def_require_file(d) ->
      sl [REQUIRE; (DQUOTE d); SEMICOLON]
  | Def_provide_module(d) ->
      sl [PROVIDE; (DQUOTE d); SEMICOLON]
  | Def_import(idopt,d) ->
      let idfrom = match idopt with
        | None     -> empty
        | Some(id) -> sl [IDENTIFIER id; FROM]
      in
      ss IMPORT <+> idfrom <+> sl [DQUOTE d; SEMICOLON]
  | Def_add_method(mml,procs) ->
      sl [ADD; METHODS; IN] <+>
      do_model_modification_location mml <+> ss SEMICOLON <+>
      do_procedures procs <+> sl [END; METHODS; SEMICOLON]
  | Def_replace_method(mml, procs) ->
      sl [REPLACE; METHODS; IN] <+>
      do_model_modification_location mml <+> ss SEMICOLON <+>
      do_procedures procs <+> sl [END; METHODS; SEMICOLON]
  | Def_add_notes(id, method_opt, notes_body) ->
      sl [ADD; NOTES; IN; IDENTIFIER id] <+>
      do_optional_method method_opt <+>
      ss SEMICOLON <+>
      do_notes_body notes_body <+>
      sl [END; NOTES; SEMICOLON];
  | Def_constant(isuni, id1, id2, dim, atom_opt, comment_opt) ->
      do_optional_universal isuni <+>
      sl [CONSTANT; IDENTIFIER id1; REFINES; IDENTIFIER id2] <+>
      do_dimension dim <+>
      do_optional_constant_val atom_opt <+>
      do_optional_comment comment_opt <+>
      ss SEMICOLON
  | Def_atom(isuni, id1, id2, dim, atom_opt, names_opt, statements, methods_opt) ->
      do_optional_universal isuni <+>
      sl [ATOM; IDENTIFIER id1; REFINES; IDENTIFIER id2 ] <+>
      do_dimension dim <+>
      do_optional_default_val atom_opt <+>
      do_optional_like_children names_opt <+>
      ss SEMICOLON <+>
      do_statements statements <+>
      do_optional_methods methods_opt <+>
      sl [END; IDENTIFIER id1; SEMICOLON]
  | Def_model(isuni, id, params_opt, wheres_opt, refines_opt, stats, methods_opt) ->
      do_optional_universal isuni <+>
      sl [MODEL; IDENTIFIER id] <+>
      do_optional_model_parameters params_opt <+>
      do_optional_model_parameter_wheres wheres_opt <+>
      do_optional_model_refines refines_opt <+>
      ss SEMICOLON <+>
      do_statements stats <+>
      do_optional_methods methods_opt <+>
      sl [END; IDENTIFIER id; SEMICOLON]
  | Def_definition(id, stats, methods_opt) ->
      sl [DEFINITION; IDENTIFIER id] <+>
      do_statements stats <+>
      do_optional_methods methods_opt <+>
      sl [END; IDENTIFIER id; SEMICOLON]
  | Def_patch(id1, id2, stats, methods_opt) ->
      sl [PATCH; IDENTIFIER id1; FOR; IDENTIFIER id2; SEMICOLON] <+>
      do_statements stats <+>
      do_optional_methods methods_opt <+>
      sl [END; IDENTIFIER id1; SEMICOLON]
  | Def_units(units) ->
      do_units_block units <+> ss SEMICOLON
  | Def_global(stats) ->
      sl [GLOBAL; SEMICOLON] <+>
      do_statements stats <+>
      sl [END; GLOBAL; SEMICOLON]

and do_model_modification_location = function
  | Model_var(id) -> sl [IDENTIFIER id]
  | Model_def     -> sl [DEFINITION; MODEL]

and do_optional_constant_val = function
  | None -> empty
  | Some(atom) -> ss CASSIGN <+> do_atomic_constant atom

and do_optional_default_val = function
  | None       -> empty
  | Some(atom) -> ss DEFAULT <+> do_atomic_constant atom
 
and do_optional_like_children = function
  | None         -> empty
  | Some(fnames) -> sl [LIKE; CHILDREN] <+> do_fnames fnames


and do_optional_method = function
  | None -> empty
  | Some(id) -> sl [METHOD; IDENTIFIER id]

and do_optional_methods = function
  | None        -> empty
  | Some(procs) -> ss METHODS <+> do_procedures procs

and do_optional_model_parameters = function
  | None        -> empty
  | Some(stats) -> ss LPAREN <+> do_statements stats <+> ss RPAREN

and do_optional_model_parameter_wheres = function
  | None        -> empty
  | Some(stats) -> ss WHERE <+> ss LPAREN <+> do_statements stats <+> ss RPAREN

and do_optional_model_refines = function
  | None ->
      empty
  | Some(Model_Refines(id, None)) ->
      sl [REFINES; IDENTIFIER id]
  | Some(Model_Refines(id, Some(stats))) ->
      sl [REFINES; IDENTIFIER id] <+> ss LPAREN <+> do_statements stats <+> ss RPAREN

and do_optional_universal = function
  | false -> empty
  | true  -> ss UNIVERSAL

and do_atomic_constant = function
  | Atom_number(number_sign,number) -> 
      do_optional_number_sign number_sign <+>
      do_number number
  | Atom_bool(true)     -> ss TRUE
  | Atom_bool(false)    -> ss FALSE
  | Atom_symbol(symbol) -> ss (SYMBOL symbol)


and do_dimension = function
  | Dim_default       -> empty
  | Dim_wildcard      -> ss DIMENSION <+> ss TIMES
  | Dim_dimexpr(dim)  -> ss DIMENSION <+> do_dimexpr dim
  | Dim_dimensionless -> ss DIMENSIONLESS

and do_dimexpr = function
  | Dimex_var(id)        -> ss (IDENTIFIER id)
  | Dimex_int(i)         -> ss (INTEGER i)
  | Dimex_div(dim1,dim2) -> do_dimexpr dim1 <+> ss DIV <+> do_dimexpr dim2
  | Dimex_mul(dim1,dim2) -> do_dimexpr dim1 <+> ss TIMES <+> do_dimexpr dim2
  | Dimex_pow(dim1,frac) -> do_dimexpr dim1 <+> ss CIRCUMFLEX <+> do_fraction frac
  | Dimex_paren(dim)     -> ss LPAREN <+> do_dimexpr dim <+> ss RPAREN

and do_fraction = function
  | Fraction(number_sign,fraction_body) ->
      do_optional_number_sign number_sign <+>
      do_fraction_body fraction_body

and do_fraction_body = function
  | Frac_int(n)    -> sl [INTEGER n]
  | Frac_frac(n,m) -> sl [LPAREN; INTEGER n; DIV; INTEGER m; RPAREN]

and do_fnames = function
  | []            -> assert false (* impossible *)
  | [fname]       -> do_fname fname
  | fname::fnames -> do_fname fname <+> ss COMMA <+> do_fnames fnames

and do_fname = function
  | (name, comment_opt) ->
      do_name name <+>
      do_optional_comment comment_opt

and do_name = function
  | Name_id(id)        -> sl [IDENTIFIER id]
  | Name_dot(name,id)  -> do_name name <+> sl [DOT; IDENTIFIER id]
  | Name_set(name,set) -> do_name name <+> ss LBRACKET <+> do_set set <+> ss RBRACKET
  | Name_der(fnames)   -> ss DERIV <+> ss LPAREN <+> do_fnames fnames <+> ss RPAREN
  | Name_pre(fname)    -> ss PRE <+> ss LPAREN <+> do_fname fname <+> ss RPAREN

and do_optional_comment = function
  | None         -> empty
  | Some(dquote) -> ss (DQUOTE dquote)

and do_notes_body = function
  | notes -> flat_map do_note (sl notes)

and do_note = function
  | Note(symbol, notes) -> ss (SYMBOL symbol) <+> flat_map do_note_value (sl notes)

and do_note_value = function
  | (fnames, bracedtext) -> do_fnames fnames <+> ss (BRACEDTEXT bracedtext)

and do_number = function
  | Number(number_literal, bracedtext_opt) ->
      do_number_literal number_literal <+>
      do_optional_bracedtext bracedtext_opt

and do_number_literal = function
  | Num_integer(i) -> ss (INTEGER i)
  | Num_real(r)    -> ss (REAL r)

and do_optional_number_sign = function
  | Num_nosign -> empty
  | Num_pos    -> ss PLUS
  | Num_neg    -> ss MINUS

and do_units_block = function
  | unit_decls -> 
      ss UNITS <+>
      flat_map do_unit_declaration (sl unit_decls) <+>
      sl [END; UNITS]

and do_unit_declaration = function
  | Unit(id,bracedtext) -> 
      sl [IDENTIFIER id; EQ; BRACEDTEXT bracedtext; SEMICOLON]

and do_procedures = function
  | []          -> empty
  | proc::procs -> do_procedure proc <+> do_procedures procs

and do_procedure = function
  | Method(id,stats) -> 
      sl [METHOD; IDENTIFIER id; SEMICOLON] <+>
      do_statements stats <+>
      sl [END; IDENTIFIER id; SEMICOLON]

and do_set = function
  | []            -> empty
  | [set_item]    -> do_set_item set_item
  | set_item::set -> do_set_item set_item <+> ss COMMA <+> do_set set

and do_set_item = function
  | Set_singleton(expr)    -> do_expr expr
  | Set_range(expr1,expr2) -> do_expr expr1 <+> ss DOTDOT <+> do_expr expr2

and do_statements = function
  | []          -> empty
  | stat::stats -> do_statement stat <+> ss SEMICOLON <+> do_statements stats

and do_statement = function
  | Stat_aliases_one(fnames,fname) ->
      do_fnames fnames <+>
      ss ALIASES <+>
      do_fname fname
  | Stat_aliases_many(fnames1,fnames2,fnames3,id1,id2,set_opt) ->
      do_fnames fnames1 <+>
      ss ALIASES <+>
      ss LPAREN <+> do_fnames fnames2 <+> ss RPAREN <+>
      ss WHERE <+> do_fnames fnames3 <+>
      sl [ISA; IDENTIFIER id1; OF; IDENTIFIER id2] <+>
      do_optional_set_values set_opt
  | Stat_arealike(fnames) ->
      do_fnames fnames <+> ss AREALIKE
  | Stat_arethesame(fnames) ->
      do_fnames fnames <+> ss ARETHESAME
  | Stat_assert(expr) ->
      ss ASSERT <+> do_expr expr
  | Stat_assign(assignment) ->
      do_assignment assignment
  | Stat_blackbox(label, id, input_args, output_args, data_arg_opt) ->
      do_label label <+>
      ss (IDENTIFIER id) <+>
      ss LPAREN <+>
      do_input_args input_args <+>
      ss SEMICOLON <+>
      do_output_args output_args <+>
      do_optional_data_arg data_arg_opt <+>
      ss RPAREN
  | Stat_call(id,set_opt) ->
      (match set_opt with
      | None      -> sl [CALL; IDENTIFIER id]
      | Some(set) -> sl [CALL; IDENTIFIER id; LPAREN] <+> do_set set <+> sl [RPAREN])
  | Stat_conditional(stats) ->
      sl [CONDITIONAL] <+>
      do_statements stats <+>
      sl [END; CONDITIONAL]
  | Stat_der(fnames) ->
      sl [DER; LPAREN] <+> do_fnames fnames <+> sl [RPAREN]
  | Stat_derivative(fnames,fname_opt) ->
      let opt_with = match fname_opt with
        | None        -> empty
        | Some(fname) -> ss WITH <+> do_fname fname
      in
      sl [DERIVATIVE; OF] <+> do_fnames fnames <+> opt_with
  | Stat_event(label_opt,fnames,switch_blocks) -> 
      do_optional_label label_opt <+>
      sl [EVENT; LPAREN] <+> do_fnames fnames <+> sl [RPAREN] <+>
      do_switch_blocks switch_blocks <+>
      sl [END; EVENT]
  | Stat_external(id,fnames) ->
      sl [EXTERNAL; IDENTIFIER id; LPAREN] <+> do_fnames fnames <+> sl [RPAREN]
  | Stat_fix_and_assign(assignment) ->
      ss FIX <+> do_assignment assignment
  | Stat_fix(fnames) ->
      ss FIX <+> do_fnames fnames
  | Stat_flow(jump) ->
      do_jump jump
  | Stat_for(id,expr,for_direction_opt,for_action,stats) ->
      sl [FOR; IDENTIFIER id; IN] <+>
      do_expr expr <+>
      (match for_direction_opt with
        | None -> empty
        | Some(fd) -> do_for_direction fd) <+>
      do_for_action for_action <+>
      do_statements stats <+>
      sl [END; FOR]
  | Stat_free(fnames) ->
      ss FREE <+> do_fnames fnames
  | Stat_glassbox(label, id, fnames, n, scope_opt) ->
      do_label label <+>
      ss (IDENTIFIER id) <+>
      ss LPAREN <+> 
      do_fnames fnames <+>
      ss SEMICOLON <+>
      ss (INTEGER n) <+>
      ss RPAREN <+>
      do_optional_scope scope_opt
  | Stat_if(expr,stats) ->
      ss IF <+> do_expr expr <+> ss THEN <+>
      do_statements stats <+>
      sl [END; IF]
  | Stat_if_else(expr,stats1,stats2) ->
      ss IF <+> do_expr expr <+> ss THEN <+>
      do_statements stats1 <+>
      ss ELSE <+>
      do_statements stats2 <+>
      sl [END; IF]
  | Stat_independent(fnames) ->
      ss INDEPENDENT <+> do_fnames fnames
  | Stat_is(fnames,id,id_opt) ->
      do_fnames fnames <+> sl [IS; IDENTIFIER id] <+> do_optional_of id_opt
  | Stat_isa(fnames, type_expr, id_opt, value_opt) ->
      do_fnames fnames <+>
      ss ISA <+>
      do_type_expr type_expr <+>
      do_optional_of id_opt <+>
      do_optional_with_value value_opt
  | Stat_isrefinedto(fnames, type_expr) ->
      do_fnames fnames <+>
      ss ISREFINEDTO <+>
      do_type_expr type_expr
  | Stat_link_ignore(symbol,fnames) ->
      sl [LINK; LPAREN; IGNORE; COMMA] <+>
      ss (SYMBOL symbol) <+> ss COMMA <+>
      do_fnames fnames <+>
      ss RPAREN
  | Stat_link_symbol(symbol,fnames) ->
      sl [LINK; LPAREN] <+>
      ss (SYMBOL symbol) <+> ss COMMA <+>
      do_fnames fnames <+>
      ss RPAREN
  | Stat_link_name(fname,fnames) ->
      sl [LINK; LPAREN] <+>
      do_fname fname <+> ss COMMA <+>
      do_fnames fnames <+>
      ss RPAREN
  | Stat_notes(notes) ->
      ss NOTES <+> do_notes_body notes <+> sl [END; NOTES]
  | Stat_option(id,expr) ->
      sl [OPTION; IDENTIFIER id] <+> do_expr expr
  | Stat_previous(fnames) ->
      ss PREVIOUS <+> do_fnames fnames
  | Stat_relation(label_opt,relation) ->
      do_optional_label label_opt <+>
      do_relation relation
  | Stat_run(run_target) ->
      ss RUN <+> do_run_target run_target
  | Stat_select(fnames,switch_blocks) ->
      sl [SELECT; LPAREN] <+> do_fnames fnames <+> sl [RPAREN] <+>
      do_switch_blocks switch_blocks <+>
      sl [END; SELECT]
  | Stat_solve ->
      ss SOLVE
  | Stat_solver(id) ->
      sl [SOLVER; IDENTIFIER id]
  | Stat_switch(fnames,switch_blocks) ->
      sl [SWITCH; LPAREN] <+> do_fnames fnames <+> sl [RPAREN] <+>
      do_switch_blocks switch_blocks <+>
      sl [END; SWITCH]
  | Stat_units(unit_defs) ->
      do_units_block unit_defs
  | Stat_unlink_symbol(symbol,fnames) ->
      sl [UNLINK; LPAREN] <+> ss (SYMBOL symbol) <+> ss COMMA <+> do_fnames fnames <+> sl [RPAREN]
  | Stat_unlink_name(fname,fnames) ->
      sl [UNLINK; LPAREN] <+> do_fname fname     <+> ss COMMA <+> do_fnames fnames <+> sl [RPAREN]
  | Stat_use(fname) ->
      ss USE <+> do_fname fname
  | Stat_when(label_opt,fnames, switch_blocks) ->
      do_optional_label label_opt <+>
      sl [WHEN; LPAREN] <+> do_fnames fnames <+> sl [RPAREN] <+>
      do_switch_blocks switch_blocks <+>
      sl [END; WHEN]
  | Stat_while(expr,stats) ->
      ss WHILE <+> do_expr expr  <+> ss DO <+>
      do_statements stats <+>
      sl [END; WHILE]
  | Stat_willbe(fnames,type_expr,id_opt,expr_opt) ->
      do_fnames fnames <+>
      ss WILLBE <+>
      do_type_expr type_expr <+>
      do_optional_of id_opt <+>
      do_optional_with_value expr_opt
  | Stat_willbethesame(fnames) ->
      do_fnames fnames <+> ss WILLBETHESAME
  | Stat_willnotbethesame(fnames ) ->
      do_fnames fnames <+> ss WILLNOTBETHESAME

and do_label = function
  | fname -> do_fname fname <+> ss COLON
  
and do_optional_label = function
  | None        -> empty
  | Some(label) -> do_label label

and do_assignment = function
  | Assign_var  (fname,expr) -> do_fname fname <+> ss ASSIGN <+> do_expr expr
  | Assign_const(fname,expr) -> do_fname fname <+> ss CASSIGN <+> do_expr expr

and do_for_action = function
  | For_check  -> ss CHECK 
  | For_create -> ss CREATE
  | For_do     -> ss DO
  | For_expect -> ss EXPECT

and do_for_direction = function
  | For_increasing -> ss INCREASING
  | For_decreasing -> ss DECREASING

and do_input_args = function
  | fnames -> do_fnames fnames <+> sl [COLON; INPUT]

and do_jump = function
  | Jump_break    -> ss BREAK
  | Jump_continue -> ss CONTINUE
  | Jump_fallthru -> ss FALLTHRU
  | Jump_return   -> ss RETURN
  | Jump_stop(bracedtext_opt) -> ss STOP <+> do_optional_bracedtext bracedtext_opt

and do_relation = function
  | Rel_expr(expr) -> do_expr expr
  | Rel_max(expr)  -> ss MAXIMIZE <+> do_expr expr
  | Rel_min(expr)  -> ss MINIMIZE <+> do_expr expr

and do_run_target = function
  | Run_public(fname)         -> do_fname fname
  | Run_hidden(fname1,fname2) -> do_fname fname1 <+> ss DBLCOLON <+> do_fname fname2

and do_switch_blocks = function
  | blocks -> flat_map do_switch_block (sl blocks)

and do_switch_block = function
  | Switch_case(set,stats)  -> ss CASE <+> do_set set <+> ss COLON <+> do_statements stats
  | Switch_otherwise(stats) -> ss OTHERWISE           <+> ss COLON <+> do_statements stats

and do_optional_data_arg = function
  | None        -> empty
  | Some(fname) -> ss SEMICOLON <+> do_fname fname <+> sl [COLON; DATA]

and do_optional_of = function
  | None -> empty
  | Some(id) -> sl [OF; IDENTIFIER id]

and do_optional_scope = function
    | None        -> empty
    | Some(fname) -> ss IN <+> do_fname fname

and do_optional_set_values = function
  | None -> empty
  | Some(set) -> sl [WITH_VALUE; LPAREN] <+> do_set set <+> sl [RPAREN]

and do_optional_with_value = function
  | None -> empty
  | Some(expr) -> ss WITH_VALUE <+> do_expr expr

and do_output_args = function
  | fnames -> do_fnames fnames <+> sl [COLON; OUTPUT]

and do_type_expr = function
  | TE_var(id)       -> ss (IDENTIFIER id)
  | TE_call(id, set) -> ss (IDENTIFIER id) <+> ss LPAREN <+> do_set set <+> ss RPAREN


and do_expr = function
  | E_number(n) -> do_number n
  | E_maxinteger -> ss MAXINTEGER
  | E_maxreal -> ss MAXREAL
  | E_boolean(true) -> ss TRUE
  | E_boolean(false) -> ss FALSE
  | E_any -> ss ANY
  | E_symbol(symbol) -> ss (SYMBOL symbol)
  | E_var(fname) -> do_fname fname
  | E_bracket(set) -> ss LBRACKET <+> do_set set <+> ss RBRACKET
  | E_binop(binop,expr1,expr2) -> do_expr expr1 <+> do_binop binop <+> do_expr expr2
  | E_unop(unop,expr) -> do_unop unop <+> do_expr expr
  | E_satisfied(fname,Some(n)) ->
      sl [SATISFIED; LPAREN] <+> do_fname fname <+> ss COMMA <+> do_number n <+> sl [RPAREN]
  | E_satisfied(fname,None) ->
      sl [SATISFIED; LPAREN] <+> do_fname fname <+> sl [RPAREN]
  | E_sum(set)          -> ss SUM          <+> ss LBRACKET <+> do_set set <+> ss RBRACKET
  | E_prod(set)         -> ss PROD         <+> ss LBRACKET <+> do_set set <+> ss RBRACKET
  | E_union(set)        -> ss UNION        <+> ss LBRACKET <+> do_set set <+> ss RBRACKET
  | E_intersection(set) -> ss INTERSECTION <+> ss LBRACKET <+> do_set set <+> ss RBRACKET
  | E_card(set)         -> ss CARD         <+> ss LBRACKET <+> do_set set <+> ss RBRACKET
  | E_choice(set)       -> ss CHOICE       <+> ss LBRACKET <+> do_set set <+> ss RBRACKET
  | E_call(id,expr) -> ss (IDENTIFIER id) <+> ss LPAREN <+> do_expr expr <+> ss RPAREN
  | E_paren(expr) -> ss LPAREN <+> do_expr expr <+> ss RPAREN

and do_binop = function
  (* arith operators *)
  | Bin_add -> ss PLUS
  | Bin_sub -> ss MINUS
  | Bin_mul -> ss TIMES
  | Bin_div -> ss DIV
  | Bin_pow -> ss CIRCUMFLEX
  (* bool operators *)
  | Bin_and -> ss AND
  | Bin_or -> ss OR
  (* numeric relations *)
  | Bin_eq -> ss EQ
  | Bin_lt -> ss LT
  | Bin_gt -> ss GT
  | Bin_leq -> ss LEQ
  | Bin_geq -> ss GEQ
  | Bin_neq -> ss NEQ
  (* bool relation *)
  | Bin_beq -> ss BEQ
  | Bin_bne -> ss BNE
  (* ??? *)
  | Bin_in -> ss IN
  | Bin_pipe -> ss PIPE
  | Bin_suchthat -> ss SUCHTHAT

and do_unop = function
  | Un_not -> ss NOT
  | Un_plus -> ss PLUS
  | Un_minus -> ss MINUS


and do_optional_bracedtext = function
  | None         -> empty
  | Some(braced) -> ss (BRACEDTEXT braced)

let tokenize_ast = do_program

