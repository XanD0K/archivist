# TODO


## New features
- [ ] Implement `list` feature
- [-] `list` feature → implement comparation functions for name and version, and get rid of `alphasort` and `versionsort`
- [ ] `list` feature → also prints in horizontal direction
- [ ] Implement `rename` feature
- [ ] Implement `backup` feature
- [-] Implement `help` feature
- [ ] Implement `log` feature
- [ ] Implement `delete` feature
- [ ] Implement `duplicate` feature
- [-] Implement `move` feature
- [ ] Implement `restore` feature
- [ ] Helper function to get full path from a file/directory
- [ ] Add more fields in the `search` feature output
- [ ] Add `qsort()` on `report` functionality

## Refactoring tasks
- [ ] Change `validade_command` to use `for` loop instead of `bsearch()`
- [ ] Change `bsearch()` to my own implementation of a Binary Search
- [ ] Modular function to create and validate path

## Testing


## BACKLOG


## DONE
- [x] Create Makefile
- [x] Validate commands and arguments
- [x] Improve commands and arguments verification by using Dispatch Table
- [x] `list` feature → implement comparation functions for sorting method
- [x] `list` feature → implement `check_element()` to update `f_counter`, `dir_counter`, `slink_counter`, `error_counter` and `total_size` variables
- [x] `list` feature → allow `argv[4]` as ASC and DESC order
- [x] Descentralize `commands.h` and create a `.h` file for each functionality
- [x] Use `getopt_long()` to allow flexible CLI arguments
- [x] Implement help flag for `list` functionality
- [x] Improve directory validation and kept `.` as default
- [x] Implement `search` feature
- [x] Add `human-readable` flag to `list` and `report` functionalities
- [x] Refactor `formatted_output()` to outputs the formated size, instead of the whole output's message
- [x] Implement `report` feature
- [x] Implement `tree` feature