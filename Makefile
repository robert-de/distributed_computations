build:
	mpicc tema3.c -o tema3 -lm
clean:
	rm tema3
run:
	mpirun --oversubscribe -np 9 ./tema3 17