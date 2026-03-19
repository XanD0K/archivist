# TODO


## New features
- [ ] Implements `list` feature
- [ ] `list` feature → implements comparation functions for name and version, and get rid of `alphasort` and `versionsort`
- [ ] `list` feature → improves `cmp_*()` functions with S_ISDIR()
- [ ] `list` feature → allows `argv[4]` as ASC and DESC order
- [ ] `list` feature → also prints in horizontal direction (just like `ls` command)
- [ ] `list` feature → implements `check_element()` function to update `f_counter`, `dir_counter`, `slink_counter` and `total_size` variables 
- [ ] Implements `rename` feature
- [ ] Implements `backup` feature
- [ ] Implements `tree` feature
- [ ] Implements `help` feature
- [ ] Implements `log` feature
- [ ] Implements `search` feature
- [ ] Implements `delete` feature
- [ ] Implements `duplicate` feature
- [ ] Implements `move` feature
- [ ] Implements `report` feature
- [ ] Implements `restore` feature

## Refactoring tasks
- [ ] Changes `validade_command` to use `for` loop instead of `bsearch()`

## Testing


## BACKLOG
- [-] Implements iterative implementation when `argc == 1`

## DONE
- [x] Creates Makefile
- [x] Validates commands and arguments
- [x] Improves commands and arguments verification by using Dispatch Table
- [x] `list` feature → implements comparation functions for sorting method