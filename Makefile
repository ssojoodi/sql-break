all: breaksql

breaksql:
	gcc -o build/breaksql breaksql.c

clean:
	rm -f build/breaksql
