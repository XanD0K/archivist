# TODO


## New features
- [ ] Implement `list` feature
- [ ] `list` feature → also prints in horizontal direction
- [ ] Implement `backup` feature
- [-] Implement `help` feature
- [ ] Implement `log` feature
- [ ] Implement `restore` feature
- [ ] Helper function to get full path from a file/directory
- [ ] Add more fields in the `search` feature output


## Refactoring tasks
- [ ] Change `validade_command` to use `for` loop instead of `bsearch()`
- [ ] Fix `validate_args()` with new max/min commands allowed
- [ ] Refactor flags checker (boilerplate in the beggining of each `CMD_element()` function)
- [ ] Change type checker to use `d_type` as default and `struct stat` as fallback
- [ ] Modular function to create and validate path
- [ ] Change content retriever (`struct dirent`) to remove boilerplate

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
- [x] Add `qsort()` on `report` functionality
- [x] Implement `tree` feature
- [x] Modular function to retrieve extensions
- [x] Implement `move` feature
- [x] `move` feature → creates destination directory when validation fails
- [x] Fix `type` flag to better interact with other flags
- [x] Implement `delete` feature
- [x] Change `perror()` to `fprintf()` to padronize all error outputs
- [x] Change `get_all_extensions()` to also convert extensions to lowercase
- [x] Change `snprintf()` to `check_path_name_size()` on all files
- [x] Move `extension` flag from `GeneralOptions` to `FilterOptions` structure
- [x] `list` feature → implement comparation functions for name and version, and get rid of `alphasort` and `versionsort`
- [x] Change all commands logic validation/parse to remove repetitive code (boilerplate)
- [x] Refactor `sort` flag logic to remove boilerplate
- [x] Refactor parsers to segregate into 3 more parsers, one for each common structure
- [x] Implement `rename` feature
- [x] Improve `recursive` flag on `list` feature
