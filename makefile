osx:
	clang++ -ansi -Wall -Wextra -pedantic -Werror -dynamiclib -o bin/filterio.dylib src/osx.cpp

linux:
	g++ -ansi -Wall -Wextra -Werror -fPIC -shared -std=c++0x -o bin/filterio.so src/linux.cpp

all: osx linux

clean:
	rm -rf bin/* outdir/cp/demo