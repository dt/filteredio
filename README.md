# Filtered IO

Dynamic library shim to intercept and filter posix IO calls.

A whitelist file specifies a path prefix and a list of entires.
Attempts or open paths matching the prefix but not in the list will fail, and attempts to list directories matching the prefix will only files if they appear in the list.

## Demo
    ./build.sh
    ./test.sh

## ENV vars to run a program with filtered IO
* `DYLD_INSERT_LIBRARIES=/absolute/path/to/filterio.dylib`
* `FILTERIO_WHITELIST=/absolute/path/to/whitelist`

## Whitelist file format
* first list is a prefix (items not matching are never filtered)
* one absolute path per line
