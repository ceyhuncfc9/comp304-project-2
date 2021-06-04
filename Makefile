all: comp304project2.c 
	gcc -o test comp304project2.c -lpthread

clean: 
	$(RM) test