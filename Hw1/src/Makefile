
hw1: main.o string_manipulation.o syscall_wrappers.o lib.o structs.o
	gcc main.o string_manipulation.o syscall_wrappers.o lib.o structs.o -o hw1

main.o: main.c
	gcc -c main.c

lib.o: lib.c
	gcc -c lib.c 

structs.o: structs.c
	gcc -c structs.c

syscall_wrappers.o:	syscall_wrappers.c
	gcc -c syscall_wrappers.c

string_manipulation.o: string_manipulation.c
	gcc -c string_manipulation.c

clean:
	rm *.o