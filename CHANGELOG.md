# Changelog


## [Unreleased] - DATE
### Added

### Changed

### Fixed

### Removed

## [0.0.3] - DATE
### Added

### Changed

### Fixed

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