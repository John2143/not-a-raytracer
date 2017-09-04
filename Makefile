CC=gcc -std=c11
CFLAGS=-Wall -Wextra -pedantic -I.
D=
SOURCES=main.c
EXECUTABLE=raytracer
LIBRARIES=-L. -lSDL2main -lSDL2

OUTPUT=$(D) $(CFLAGS) $(SOURCES) $(LIBRARIES) -o $(EXECUTABLE)

all:
	$(CC) -DDEBUG -g $(OUTPUT)
prod:
	$(CC) -O3 $(OUTPUT)

clean:
	rm $(EXECUTABLE).exe
