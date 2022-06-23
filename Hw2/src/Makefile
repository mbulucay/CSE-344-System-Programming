
run: main.o vector.o syscall_wrappers.o
	gcc -c child.c
	gcc child.o syscall_wrappers.o vector.o -lm -o ch
	gcc main.o syscall_wrappers.o vector.o -lm -o processP
	



childrun: vector.o syscall_wrappers.o
	gcc -c child.c
	gcc child.o syscall_wrappers.o vector.o -lm -o ch
	./ch 424236764196452654333162264798 & ./ch abcb1ce5gk9ltno123456er0eqwasd &



main.o: main.c
	gcc -c $<

syscall_wrappers.o:	syscall_wrappers.c
	gcc -c $<

vector.o: vector.c
	gcc -c $<



rm: 
	rm *.o result

clean:
	rm *.o result ch processP

memo:
	valgrind --leak-check=yes --track-origins=yes ./processP -i numbers -o result

