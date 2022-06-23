run:
	gcc main.c -o hw4 -lrt -lpthread -Wall -g

clean:
	rm hw4

memo2:
	valgrind --leak-check=yes --track-origins=yes ./hw4 -C 4 -N 2 -F data

memo:
	valgrind --leak-check=yes --track-origins=yes ./hw4 -C 10 -N 5 -F data