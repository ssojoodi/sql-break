all: breaksql

breaksql:
	mkdir -p build
	gcc -Wall -Wextra -o build/breaksql breaksql.c

test: breaksql
	bash test.sh

clean:
	rm -f build/breaksql
