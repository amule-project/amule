# c aMule statistics (cas) Makefile
# Writen on 11th of March 2004

FLAGS=-g -O2 -Wall
PREFIX=/usr/bin
INSTALL = /bin/install 

cas : cas.o 
	gcc -o cas cas.o

cas.o : cas.c
	gcc -c cas.c ${FLAGS}
	
clean :
	rm cas *.o

install :
	make
	${INSTALL} cas ${PREFIX}

uninstall :
	rm /usr/bin/cas
