# Target library
lib := libfs.a

OBJ := fs.o disk.o
CC := gcc
CFLAGS := -Wall -Werror -MMD
CFLAGS += -g

all: $(lib)

deps := $(patsubst %.o,%.d,$(OBJ))
-include $(deps)

$(lib): $(OBJ)
	ar rcs $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(lib) $(OBJ) $(deps)