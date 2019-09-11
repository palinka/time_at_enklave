# Time at Enklave

Enklave is the co-working space where I am refreshing and modernizing my C++ knowledge. For every check-in and check-out at Enklave I receive an e-mail reflecting my presence.

This C++ example project computes the time I spend in the co-working space. For that I download all my mails to a folder and spin up this program.

## Prerequisites
Works with:
* cmake version 3.15.2
* gcc version 7.4.0
* TODO Dependencies
    * google test
    * date.h

## Compile and run
```
# Compile
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

## What is should do (Algorithm)
* Scan a folder containing for exported emails as files.
* Parse all files for the following pattern:
    * Sender name: "Enklave"
    * Subject contains: "Check in" or "Check out"
* Based on the timestamp of the matched e-mails, compute how much time I spend at Enklave.

## TODOs
* Can I push date.h?
* Evolve program and reflect that with different tags.
    * see CppCon 2018: Jason Turner “Applied Best Practices”.
    * ranges
    * multithreading / coroutines
    * Eventually set-up continious integrations, e.g. conan + travis
* Open relevant files and search for sender.
* Grab TODOs

