# TODO


## New features
- [ ] Implement `list` feature
- [-] `list` feature → implement comparation functions for name and version, and get rid of `alphasort` and `versionsort`
- [ ] `list` feature → also prints in horizontal direction
- [ ] `list` feature → allow to list for a specific name
- [ ] Implement `rename` feature
- [ ] Implement `backup` feature
- [ ] Implement `tree` feature
- [ ] Implement `help` feature
- [ ] Implement `log` feature
- [ ] Implement `search` feature
- [ ] Implement `delete` feature
- [ ] Implement `duplicate` feature
- [ ] Implement `move` feature
- [ ] Implement `report` feature
- [ ] Implement `restore` feature
- [ ] Helper function to get full path from a file/directory


## Refactoring tasks
- [ ] Change `validade_command` to use `for` loop instead of `bsearch()`
- [ ] Merge `search` feature into `list`


## Testing


## BACKLOG
- [-] Implement iterative implementation when `argc == 1`


## DONE
- [x] Create Makefile
- [x] Validate commands and arguments
- [x] Improve commands and arguments verification by using Dispatch Table
- [x] `list` feature → implement comparation functions for sorting method
- [x] `list` feature → implement `check_element()` function to update `f_counter`, `dir_counter`, `slink_counter` and `total_size` variables
- [x] `list` feature → allow `argv[4]` as ASC and DESC order
- [x] Descentralize `commands.h` and create a `.h` file for each functionality
- [x] Use `getopt()` to allow flexible CLA