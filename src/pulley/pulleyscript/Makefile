CC=gcc
CFLAGS=-ggdb3 -DDEBUG

all: pulleyscript compiler
	@echo Build successful

compiler: parser.o lexhash.o bitset.o variable.o condition.o generator.o driver.o resist.o squeal.o compiler.o
	$(CC) $(CFLAGS) -o $@ parser.o lexhash.o bitset.o variable.o condition.o generator.o driver.o resist.o squeal.o compiler.o -ll -lsqlite3

pulleyscript: parser.o lexhash.o bitset.o variable.o condition.o generator.o driver.o resist.o squeal.o
	$(CC) $(CFLAGS) -o $@ parser.o lexhash.o bitset.o variable.o condition.o generator.o driver.o resist.o squeal.o -ll -lsqlite3

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
