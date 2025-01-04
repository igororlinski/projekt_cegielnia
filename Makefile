CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Pliki źródłowe
SRCS = conveyor.c dispatcher.c main_brickyard.c truck.c worker.c

# Pliki nagłówkowe
HEADERS = brickyard.h


OBJS = $(SRCS:.c=.o)
TARGET = main_brickyard

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Czyszczenie plików wynikowych
clean:
	rm -f $(OBJS) $(TARGET)