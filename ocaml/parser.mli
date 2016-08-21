val tokenize: in_channel -> Grammar.token Sequence.t
val parse: in_channel -> Types.program

val print_tokens: out_channel -> Grammar.token Sequence.t -> unit
