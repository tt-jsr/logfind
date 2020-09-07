CC=gcc
CDEBUG = -g -O0
CPPFLAGS = $(CDEBUG) -I.
LDFLAGS=-g
LIBS = -lstdc++ 
DEPS = file.h ahocorasick.h aho_queue.h aho_text.h aho_trie.h

SRC	= main.cpp file.cpp  

OBJS  = $(SRC:.cpp=.o) ahocorasick.o aho_queue.o aho_trie.o

logfind : $(OBJS) $(DEPS)
	$(CC) $(CPPFLAGS) -o $@ $(OBJS) $(LIBS)

ahocorasick.o: ahocorasick.c 
	$(CC) $(CPPFLAGS) -c ahocorasick.c -o ahocorasick.o 

aho_queue.o: aho_queue.c 
	$(CC) $(CPPFLAGS) -c aho_queue.c -o aho_queue.o 

aho_trie.o: aho_trie.c
	$(CC) $(CPPFLAGS) -c aho_trie.c -o aho_trie.o 

main.o: main.cpp
	$(CC) $(CPPFLAGS) -c main.cpp -o main.o 

file.o: file.cpp
	$(CC) $(CPPFLAGS) -c file.cpp -o file.o 

clean: 
	rm $(OBJS) logfind



