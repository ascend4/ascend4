_a4_completion_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
_a4_repo_root="$(cd -- "$_a4_completion_dir/../.." && pwd)"
_a4_dev_a4="$_a4_repo_root/a4"
_a4_tilde_a4=""

if [[ $_a4_dev_a4 == "$HOME/"* ]]; then
	_a4_tilde_a4="~${_a4_dev_a4#$HOME}"
fi

_a4_resolve_cmd() {
	local typed=${COMP_WORDS[0]}

	if [[ $typed == '~/'* ]]; then
		printf '%s\n' "${typed/#\~/$HOME}"
		return 0
	fi

	if [[ $typed == */* ]]; then
		printf '%s\n' "$typed"
		return 0
	fi

	if command -v a4 >/dev/null 2>&1; then
		command -v a4
		return 0
	fi

	printf '%s\n' "$_a4_dev_a4"
}

_a4_find_subcommand_index() {
	local i
	for ((i = 1; i <= COMP_CWORD; ++i)); do
		case ${COMP_WORDS[i]} in
			open|gui|run|int|script|cutest|completion|units|env|q)
				printf '%s\n' "$i"
				return 0
				;;
		esac
	done
	return 1
}

_a4_find_model_file() {
	local subcmd_index=$1
	local i word expect=

	for ((i = subcmd_index + 1; i < COMP_CWORD; ++i)); do
		word=${COMP_WORDS[i]}

		if [[ -n $expect ]]; then
			case $expect in
				print)
					if [[ $word == -* ]]; then
						expect=
					else
						continue
					fi
					;;
				*)
					expect=
					continue
					;;
			esac
		fi

		case $word in
			-m|--model|-e|--engine|-s|--start|-d|--duration|-u|--units|-o|--output|-r|--run-method|--steps|--microstates)
				expect=arg
				continue
				;;
			-p|--print)
				expect=print
				continue
				;;
			-*)
				continue
				;;
			*)
				printf '%s\n' "$word"
				return 0
				;;
		esac
	done

	return 1
}

_a4_query() {
	local cmd=$(_a4_resolve_cmd) || return 1
	"$cmd" complete "$@" 2>/dev/null
}

_a4_complete_files() {
	local cur=$1
	COMPREPLY=( $(compgen -f -- "$cur") )
	compopt -o filenames 2>/dev/null
}

_a4_complete_models() {
	local file=$1
	local cur=$2
	COMPREPLY=( $(_a4_query models "$file" "$cur") )
}

_a4_complete_cutest_names() {
	local cur=$1
	COMPREPLY=( $(_a4_query cutest "$cur") )
}

_a4_complete_cutest_tflag() {
	local cur=$1
	local prefix=${cur#-t}
	local reply
	COMPREPLY=()
	for reply in $(_a4_query cutest "$prefix"); do
		COMPREPLY+=( "-t$reply" )
	done
}

_a4_complete() {
	local cur prev subcmd_index subcmd
	cur=${COMP_WORDS[COMP_CWORD]}
	prev=
	if (( COMP_CWORD > 0 )); then
		prev=${COMP_WORDS[COMP_CWORD - 1]}
	fi

	if ! subcmd_index=$(_a4_find_subcommand_index); then
		COMPREPLY=( $(compgen -W "--debug --gdb --gdb1 --valgrind --callgrind open gui run int script cutest completion units env q" -- "$cur") )
		return 0
	fi

	subcmd=${COMP_WORDS[subcmd_index]}

	case $subcmd in
		open|run|int)
			if [[ $prev == '-m' || $prev == '--model' ]]; then
				local model_file
				model_file=$(_a4_find_model_file "$subcmd_index") || return 0
				_a4_complete_models "$model_file" "$cur"
				return 0
			fi

			if [[ $COMP_CWORD -eq $((subcmd_index + 1)) ]]; then
				_a4_complete_files "$cur"
				return 0
			fi

			if [[ -z $cur && $prev != -* ]]; then
				if [[ $subcmd == 'open' ]]; then
					COMPREPLY=( $(compgen -W "-m --model" -- "$cur") )
				else
					COMPREPLY=( $(compgen -W "-m --model -i --integrate --int -e --engine -s --start -d --duration --steps -u --units -o --output --plot --microstates -r --run-method -p --print -n --no-test" -- "$cur") )
				fi
				return 0
			fi

			case $cur in
				-*)
					if [[ $subcmd == 'open' ]]; then
						COMPREPLY=( $(compgen -W "-m --model" -- "$cur") )
					else
						COMPREPLY=( $(compgen -W "-m --model -i --integrate --int -e --engine -s --start -d --duration --steps -u --units -o --output --plot --microstates -r --run-method -p --print -n --no-test" -- "$cur") )
					fi
					return 0
					;;
			esac
			;;
		cutest)
			if [[ $prev == '-t' || $prev == '--list-tests' ]]; then
				_a4_complete_cutest_names "$cur"
				return 0
			fi

			if [[ $prev == '-e' || $prev == '--except' ]]; then
				_a4_complete_cutest_names "$cur"
				return 0
			fi

			if [[ $cur == -t* && $cur != '-t' ]]; then
				_a4_complete_cutest_tflag "$cur"
				return 0
			fi

			if [[ $cur == -* ]]; then
				COMPREPLY=( $(compgen -W "-v --verbose -s --silent -n --normal -r --run -e --except --on-error= --help -l --list-suites -t --list-tests" -- "$cur") )
				return 0
			fi

			local names
			names=$(_a4_query cutest "$cur")
			COMPREPLY=( $(compgen -W "$names" -- "$cur") )
			return 0
			;;
	esac

	COMPREPLY=()
}

complete -o bashdefault -o default -F _a4_complete ./a4
complete -o bashdefault -o default -F _a4_complete "$_a4_dev_a4"
if [[ -n $_a4_tilde_a4 ]]; then
	complete -o bashdefault -o default -F _a4_complete "$_a4_tilde_a4"
fi
complete -o bashdefault -o default -F _a4_complete a4
