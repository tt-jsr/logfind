CC=gcc
CDEBUG = -g -O0
CRELEASE = -O3
CPPFLAGS = $(CDEBUG) -I. -L.
#CPPFLAGS = $(CRELEASE) -I. -L.
LDFLAGS=-g
LIBS = -lstdc++ -lreadline -lz -lpthread
DEPS = file.h aho_context.h buffer.h lru_cache.h application.h linebuf.h lru_cache.h pattern_actions.h actions.h parse.h utilities.h list_cmd.h cat_cmd.h _acism.h acism.h msutil.h

SRC	= main.cpp file.cpp buffer.cpp aho_context.cpp lru_cache.cpp application.cpp pattern_actions.cpp actions.cpp parse.cpp utilities.cpp list_cmd.cpp cat_cmd.cpp acism.cpp acism_dump.cpp msutil.cpp acism_create.cpp acism_file.cpp

OBJS	= $(SRC:.cpp=.o) 

logfind : $(OBJS) $(DEPS)
	$(CC) $(CPPFLAGS) -o $@ $(OBJS) $(LIBS)

clean: 
	rm $(OBJS) logfind

