CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

SRC = src/sim-virtual.c
TARGET = exec/sim-virutal

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p exec
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
