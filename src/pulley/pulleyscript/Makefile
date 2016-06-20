CC=gcc
CFLAGS=-ggdb3 -DDEBUG

SRC=parser.o lexhash.o bitset.o variable.o condition.o generator.o driver.o resist.o squeal.o

# Depending on your Linux distribution, the flex library (providing
# yywrap(), among others) may be called libl or libfl (OpenSUSE).
LIB_FLEX=-ll
# LIB_FLEX=-lfl
LIBS=$(LIB_FLEX) -lsqlite3

all: pulleyscript compiler
	@echo Build successful

compiler: $(SRC) compiler.o
	$(CC) $(CFLAGS) -o $@ parser.o lexhash.o bitset.o variable.o condition.o generator.o driver.o resist.o squeal.o compiler.o $(LIBS)

pulleyscript: $(SRC)
	$(CC) $(CFLAGS) -o $@ parser.o lexhash.o bitset.o variable.o condition.o generator.o driver.o resist.o squeal.o $(LIBS)

anew: clean all

compiler.o: compiler.c
	$(CC) $(CFLAGS) -c -o $@ $<

parser.o: parser.c
	$(CC) $(CFLAGS) -c -o $@ $<

lexhash.o: lexhash.c lexhash.h
	$(CC) $(CFLAGS) -c -o $@ $<

bitset.o: bitset.c bitset.h
	$(CC) $(CFLAGS) -c -o $@ $<

variable.o: variable.c variable.h variable_int.h
	$(CC) $(CFLAGS) -c -o $@ $<

condition.o: condition.c condition.h
	$(CC) $(CFLAGS) -c -o $@ $<

driver.o: driver.c driver.h generator_int.h
	$(CC) $(CFLAGS) -c -o $@ $<

resist.o: resist.c resist.h
	$(CC) $(CFLAGS) -c -o $@ $<

generator.o: generator.c generator.h generator_int.h
	$(CC) $(CFLAGS) -c -o $@ $<

squeal.o: squeal.c
	$(CC) $(CFLAGS) -c -o $@ $<

parser.c: pulley.lex pulley.yacc lexhash.h
	bison pulley.yacc
	flex -o $@ pulley.lex

clean:
	@echo Wiping intermediate results and targets
	rm -f pulley.output pulley.tab.cacc parser.c *.o pulleyscript compiler

tags:
	ctags *.[ch]

test: compiler demo.ply
	./compiler demo.ply
