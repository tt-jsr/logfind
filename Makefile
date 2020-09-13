CC=gcc
CDEBUG = -g -O0
CPPFLAGS = $(CDEBUG) -I.
LDFLAGS=-g
LIBS = -lstdc++ -lreadline
DEPS = file.h ahocorasick.h aho_queue.h aho_text.h aho_trie.h aho_context.h buffer.h lru_cache.h application.h linebuf.h lru_cache.h pattern_actions.h actions.h parse.h utilities.h

SRC	= main.cpp file.cpp buffer.cpp aho_context.cpp lru_cache.cpp application.cpp pattern_actions.cpp actions.cpp parse.cpp utilities.cpp ahocorasick.cpp aho_queue.cpp aho_trie.cpp

OBJS	= $(SRC:.cpp=.o) 

logfind : $(OBJS) $(DEPS)
	$(CC) $(CPPFLAGS) -o $@ $(OBJS) $(LIBS)

clean: 
	rm $(OBJS) logfind



