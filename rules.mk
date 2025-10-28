obj/src-strpool.o: src/strpool.c src/strpool.h src/utils.h
	$(CC) $(CFLAGS) -c -o $@ src/strpool.c
obj/src-parser.o: src/parser.c src/parser.h src/match.h src/strpool.h src/utils.h
	$(CC) $(CFLAGS) -c -o $@ src/parser.c
obj/src-match.o: src/match.c src/match.h src/utils.h
	$(CC) $(CFLAGS) -c -o $@ src/match.c
obj/src-main.o: src/main.c src/parser.h src/match.h src/strpool.h src/utils.h
	$(CC) $(CFLAGS) -c -o $@ src/main.c
