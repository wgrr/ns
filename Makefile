SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

CFLAGS = -Wall -Werror -Wextra -pedantic

AOUT = ns

all: $(AOUT)

$(OBJ): $(SRC)
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

$(AOUT): $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $^
