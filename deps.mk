
$(OBJ_DIR)/src-strpool.o : src/strpool.c src/strpool.h src/utils.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/src-parser.o : src/parser.c src/parser.h src/match.h src/strpool.h src/utils.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/src-match.o : src/match.c src/match.h src/utils.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/src-main.o : src/main.c src/parser.h src/match.h src/strpool.h src/utils.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

