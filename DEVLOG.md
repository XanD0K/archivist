# Development Log

## [DATE]
**Plans**

**Challenges**

**Progress**


## [DATE]
**Plans**

**Challenges**

**Progress**


## [2026-03-24]
**Plans**
- Implements `search` feature

**Challenges**
- Implementing multiple flags, applied to recursive elements
- Deciding the best way to call each flag to each element, especially when `recursive` flag was `true`

**Progress**
- First, I was thinking of getting all elements into an `char **array`, and start removing them based on chosen flags
- Then I decided the best way of implementing a search feature was to check each element at a time, printing only those who passed all checks
- Didn't struggle with CLI arguments parser, help message, declarations, etc., especially after implementing the `list` feature


## [2026-03-20]
**Plans**
- Improves CLI arguments validations
- Implements `list` feature

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
- Implements commands and arguments verification

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
- Creates a Makefile for Final Project `Archivist`

**Challenges**
- Didn't know how to build a Makefile
- Didn't know nothing about its syntaxes, and organization - e.g. wildcard, dependecy files (.d), targets, variables, flags

**Progress**
- Spent the last 3 days studying Makefile:
    - Colby College tutorial: https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
    - GNU Make Tutorial: https://www.gnu.org/software/make/manual/make.html#How-Make-Works
    - Help from Grok (AI) with all this process
- Learned how to build a professional Makefile
- Finished Makefile implementation for `Archivist`