# Time at Enklave

Enklave is the co-working space where I am refreshing and modernizing my C++ knowledge. For every check-in and check-out at Enklave I receive an e-mail reflecting my presence.

This C++ example project computes the time I spend in the co-working space. For that I download all my mails to a folder and spin up this program.

## Prerequisites
* TODO Technical prerequisites.
    * Cmake
    * Compiler versions
    * date.h
* Download all e-mails to one folder.

## Compile and run
* TODO

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
* Grab TODOs
* Pass path by cmd-arg.
* Open relevant files and search for sender.
* Ensure project and tests can be build and run from cmd.
    * https://stackoverflow.com/questions/54678817/compile-run-clion-project-from-terminal

## Decisions
* I use std::optional to return from most functions. std::expected is not yet there. I 