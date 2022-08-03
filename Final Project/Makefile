build:
	mpicxx -fopenmp -c main.c -o main.o
	mpicxx -fopenmp -c cFunctions.c -o cFunctions.o
	mpicxx -fopenmp -o runFile main.o cFunctions.o

clean:
	rm -f *.o ./runFile
	rm -f output.txt

run:
	mpiexec -np 2 ./runFile
