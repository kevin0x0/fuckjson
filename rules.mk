obj/src-strpool.o: src/strpool.c src/strpool.h src/utils.h
	$(CC) $(CFLAGS) -c -o $(CURDIR)/obj/src-strpool.o $(CURDIR)/src/strpool.c
obj/src-parser.o: src/parser.c src/parser.h src/match.h src/strpool.h src/utils.h
	$(CC) $(CFLAGS) -c -o $(CURDIR)/obj/src-parser.o $(CURDIR)/src/parser.c
obj/src-match.o: src/match.c src/match.h src/utils.h
	$(CC) $(CFLAGS) -c -o $(CURDIR)/obj/src-match.o $(CURDIR)/src/match.c
obj/src-main.o: src/main.c src/parser.h src/match.h src/strpool.h src/utils.h
	$(CC) $(CFLAGS) -c -o $(CURDIR)/obj/src-main.o $(CURDIR)/src/main.c
