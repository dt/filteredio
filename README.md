# Filtered IO

Dynamic library shim to intercept and filter posix IO calls.

A whitelist file specifies a path prefix and a list of entires.
Attempts or open paths matching the prefix but not in the list will fail, and attempts to list directories matching the prefix will only files if they appear in the list.

## Demo (OSX)
    make clean && make osx && ./test/mac-java.sh
    make clean && make osx && ./test/mac-scala.sh

## Demo (Linux)
    make clean && make linux && ./test/linux-java.sh

## ENV vars to run a program with filtered IO
* `FILTERIO_WHITELIST=/absolute/path/to/whitelist`
* `DYLD_INSERT_LIBRARIES=/absolute/path/to/filterio.dylib` (on OSX)
* `LD_PRELOAD=/absolute/path/to/filterio.dylib` (on Linux)


## Whitelist file format
* first list is a prefix (items not matching are never filtered)
* one absolute, real path per line

## TODO
* Stop using realpath, allow absolute, non-realpaths
* share whitelist between subprocesses
