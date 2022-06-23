run:
	gcc main.c -o hw5 -lrt -lpthread -pthread -lm -Wall -g

clean:
	rm hw5

memo:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./hw5 -i data1 -j data2 -o output.csv -n 3 -m 2

memo2:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./hw5 -i data1 -j data2 -o output.csv -n 3 -m 4

memo3:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./hw5 -i data1 -j data2 -o output.csv -n 4 -m 4

memo4:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./hw5 -i data1 -j data2 -o output.csv -n 4 -m 8

memo5:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./hw5 -i data1 -j data2 -o output.csv -n 5 -m 4

run_n5m2:
	./hw5 -i data1 -j data2 -o output.csv -n 5 -m 2

run_n5m4:
	./hw5 -i data1 -j data2 -o output.csv -n 5 -m 4

run_n5m8:
	./hw5 -i data1 -j data2 -o output.csv -n 5 -m 8

run_n5m16:
	./hw5 -i data1 -j data2 -o output.csv -n 5 -m 16
