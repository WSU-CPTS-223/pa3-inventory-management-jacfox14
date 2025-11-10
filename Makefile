out: clean compile execute


compile: src/main.cpp
	g++ -g -Wall -std=c++14 src/main.cpp -o mainexe

test: src/tests.cpp
	g++ -g -Wall -std=c++14 src/tests.cpp -o testexe

run-test: test
	./testexe

execute: mainexe
	./mainexe

clean:
	rm -f mainexe