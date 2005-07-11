CFLAGS = -g -Wall
LDFLAGS = -g

all: php

php: php_syntree.o php_parser.o php_lexer.o php_core_lib.o
	gcc ${LDFLAGS} -o php php_syntree.o php_parser.o php_lexer.o php_core_lib.o -lstdc++

php_lexer.c: php_lexer.l
	lex -F -ophp_lexer.c php_lexer.l


php_lexer.o: php_lexer.c
	gcc -c -o php_lexer.o ${CFLAGS} php_lexer.c

php_parser.c: php_parser.y
	bison --debug -t -d -v -o php_parser.c php_parser.y

php_parser.o: php_parser.c
	gcc -c -o php_parser.o ${CFLAGS} -DYACC_MSG -DYYDEBUG=1 php_parser.c

php_syntree.o: php_syntree.cpp php_syntree.h
	gcc -c -o php_syntree.o ${CFLAGS} php_syntree.cpp

php_core_lib.o: php_core_lib.cpp php_core_lib.h php_syntree.h
	gcc -c -o php_core_lib.o ${CFLAGS} php_core_lib.cpp

clean:
	rm -f php_lexer.c php_parser.c *.o
