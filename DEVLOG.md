# Development Log

## [DATE]
**Plans**

**Challenges**

**Progress**


## [DATE]
**Plans**
- Implement `delete` feature
**Challenges**

**Progress**


## [2026-04-01]
**Plans**
- Implement `move` feature

**Challenges**
- Comprehend the order in which recursion should occur → one directory must be created before moving to the next subdirectory
- Understand which filters should be applied to directories and which one should be applied to files

**Progress**
- Created `FilterOptions` structure with filter flags, currently being used in `search`, and will also be implemented on `move`
- Created `get_valid_destination()`, that recursivelly creates the destination directory
- For every directory created on destinarion, I check path's size to prevent overflow and truncation with `snprintf()`
- Fully implemented `move` feature by improving UX with `dry-run`, `interactive` and `verbose` flags


## [2026-03-29]
**Plans**
- Implement `tree` feature

**Challenges**
- Understand how to propagate prefix across multiple subdirectories

**Progress**
- `tree` functionality is just a good combination of style and recursiveness. First I made recursion works, and just then I included the symbols (`├──` `└──` `│`) and indentations, always propagating to next subdirectory
- `tree` functionality fully implemented


## [2026-03-27]
**Plans**
- Implement `report` feature

**Challenges**
- Finding the best data structure to keep track of all extensions efficiently
- Free memory on all cases, specially when using `strdup()` and `asprintf()`
- Still confusing dealing with different types

**Progress**
- First thought was to use an ordered linked list to manage all extensions
- Although it seemed like a solid idea, Grok (xAI) suggested using a Dynamic Array instead
- On pset5 'speller', among all implementations I've made, one of them was a Dynamic Hash Table, that doubled its size when a specific factor was reached. Used the same strategy to implement a Dynamic Array that holds every found extension
- I first tried an array of pointers to the `Extension` struct (`Extension **ext`)
- Then I decided to move to an array of `Extension` structs (`Extension *ext`), keeping code cleaner and memory allocation simpler
- Fixed problems with memory leak specifically with `strdup()` and `asprintf()`. I didn't know I should `free()` strings returned by those functions:
    - https://manual.cs50.io/3/strdup
    - https://manual.cs50.io/3/asprintf
- Fixed problems when comparing and making operations with different types: `size_t` with `int`, `const char *` with `char *`, `ssize_t` with `size_t`. Needs to pay more attention about the types I declare and the casts needed to make all operations work.
- `report` feature is fully implemented


## [2026-03-24]
**Plans**
- Implement `search` feature

**Challenges**
- Implementing multiple flags, applied to recursive elements
- Deciding the best way to call each flag to each element, especially when `recursive` flag was `true`

**Progress**
- First, I was thinking of getting all elements into an `char **array`, and start removing them based on chosen flags
- Then I decided the best way of implementing a search feature was to check each element at a time, printing only those who passed all checks
- Didn't struggle with CLI arguments parser, help message, declarations, etc., especially after implementing the `list` feature


## [2026-03-20]
**Plans**
- Improve CLI arguments validations
- Implement `list` feature

**Challenges**
- Learn how to use `getopt_long()` to allow multiple flags in no fixed order and a better flags parsing/validation

**Progress**
- Changed from manual CLI arguments parser and validation to an implementation that uses `getopt_long()`
- Learned how to structure and build a CLI parser with `getopt_long()`:
    - Michael Kerrisk's Linux manual page: https://man7.org/linux/man-pages/man3/getopt.3.html
- Fully implemented `list` feature
- Successfully implemente CLI arguments parser with `getopt_long()`


## [2026-03-17]
**Plans**
- Implement commands and arguments verification

**Challenges**
- Find the best way to make validations
- Segregate validation from `main.c`, keeping it cleaner
- Implement Dispatch Table

**Progress**
- First implemented validation in `main.c`, with auxiliar functions in `utils.c`
- Implemented Binary Search (`bsearch()`) in an array to search for command's index, and used `switch()` to improve validation
- Switched to a Dispatch Table, which included name, command (function) and minimum and maximum number of arguments
- Fully implemented command and arguments validation by using Dispatch Table
- Fully customized error messages (`errno` and `fprintf(stderr, ...)`)
- Segregated commands logic into a segregate file (`commands.c`), keeping `main.c` cleaner


## [2026-03-15]
**Plans**
- Create a Makefile for Final Project `Archivist`

**Challenges**
- Didn't know how to build a Makefile
- Didn't know nothing about its syntaxes, and organization - e.g. wildcard, dependecy files (.d), targets, variables, flags

**Progress**
- Spent the last 3 days studying Makefile:
    - Colby College tutorial: https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
    - GNU Make Tutorial: https://www.gnu.org/software/make/manual/make.html#How-Make-Works
    - Help from Grok (xAI) with all this process
- Learned how to build a professional Makefile
- Finished Makefile implementation for `Archivist`