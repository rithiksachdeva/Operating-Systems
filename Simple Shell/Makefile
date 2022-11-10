SRC = *.c
OBJ = $(SRC:.c=.o)
BIN = sshell
CFLAGS = -Wall -Werror

all: $(BIN)

$(BIN): $(OBJ)
	gcc $^ $(CFLAGS) -o $@

%.o: %.c
	gcc -c $^ $(CFLAGS) -o $@

clean: 
	rm sshell *.o
