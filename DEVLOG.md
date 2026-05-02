# Development Log

## [2026-05-01]
**Plans**
- Last checks before submitting
- Checks for typos, bugs and possible improvements

**Progress**
-  `filter` variable is now a field on `CommandContext` structure, with its core logic defined in `settup_command()`
- Improved UX by adding `\` at the end of found directories on `list` and `search` features
- Fixed typos and minor bugs. All features properly working


## [2026-04-30]
**Plans**
- Test all features

**Challenges**
- Fix parser to prevent
- Hash function for check changes in file

**Progress**
- When testing `list` feature, I noticed my parser had 2 buggs:
    - Bitmasks overlap on all 3 flags structures (`CommonOptions`, `FilterOptions` and `ActionOptions`), since they all started with `(1U << 0)`. Decided to use a 8 bit block for each structure (0-7, 8-15, 16-23, 24-31) to prevent overlapping, and kept extra space if decided to add new flags later
    - Invalid flags weren't being correctly identified, mainly because I was doing up to 4 parsers, each one starting from the beginning `optind = opt_start`. Solved this by doing just one parser per feature, and refactoring general parsers to not use `getopts_long()`, and just check individual flags
- When testing `tree` feature, I created the `scandir_show_hidden_files`, used in `scandir()`
- Created `check_directory_flags()` and `check_file_flags()` and put in `utils_filter.c` file, together with all flag checker functions, to remove boilerplate on almost all features, specially the ones that use `FilterOptions` structure
- When testing `move` feature, I was wondering other flags for the `ls` command (I already knew it had `-a | --all` and `-R | --recursive` flags). I then asked Grok(xAI) about all flags available in `ls` command, and I noticed that my new filter was similar to the flag `-A | --almost-all`, whith the difference that my filter excludes all hidden directories, while `ls -A` only excluded `"."` and `".."`. Decided to implement my own version of `-A | --almost-all` flag, which makes `scandir()` uses `scandir_show_hidden_files` filter. With that, all features (except `tree` and `recover`) can now use both `-a | --all` and `-A | --almost-all` flags
- When testing `rename` feature, I noticed a `.ini` file was being renamed, even though it wasn't being displayed on directory (even with "show hidden files" turned on). I decided to create `is_system_file()` to warn users about renaming those files. Grok (xAI) helped me with the list of system/configuration files
- When testing `backup` feature, I noticed my `file_needs_backup()` function, which was responsible dor the incremental backup, wasn't working properly. After digging into this problem, I discovered the issue: the directories I was working with, both using Micrsoft's OneDrive, could potentialy change the values of `st_mtim.tv_sec` and `st_mtim.tv_nsec`, used to check consistency between source and destination files. I had to remove that checker, but since keeping only the size checker wasn't relyable, I decided to implement a hash to identify changes. Used Grok's (xAI) help and spent the morning of April 30th learning how to implement the CRC32 algorithm in two ways: first one that checks each bit from every bite manually, and the second one, which is the one implemented on my program, that uses a table with all possible values for each byte (0 to 255)
- All features successfully tested, all flags properly working, code cleaned and improved!


## [2026-04-22]
**Plans**
- Improve code, fix bugs, remove boilerplates

**Challenges**
- Walk through all files and all functions, finding the best improvements

**Progress**
- Changed `list` feature output logic, to be more clean and direct, concentrating the block of code that deals with printing the output message
- Fixed `search` feature do handle a better interaction between `type` flag and other flags, when chosen type was "directory"
- Refactores all features to rely on `d_type` (from `struct dirent`) to determine the type of the element, instead of always calculating `struct stat` for that element. The `stat` is now only being used as a fallback option, when `d_type` is UNKNOWN, or when it's other types besides regular files, simbolic links and directories
- Also changed `list` feature counters to be a new `ListCounters` structure, which improved code, keeping it cleaner and improving maintainability
- Cleaned `search` feature logic, refactored its usage, improved functions
- Changed all `stat()` calls to be `lstat()` instead, preventing following path when element's type is simbolic link
- Changed `concatenates_prefix()` to use `memcpy()` instead of `strcpy()` + `strcat()`, which has a better performance
- Improved `tree` feature, fixed bugs, cleaned code
- Fixed declaration logic of `dst_dir` and `base_dir` for `move`, `backup` and `recover` features. On those functions, those directories were inverted. Added `char *dst_dir` field to `CommandContext` stucture, and improved directories logic for all features
- Implemented last improvements on `move`, `delete`, `rename`, `backup` and `recover` features, keeping code cleaner
- Implemented output messages on `rename`, `backup` and `recover` features


## [2026-04-17]
**Plans**
- Implement `log` feature
- Fix bugs and improve code

**Challenges**
- Manage .log files, read and write line by line, define its structure

**Progress**
- On `validate_command()`, I was using `bsearch()` to search for the command in the `cmd_table`. I remember the information that Binary Search wouldn't be ideal if the list was small (less than 40 items). Since my table has only 10 commands, a simples `for` loop with early exit would be better
- Fully implemented `log` feature


## [2026-04-15]
**Plans**
- Implement `backup` feature
- Implement `recover` feature
- Remove boilerplates
- Improve error and parser outputs

**Challenges**
- Bitwise operators

**Progress**
- Improved error code by using a `enum` type, instead of hardcoding each error, which keeps code cleaner
- Improved CLI arguments parsers output by preventing code running with unsupported flag. First idea was to do a for loop and check if given flag is a supported flag for that feature. Grok (xAi) suggested to use bitmasks and bitwise operators to define wich flags are allowed in each feature. Proceeded with that second idea: gave each flag a bitmask and defined in each feature a `uint32_t` variable with all supported flags. Now, even though the feature has the unsupported flag in its structure, the bitwise operation inside general parsers will check the flag and display an error message if unsupported, improving UX
- To implement `backup` and `recover` features, I first had the idea to use `FILE *` + `fread() | fwrite()` and copy bytes by bytes, which was the only way I knew how to do. Grok (xAi) suggested the usage of `copy_file_range()`, which was a better and more modern way of copying files. I follow that idea, which would be a great opportunity to learn a new way of handling files:
    - https://man7.org/linux/man-pages/man2/copy_file_range.2.html
    - https://manual.cs50.io/2/copy_file_range
    - https://man7.org/linux/man-pages/man3/open.3p.html
    - https://man7.org/linux/man-pages/man3/close.3p.html
- I first decided to do an incremental backup, which would only trigger for files that were modified. To do this, first checks file's existence on destination directory, than compare `st_mtim` and `st_size`, to check for changes. It still has a corner case that leads to orphan files: if the name of the file is modified, the program won't recognize this change, will create a new backup, and the first backed up file (the one with the old name) will have no relation to the file in the source directory. I could create a function that makes hashes for each file, and I could just compare the hashes from source and destination directories, but that will be something to do later
- I then faced a logic issue: the `recover` feature would work just like a "copy" feature, where files could just be copied from any directory. To solve this, I decided to add a hidden marked file (`.archivist-backup`) to the destination directory, which would define that directory as the holder of backed up files. It happens right after `backup` finishes traversing all files. Now, the `recover` feature shall only work if the source directory (destination directory when backing up) has that hidden marked file. It still has one problem: user could just manually transfer files from any directory to the backed up directory, and use the `recover` feature. Later I'll add a hidden marked file for each file in the directory, which will prevent this behavior
- `recover` feature was just a simplier version of the `backup` feature, with almost no flags and that uses the same functions. It was an easy implementation
- `backup` and `recover` features successfully implemented


## [2026-04-11]
**Plans**
- Implement `rename` feature
- Remove boilerplates

**Challenges**

**Progress**
- Concentrated all comparation functions used by `scandir()` into a new file (`utils_sort.c`)
- Created 3 general parsers, one for each structure (`parse_common_opts`, `parse_filter_options` and `parse_action_options`). It removed boilerplates and kept each features's parser cleaner and focused on its own unique flags
- Created new `generate_unique_name()` to create incremental file name
- `rename` feature implemented, still with some repetitive code, specially with flags checker logic/functions


## [2026-04-08]
**Plans**
- Fix bugs and memory leak
- Remove boileplates

**Challenges**
- Find issues, erros, typos
- Find the best way to remove boilerplates

**Progress**
- Some functions where still leaking memory (e.g. `get_all_extensions()`)
- Improved usage of `snprintf()`
- Improved construction of `prompt` on features where user's input was required (`-i|--interactive` flag) by using `asprintf()`
- Created `CommandContext` structure and `setup_command()` function to handle command initialization. It elimated the first boilerplate on most features: 
    - Check for `help` flag
    - Define initial values
    - Validate `base_dir`
    - Parse CLI arguments → Grok (xAi) did help me with this, specilially refactoring the prototype of the function and casting it to a new value
- The other boilerplate I thought that was in the program wasn't actually a boilerplate:
    - `dirent` struct
    - call action function
    - free memory
    - output message
- The logic behind each one was the same, bust the structure and fucntions' signatures were all different, as well as the block of memory that should be freed and the output messages. Decided not to remove this "boilerplate" because it would just create complexity without any substantial gain. Instead, I just created helper functions to eliminate some repetitive functions


## [2026-04-05]
**Plans**
- Implement `delete` feature

**Challenges**
- Better way to improve relationship across all flags
- Refactor `type` flag logic for directories so it can interact better with all other flags

**Progress**
- While implementing basic function to validate the `delete` feature, I thought I could remove some repetition (boilerplate) from all files. All coomand's files follow the same logic:
    - Check for `help` flag
    - Defines starting values
    - Validates base directory
    - Parses CLI arguments
- Implemented a helper function and a structure to remove the boilerplate, but in the end it ended up by removing just 5-8 lines from each file. In other words, I gave the code more complexity for no substantial gain. Undid all changes, keeping the code simpler but easier to maintain
- After finishing the program, I'll go back and refator everything, removing this boilerplate, the one at the end of each handler function (`struc dirent`, recursiveness and `free()` memory) and the one at beggining of each action functions (removes `.` and `..`, creates new path, calls `snprintf()`, recursiveness, `free()`)
- Noticed a bug on the way `type` flag interacted with other flags, specially `contains`, `max-size` and `min-size`. Created `match_directory_size()` to recursivelly calculates the size of a directory, and `delete_directory()` to recursivelly deletes a directory
- Rest of `delete` feature was implemented just like all other features


## [2026-04-01]
**Plans**
- Implement `move` feature

**Challenges**
- Comprehend the order in which recursion should occur → one directory must be created before moving to the next subdirectory
- Understand which filters should be applied to directories and which one should be applied to files

**Progress**
- Created `FilterOptions` structure with filter flags, currently being used in `search`, and will also be implemented on `move`
- Created `get_valid_destination()`, that recursivelly creates the destination directory
- For every directory created on destinarion, I check path's size to prevent overflow and truncation with `snprintf()`
- Fully implemented `move` feature by improving UX with `dry-run`, `interactive` and `verbose` flags


## [2026-03-29]
**Plans**
- Implement `tree` feature

**Challenges**
- Understand how to propagate prefix across multiple subdirectories

**Progress**
- `tree` functionality is just a good combination of style and recursiveness. First I made recursion works, and just then I included the symbols (`├──` `└──` `│`) and indentations, always propagating to next subdirectory
- `tree` functionality fully implemented


## [2026-03-27]
**Plans**
- Implement `report` feature

**Challenges**
- Finding the best data structure to keep track of all extensions efficiently
- Free memory on all cases, specially when using `strdup()` and `asprintf()`
- Still confusing dealing with different types

**Progress**
- First thought was to use an ordered linked list to manage all extensions
- Although it seemed like a solid idea, Grok (xAI) suggested using a Dynamic Array instead
- On pset5 'speller', among all implementations I've made, one of them was a Dynamic Hash Table, that doubled its size when a specific factor was reached. Used the same strategy to implement a Dynamic Array that holds every found extension
- I first tried an array of pointers to the `Extension` struct (`Extension **ext`)
- Then I decided to move to an array of `Extension` structs (`Extension *ext`), keeping code cleaner and memory allocation simpler
- Fixed problems with memory leak specifically with `strdup()` and `asprintf()`. I didn't know I should `free()` strings returned by those functions:
    - https://manual.cs50.io/3/strdup
    - https://manual.cs50.io/3/asprintf
- Fixed problems when comparing and making operations with different types: `size_t` with `int`, `const char *` with `char *`, `ssize_t` with `size_t`. Needs to pay more attention about the types I declare and the casts needed to make all operations work
- `report` feature is fully implemented


## [2026-03-24]
**Plans**
- Implement `search` feature

**Challenges**
- Implementing multiple flags, applied to recursive elements
- Deciding the best way to call each flag to each element, especially when `recursive` flag was `true`

**Progress**
- First, I was thinking of getting all elements into an `char **array`, and start removing them based on chosen flags
- Then I decided the best way of implementing a search feature was to check each element at a time, printing only those who passed all checks
- Didn't struggle with CLI arguments parser, help message, declarations, etc., especially after implementing the `list` feature


## [2026-03-20]
**Plans**
- Improve CLI arguments validations
- Implement `list` feature

**Challenges**
- Learn how to use `getopt_long()` to allow multiple flags in no fixed order and a better flags parsing/validation

**Progress**
- Changed from manual CLI arguments parser and validation to an implementation that uses `getopt_long()`
- Learned how to structure and build a CLI parser with `getopt_long()`:
    - Michael Kerrisk's Linux manual page: https://man7.org/linux/man-pages/man3/getopt.3.html
- Fully implemented `list` feature
- Successfully implemente CLI arguments parser with `getopt_long()`


## [2026-03-17]
**Plans**
- Implement commands and arguments verification

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
- Create a Makefile for Final Project `Archivist`

**Challenges**
- Didn't know how to build a Makefile
- Didn't know nothing about its syntaxes, and organization - e.g. wildcard, dependecy files (.d), targets, variables, flags

**Progress**
- Spent the last 3 days studying Makefile:
    - Colby College tutorial: https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
    - GNU Make Tutorial: https://www.gnu.org/software/make/manual/make.html#How-Make-Works
    - Help from Grok (xAI) with all this process
- Learned how to build a professional Makefile
- Finished Makefile implementation for `Archivist`
