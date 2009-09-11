CFLAGS = -std=c99
CC = gcc

all: csfx

sfxdata.o: sfxdata.rc
	windres $^ $@

main.o: %.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

csfx: main.o sfxdata.o
	$(CC) -o $@ $^ -lcomctl32

clean:
	$(RM) *.o csfx.exe