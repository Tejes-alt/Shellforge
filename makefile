CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -O2 -g
CPPFLAGS ?= -Iinclude
LDLIBS ?= -lreadline
TARGET = shellforge
SRC = src/background.c src/builtin.c src/executor.c src/job_control.c src/jobs.c src/main.c src/parser.c src/util.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) test.txt sf_test_output.txt

.PHONY: all clean test

test: $(TARGET)
	bash tests/test_noninteractive.sh
