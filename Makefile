CC=clang
CFLAGS=-Werror=vla -Wshadow -Wswitch-default -std=c11
CFLAG_SAN=-fsanitize=address
DEPS=
OBJ=hashmap.o

hashmap.o: hashmap.c hashmap.h $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(CFLAG_SAN)
	
clean:
	rm *.o
