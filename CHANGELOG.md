# Changelog


## [Unreleased] - DATE
### Added

### Changed

### Fixed

### Removed


## [0.1.1] - DATE
### Added
- Added `help` flag for `list` functionality, which fully describes the command and all flags available

### Changed
- Renamed `get_directory()` to `get_valid_directory()` and included logic to keep `.` as the default directory if user didn't provide one

### Fixed

### Removed


## [0.1.0] - 2026-03-20
### Added
- Fully implemented `list` functionality
- Added `order`, `reverse`, `recursive`, `dir-first` and `case-sensitive` flags to `list` functionality

### Changed
- Implemented CLI arguments parser and validation with `getopt_long()`
- Segregated `commands.h`, reallocating all `list` functionality logic into `list.h`

### Removed
- Removed DIR *dir implementation on `commands.c`, before moving to handler functions. Kept directory check just with `struct stat st`


## [0.0.2] - 2026-03-17
### Added
- Implemented commands and arguments validations
- Segregated validation logic from `main.c`

### Changed
- Improved validation logic, changing from `switch()` and multiple `if`-`else` checks to a Dispatch Table


## [0.0.1] - 2026-03-15
### Added
- Created Makefile