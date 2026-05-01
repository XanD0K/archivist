# TODO


## New features


## Refactoring tasks
- [-] Improve error codes (`error_code.h`) to embrace every possible error
- [ ] `list` and `search` features → improve output by adding a `/` on diretctories
- [ ] Fix output message do display correct number of files for each directory
- [ ] Moves `filter` variable to be a `CommandContext` field


## Testing


## BACKLOG (Postponed)
- Implement `copy` feature
- `list` feature → also prints in horizontal direction (like `ls` command)
- `backup` feature → add hidden marker by file to prevent recovering of files that weren't backed up


## DONE
- [x] Create Makefile
- [x] Validate commands and arguments
- [x] Improve commands and arguments verification by using Dispatch Table
- [x] `list` feature → implement comparation functions for sorting method
- [x] `list` feature → implement `check_element()` to update `f_counter`, `dir_counter`, `slink_counter`, `error_counter` and `total_size` variables
- [x] `list` feature → allow `argv[4]` as ASC and DESC order
- [x] Descentralize `commands.h` and create a `.h` file for each functionality
- [x] Use `getopt_long()` to allow flexible CLI arguments
- [x] Implement `list` feature
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
- [x] Fix error code on parsers
- [x] Fix error code on all files
- [x] Improve parser output message for invalid or not allowed flag
- [x] Implement `backup` feature
- [x] Implement `recover` feature
- [x] Implement `help` feature
- [x] Implement `log` feature
- [x] Change `validate_command()` to use `for` loop instead of `bsearch()`
- [x] Add more fields in the `search` feature output
- [x] Fix `validate_args()` with new max/min commands allowed
- [x] Change type checker to use `d_type` as default and `struct stat` as fallback
- [x] Fix directory retriever in `move`, `backup` and `recover` features (all three need destination directory)
- [x] Refactor flags checker (boilerplate in the beggining of each `CMD_element()` function)
- [x] Implement flag `-A | --almost-all`, which will use `scandir_show_hidden_file` filter
- [x] Apply `get_suffix()` on other features
- [x] `log` feature → add flag to clean .log file (`-E|--erase`)
- [x] Test all features
