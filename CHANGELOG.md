# Changelog


## [VERSION] - DATE
### Added

### Changed

### Fixed

### Removed


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