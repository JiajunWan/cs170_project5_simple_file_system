all: auto_grader

auto_grader: autograder_main.c fs_lib disk_lib
	g++ autograder_main.c fs.o disk.o -o autograder_main

fs_lib: fs.cpp
	g++ -c fs.cpp -o fs.o

disk_lib: disk.c
	g++ -c disk.c -o disk.o

clean:
	rm -f autograder_main *.o
