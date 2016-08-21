exception NotImplemented of int

exception LexerError of string
exception SyntaxError of string


(* Separate types to avoid mixing the strings: *)
type identifier = Identifier of string
type bracedtext = BracedText of string
type symbol = Symbol of string
type dquote = DQuote of string

type program = definition list

and definition = 
  | Def_require_file of dquote
  | Def_provide_module of dquote
  | Def_import of identifier option * dquote
  | Def_add_method of model_modification_location * procedure list
  | Def_replace_method of model_modification_location * procedure list
  | Def_add_notes of identifier * identifier option * note list
  | Def_constant of
      bool * identifier * identifier * dimension * atomic_constant option *
      dquote option
  | Def_atom of 
      bool * identifier * identifier * dimension * atomic_constant option *
      fname list option * statement list * procedure list option
  | Def_model of 
      bool * identifier *
      statement list option * statement list option * model_refines option *
      statement list * procedure list option
  | Def_definition of identifier * statement list * procedure list option
  | Def_patch of identifier * identifier * statement list * procedure list option
  | Def_units of unit_def list
  | Def_global of statement list

and model_modification_location =
  | Model_var of identifier
  | Model_def

and model_refines = Model_Refines of identifier * statement list option

and atomic_constant =
  | Atom_number of number_sign * number
  | Atom_bool of bool
  | Atom_symbol of symbol

and dimension =
  | Dim_default
  | Dim_wildcard
  | Dim_dimexpr of dimexpr
  | Dim_dimensionless

and dimexpr =
  | Dimex_var of identifier
  | Dimex_int of int
  | Dimex_div of dimexpr * dimexpr
  | Dimex_mul of dimexpr * dimexpr
  | Dimex_pow of dimexpr * fraction
  | Dimex_paren of dimexpr

and fraction = Fraction of number_sign * fraction_body

and fraction_body =
  | Frac_int of int
  | Frac_frac of int * int

and fname = (name * dquote option)

and name =
  | Name_id of identifier
  | Name_dot of name * identifier
  | Name_set of name * set
  | Name_der of fname list
  | Name_pre of fname

and note = Note of symbol * (fname list * bracedtext) list

and number = Number of number_literal * bracedtext option

and number_literal =
  | Num_integer of int
  | Num_real of float

and number_sign = 
  | Num_nosign
  | Num_pos
  | Num_neg

and unit_def = Unit of identifier * bracedtext

and procedure = Method of identifier * statement list

and set = set_item list

and set_item = 
  | Set_singleton of expr
  | Set_range of expr * expr

and statement = 
  | Stat_aliases_one of fname list * fname
  | Stat_aliases_many of fname list * fname list * fname list * identifier * identifier * set option
  | Stat_arealike of fname list
  | Stat_arethesame of fname list
  | Stat_assert of expr
  | Stat_assign of assignment
  | Stat_blackbox of fname * identifier * fname list * fname list * fname option
  | Stat_call of identifier * set option
  | Stat_conditional of statement list
  | Stat_der of fname list
  | Stat_derivative of fname list * fname option
  | Stat_event of fname option * fname list * switch_block list
  | Stat_external of identifier * fname list
  | Stat_fix_and_assign of assignment
  | Stat_fix of fname list
  | Stat_flow of jump
  | Stat_for of identifier * expr * for_direction option * for_action * statement list
  | Stat_free of fname list
  | Stat_if of expr * statement list
  | Stat_glassbox of fname * identifier * fname list * int * fname option
  | Stat_if_else of expr * statement list * statement list
  | Stat_independent of fname list
  | Stat_is of fname list * identifier * identifier option
  | Stat_isa of fname list * type_expr * identifier option * expr option
  | Stat_isrefinedto of fname list * type_expr
  | Stat_link_ignore of symbol * fname list
  | Stat_link_symbol of symbol * fname list
  | Stat_link_name of fname * fname list
  | Stat_notes of note list
  | Stat_option of identifier * expr
  | Stat_previous of fname list
  | Stat_relation of fname option * relation
  | Stat_run of run_target
  | Stat_select of fname list * switch_block list
  | Stat_solve
  | Stat_solver of identifier
  | Stat_switch of fname list * switch_block list
  | Stat_units of unit_def list
  | Stat_unlink_symbol of symbol * fname list
  | Stat_unlink_name of fname * fname list
  | Stat_use of fname
  | Stat_when of fname option * fname list * switch_block list
  | Stat_while of expr * statement list
  | Stat_willbe of fname list * type_expr * identifier option * expr option
  | Stat_willbethesame of fname list
  | Stat_willnotbethesame of fname list 

and assignment =
  | Assign_var   of fname * expr
  | Assign_const of fname * expr

and for_action = 
  | For_check
  | For_create
  | For_do
  | For_expect

and for_direction = 
  | For_increasing
  | For_decreasing

and jump = 
  | Jump_break
  | Jump_continue
  | Jump_fallthru
  | Jump_return
  | Jump_stop of bracedtext option

and relation = 
  | Rel_expr of expr
  | Rel_max of expr
  | Rel_min of expr

and run_target =
  | Run_public of fname
  | Run_hidden of fname * fname

and switch_block =
  | Switch_case of set * statement list
  | Switch_otherwise of statement list

and type_expr =
  | TE_var of identifier
  | TE_call of identifier * set

and expr = 
  | E_number of number
  | E_maxinteger
  | E_maxreal
  | E_boolean of bool
  | E_any
  | E_symbol of symbol
  | E_var of fname
  | E_bracket of set
  | E_binop of binop * expr * expr
  | E_unop of unop * expr
  | E_satisfied of fname * number option
  | E_sum of set
  | E_prod of set
  | E_union of set
  | E_intersection of set
  | E_card of set
  | E_choice of set
  | E_call of identifier * expr
  | E_paren of expr

and binop =
  (* arith operators *)
  | Bin_add
  | Bin_sub
  | Bin_mul
  | Bin_div
  | Bin_pow
  (* bool operators *)
  | Bin_and
  | Bin_or
  (* numeric relations *)
  | Bin_eq
  | Bin_lt
  | Bin_gt
  | Bin_leq
  | Bin_geq
  | Bin_neq
  (* bool relation *)
  | Bin_beq
  | Bin_bne
  (* ??? *)
  | Bin_in
  | Bin_pipe
  | Bin_suchthat

and unop =
  | Un_not
  | Un_plus
  | Un_minus
