# Time at Enklave
Enklave is the co-working space where I am working on refreshing and modernizing my C++ knowledge. For every check-in and check-out at Enklave I receive an automated email which confirms my presence.

This C++ example project computes the time I spent in the co-working space. It scans a local folder containing the exported emails, filters out the relevant ones and computes the total time spent in Enklave.


## Compile and run

### Linux

```
# Compile

# cmake --version
# cmake version 3.15.2

# g++ --version
# g++ (Ubuntu 9.1.0-2ubuntu2~18.04) 9.1.0

mkdir build && cd build

# Debug
cmake ../

# OR release
cmake -DCMAKE_BUILD_TYPE=Release ../

# Compile
make -j2

# Run tests
./time_at_enklave_tests

# Run program
./time_at_enklave

# Render the documentation
# Output is written to doc/
# Requireds installed doxygen and optional graphviz
doxygen
```

The program will use some test-files contained in 'tests/data/' by default. However, just pass a folder to another location like this:

```
./time_at_enklave /some/other/path
```

### Windows
Use CMake to generate a Visual Studio project; tested once with Visual Studio 2019.

### CLion

This repo contains the '.idea' folder used by the IDE CLion.

## Dependencies and 3rd party

[date.h](include/date.h) is included in folder 'includes/' for convenience. See [github](https://github.com/HowardHinnant/date) for original source.

## Potential new Features
* Implement other strategies to deal with forgotten events.
* For every bought coffee or snack, an email is send. Count this items.
* Actually compute how much money should be charged:
    * Find out how Enklave deals with forgotten events, e.g. a check-out is received, but no check-in happened beforehand et al.
    * Every started hour is charged, so the interval to compute the actual price is "started hours" and not seconds.

## TODOs

### Must have
* Add [[nodiscard]] and reason if subproblems could be solved at compile time.

### Optional
* Check if std::istream_iterator could help with a more FP style for the very imperative 'parse_file'-function.
* Configure a Continuous Integration pipeline such that, e.g. after each push the code gets compiled by various compilers and tests are executed.
* Build system: eventually check minimum installed compiler versions.
* Check if more pedantic compile flags are required.
* Eventually add concurrency / multithreading.
