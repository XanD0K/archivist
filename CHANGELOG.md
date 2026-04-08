# Changelog


## [0.5.1] - 2026-04-07
### Added
- Created auxiliar funcion `free_extensions()` to free the entire array of extensions
- Added `cmp_version()` to sort by version on `list` feature

### Changed
- Changed `GeneralOptions` structure's name to `CommonOptions`
- Changed all `perror()` to `fprintf()` to keep padronize error messages
- `get_all_extensions()` also converts extension to lowercase, improving `strcasecmp()`
- Changed prompt construction to use `asprintf()` instead of `snprintf()`
- Changed path construction to use `check_path_name_size()`

### Fixed
- Fixe memory leak on `get_all_extensions()`
- Fixed typos, name collision and missing libraries on `utils.h`, `utils_filter.c` and `search_cmd.c`
- Fixed typos, bugs and duplicated logic on `delete_element()` and `delete_directory()`
- Fixed memory leak when using `get_all_extensions()`


## [0.5.0] - 2026-04-05
### Added
- Implemented `delete` feature
- Created helper functions `delete_directory()`, `match_directory_size()` and `match_extension()`

### Changed
- Moved all commands' help flags to `help.c`
- Created `ActionOptions` structure for new reused flags
- Moved `check_directory_size()` to `utils.c`

### Fixed
- Fixed bug on `get_answer()`
- Fixed `free(namelist[i])` throughout all files, putting them in the right place
- Fixed CLI arguments parser in `list.c`


## [0.4.0] - 2026-04-01
### Added
- Fully implemented `move` feature
- Implemented `get_answer()` helper function to get user's input

### Changed
- Moved `Extension` structure to `utils.h`, to be a modular struct
- Created `utils_filter.c` to hold all helper functions related to filters and flags
- Moved the logic to retrieve user's extension into `get_all_extensions()` helper function in `utils.c`
- Moved the logic to get suffix from paths into `get_suffix()` helper function `utils.c`


## [0.3.3] - 2026-03-29
### Added
- Implemented `tree` feature
- Created `FilterOptions` structure on `cli_opts.h`
- Started development of `move` feature

### Changed
- `search` feature now uses new `FilterOptions` structure
- Moved `get_size()` to `utils.c`


## [0.3.2] - 2026-03-28
### Added
- `sort` flag for the `report` functionality, allowing `name`, `size` and `distance`

### Changed
- `check_sort()` is now a reusable helper function

### Removed
- `open_directory()` was removed from `utils.c` (by using `scandir()` it was not needed anymore)


## [0.3.1] - 2026-03-27
### Added
- Fully implementation of `report` functionality

### Changed
- Created `check_help()` to check for command's `help` flag, removing repetition throughout files
- Changed the array of pointers to the `Extension` struct (`Extension **ext`) to be an array of `Extension` structs (`Extension *ext`)

### Fixed
- Memory allocation, clean memory and free memory completely fixed


## [0.3.0] - 2026-03-25
### Added
- `formatted_output()` to improve output when `human-readable` flag is toggled on
- New flag for `list` and `report` functionalities: `--human-readable | -h`
- First implementation of `report` feature

### Changed
- Made `get_extension()` to keep modularity among files

## [0.2.0] - 2026-03-24
### Added
- Fully implemented `search` feature
- Added `search` explanation on `help` functionality

### Changed
- Changed `search` usage to be `./archivist search NAME [DIRECTORY] [FLAGS]`, making NAME the (required) third argument and DIRECTORY the (optional) fourth argument


## [0.1.1] - 2026-03-22
### Added
- Added `help` flag for `list` functionality, which fully describes the command and all flags available
- Started implementation of `help` functionality, with description of all available commands

### Changed
- Renamed `get_directory()` to `get_valid_directory()` and included logic to keep `.` as the default directory if user didn't provide one
- Change `case-insensitive` flag to `ignore-case`


## [0.1.0] - 2026-03-20
### Added
- Fully implemented `list` functionality
- Added `order`, `reverse`, `recursive`, `dir-first` and `case-sensitive` flags to `list` functionality

### Changed
- Implemented CLI arguments parser and validation with `getopt_long()`
- Segregated `commands.h`, reallocating all `list` functionality logic into `list.h`

### Removed
- Removed DIR *dir implementation on `commands.c`. Kept directory check just with `struct stat st`


## [0.0.2] - 2026-03-17
### Added
- Implemented commands and arguments validations
- Segregated validation logic from `main.c`

### Changed
- Improved validation logic, changing from `switch()` and multiple `if`-`else` checks to a Dispatch Table


## [0.0.1] - 2026-03-15
### Added
- Created Makefile