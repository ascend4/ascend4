import shutil, os, sys

olddir = os.getcwd()
os.chdir(sys.path[0])
shutil.copy("ascParse.c","ascParse_no_yacc.c");
shutil.copy("ascParse.h","ascParse_no_yacc.h");
shutil.copy("scanner.c","scanner_no_lex.c");

