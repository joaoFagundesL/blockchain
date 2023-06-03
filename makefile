all:
	gcc -Wall -Wextra main.c blockchain.c blockchain.h mtwister.c -lcrypto -o main -g -O2

run:
	./main

clean:
	echo Removing files...
	rm -rf main *.txt *.bin

