CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_XOPEN_SOURCE=500 -g -O0

# Programy
PROGRAMS = conveyor dispatcher main_brickyard truck worker

# Pliki nagłówkowe
HEADERS = brickyard.h

# Reguły
all: $(PROGRAMS)

conveyor: conveyor.o
	$(CC) $(CFLAGS) -o $@ $^

dispatcher: dispatcher.o
	$(CC) $(CFLAGS) -o $@ $^

main_brickyard: main_brickyard.o
	$(CC) $(CFLAGS) -o $@ $^

truck: truck.o
	$(CC) $(CFLAGS) -o $@ $^

worker: worker.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(PROGRAMS) *.o

.PHONY: all clean
