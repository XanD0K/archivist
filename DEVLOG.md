# Development Log

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