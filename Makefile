CC = clang
CFLAGS= -std=c99 -D_DEFAULT_SOURCE -Wall -Wconversion -pedantic

SRC = src
OBJS = $(SRC)/main.o $(SRC)/mines.o
DEPS = $(OBJS:.o=.d)

.PHONY: clean


cursed-mines: $(OBJS)
	$(CC) $(CFLAGS) -lcurses -o $@ $(OBJS)

$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -MMD -c -o $@ $<

clean:
	rm -f cursed-mines
	rm -f src/*.o
	rm -f src/*.d

-include $(DEPS)

