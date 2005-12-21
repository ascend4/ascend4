dnl macro ASC_WISHVAR
dnl --------------------------------------------------------------------
dnl See if we can find a wish shell var.
dnl --------------------------------------------------------------------
AC_DEFUN(ASC_WISHVAR,
[
# asc_wishvar( tcl_library,  $with_wish)
# generate the executable to extract a variable
# from the tickle shell
cat << EOF > wishvar.tmp.tcl
#! $3
# this is a generated file and can be deleted any time.
if {\$argc != 1} {
        puts stderr "\$argv0 needs variable name"
        exit 1
}
set var [[lindex \$argv 0]]
puts [[set \$var]]
exit 0
EOF

#call it
# usage wishvar varname shell-to-test-in
$1=`$3 wishvar.tmp.tcl -- $2`
]
)

