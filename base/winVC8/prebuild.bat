cd ..\generic\compiler
del scanner.c
del ascParse.h
del ascParse.c
flex -Pzz_ -t scanner.l >scanner.c
bison -d -o ascParse.c ascParse.y
sed -i -e "s/yy/zz_/g" scanner.c
sed -i -e "/#ifndef YYSTYPE/,/#endif/d" -e "s/yy/zz_/g" -e "s/YY/ZZ_/g"  ascParse.c
sed -i -e "s/yy/zz_/g" -e "s/YY/ZZ_/g" ascParse.h