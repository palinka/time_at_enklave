# Time at Enklave

Enklave is the co-working space where I am refreshing and modernizing my C++ knowledge. For every check-in and check-out at Enklave I receive an automated e-mail reflecting my presence.

This C++ example project computes the time I spend in the co-working space. For that it scans a local folder containing the exported mails, filters out the relevant ones and computes the total time.

## Compile and run

### Linux
```
# Compile

# cmake --version
# cmake version 3.15.2

# g++ --version
# g++ (Ubuntu 9.1.0-2ubuntu2~18.04) 9.1.0

mkdir build && cd build
cmake ../ && make

# Run tests
./time_at_enklave_tests

# Run program
./time_at_enklave
```

The program will use some test-files contained in 'tests/data/' by default. However, just pass a folder to another location like this:

```
./time_at_enklave /some/other/path
```

### Windows
Use CMake to generate a Visual Studio project; tested with Visual Studio 2019.

### CLion

This repo contains the '.idea' folder used by the IDE CLion.

# Dependencies and 3rd party

[date.h](include/date.h) is included in folder 'includes/' for convenience. See [github](https://github.com/HowardHinnant/date) for original source.

## TODOs
* Get rid of config-file.
* Find a better solution for the filesystem header and include spread across files.
* If a check-in / check-out pair does not match, because a file is missing, the program exits instead of computing the other pairs.
* Check compiler versions with cmake, e.g. gcc 8.0 is required for filesystem.
* Add concurrency?
* Remove unnecessary std::
* Ensure passing by ref is really not required.
* Grab TODOs

## Some learnings
* 'using' is not a type definition, but a type alias. So it can't be combined in an elegant way with variants if two aliases use the same type. Unluckily the compiles informs about this not when declaring the variant, but when std::get<type>'ing the value.
