all: mysh

clean:
	rm -rf mysh

mysh: mysh.cpp
	g++ -o mysh mysh.cpp -Wall
