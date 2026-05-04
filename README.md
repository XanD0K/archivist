# Archivist
#### Video Demo:


# Description
This program is my final project for "[**CS50's Introduction to Computer Science**](https://pll.harvard.edu/course/cs50-introduction-computer-science)" course, from Harvard University.
It was developed in C, and it is a directory and file manager, with the purpose of applying all knowledge and skills learned throughout the course.

---


## Features
- [`backup`](#backup) → backs up the content from source directory into destination directory
- [`delete`](#delete) → deletes the content of a directory
- [`help`](#help) → displays help message
- [`list`](#list) → lists content from a directory
- [`log`](#log) → displays the history log, which stores all actions performed by destructive features
- [`move`](#move) → moves the content from source directory to destination directory
- [`recover`](#recover) → recovers the content from a directory that was previously backed up
- [`rename`](#rename) → renames all files from a directory
- [`report`](#report) → calculates the proportion of all files' extensions in a directory
- [`search`](#search) → searches for a specific element in a directory
- [`tree`](#tree) → displays the content of a directory in a tree format

---


## Table of Contents
- [Description](#description)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
    - [Common Flags](#common-flags)
        - [Detailed Flag Behavior](#detailed-flag-behavior)
    - [Commands](#commands)
- [Files Overview](#files-overview)
- [Design Choices](#design-choices)
- [Acknowledgments](#acknowledgments)
- [Future Plans](#future-plans)
- [License](#license)
---


## Installation
This program works through CLI arguments, with no installation needed. Just run those 2 instructions:
- `make archivist`
- `./archivist help` or `./bin/archivist help`

---


## Usage
### Common Flags
|FLAGS                   |DESCRIPTION                                                 |TAKES ARGUMENT|DEFAULT VALUE|
|:-----------------------|:-----------------------------------------------------------|:-------------|:------------|
|`-a`, `--all`           |Displays hidden files and hidden directories                |No            |off          |
|`-A`, `--almost-all`    |Displays hidden files, but hides hidden directories         |No            |off          |
|`-h`, `--human-readable`|Outputs sizes in a readable format                          |No            |off          |
|`-R`, `--recursive`     |Also traverses subdirectories                               |No            |off          |
|`-s`, `--sort`          |Sorts directory's content                                   |Yes           |`name`       |
|`-c`, `--contains`      |Filters for files/subdirectories that contain a word/pattern|Yes           |-------------|
|`-e`, `--extension`     |Filters for files with specific extensions                  |Yes           |-------------|
|`--max-size`            |Filters for files with size lower than maximum size         |Yes           |-------------|
|`--min-size`            |Filters for files with size greater than minimum size       |Yes           |-------------|
|`-t`, `--type`          |Filters the element's type                                  |Yes           |-------------|
|`-d`, `--dry-run`       |Simulates changes                                           |No            |off          |
|`-i`, `--interactive`   |Asks for user's confirmation before performing an action    |No            |off          |
|`-v`, `--verbose`       |States every performed action on terminal                   |No            |off          |

---


#### Detailed Flag Behavior
**`-a`, `--all`**
- **Description:** displays all files and directories, including hidden ones (those starting with a `.`). It does not show the special entries `.` and `..`
- **Default Value:** off (shows only visible elements)
- **Note:** this flag is ignored if `-A|--almost-all` flag is also present
- **Features:** [`backup`](#backup), [`delete`](#delete), [`list`](#list), [`move`](#move), [`rename`](#rename), [`report`](#report), [`search`](#search), [`tree`](#tree)
- **Example:** `./archivist list -a`

**`-A`, `--almost-all`**
- **Description:** displays all files and directories, but only displays hidden files (those starting with a `.`). It does not show hidden directories, neither the special entries `.` and `..`
- **Default Value:** off (shows only visible elements)
- **Note:** this flag takes precedence when provided with the flag `-a|--all`
- **Features:** [`backup`](#backup), [`delete`](#delete), [`list`](#list), [`move`](#move), [`rename`](#rename), [`report`](#report), [`search`](#search)
- **Example:**  
`./archivist list -A`   
`./archivist list -a -A` → same result as above

**`-h`, `--human-readable`**
- **Description:** displays all sizes in a readable format (e.g. `930.53 KB` instead of `952864 bytes`)
- **Default Value:** off (shows sizes in `bytes` unit)
- **Features:** [`delete`](#delete), [`list`](#list), [`move`](#move), [`report`](#report)
- **Example:** `./archivist list -h`

**`-R`, `--recursive`**
- **Description:** traverses not only directories but also its subdirectories
- **Default Value:** off
- **Features:** [`backup`](#backup), [`delete`](#delete), [`list`](#list), [`move`](#move), [`rename`](#rename), [`report`](#report), [`search`](#search)
- **Example:** `./archivist list -R`

**`-s`, `--sort`**
- **Description:** sorts content from a directory
- **Default Value:** `name`
- **Note:** check available sort methods for each feature that supports this flag
- **Features:** [`list`](#list), [`rename`](#rename), [`report`](#report)
- **Example:**  
`./archivist list -s`  
`./archivist list -s name` → same result as above

**`-c`, `--contains`**
- **Description:** filters for elements that matches specified pattern. This flag requires an argument, which is the pattern you want to use to filter for elements
- **Features:** [`backup`](#backup), [`delete`](#delete), [`move`](#move), [`rename`](#rename), [`search`](#search)
- **Example:** `./archivist search -c abas` → includes only elements that have the sequence `abas` in their name

**`-e`, `--extension`**
- **Description:** filters for elements that matches specified extension(s). This flag requires an argument, which is the extension(s) you want to use to filter for elements
- **Note:** if want to filter for more than one extension, separate them with a comma and don't use leading period (e.g. pdf,jpg,txt)
- **Features:** [`backup`](#backup), [`delete`](#delete), [`move`](#move), [`rename`](#rename), [`report`](#report), [`search`](#search)
- **Example:**  
`./archivist report -e pdf`  
`./archivist report -e jpg,jpeg`

**`--max-size` , `--min-size`**
- **Description:** filters for elements with lower/higher size than provided maximum/minimum size. This flag requires an argument, which is the size you want to use to filter for elements
- **Note:** you can also specify the unit. If not specified, it will use the default value: `B` (bytes)
- **Units:** `B `| `K`, `KB` | `M`, `MB` | `G`, `GB` | `T`, `TB`
- **Features:** [`backup`](#backup), [`delete`](#delete), [`move`](#move), [`rename`](#rename), [`search`](#search)
- **Example:**  
`./archivist search --max-size 50` → 50 bytes  
`./archivist search --max-size 50B` → same result as above  
`./archivist search --min-size 50MB` → 50 megabytes

**`-t` , `--type`**
- **Description:** considers only specified type when performing the action or checking for validity
- **Allowed:** `f`, `file` | `sl`, `slink`, `symbolic-link` | `d`, `dir`, `directory`
- **Features:** [`delete`](#delete), [`move`](#move), [`search`](#search)
- **Notes:** when specified as `"directory"`, this flag has a unique interaction with the flags `contains`, `max-size` and `min-size`
    - `contains` → will consider the name of the subdirectory instead of the name of its files
    - `max-size` and `min-size` → will consider the maximum and minimum size of the whole subdirectory instead of the size of each of its files
- **Example:** `./archivist search --type d`

**`-d`, `--dry-run`**
- **Description:** simulates changes, without actually executing them. Useful if you want to see the result of a command. Recommended for destructive features
- **Default Value:** off
- **Features:** [`backup`](#backup), [`delete`](#delete), [`move`](#move), [`recover`](#recover), [`rename`](#rename)
- **Example:** `./archivist delete -d`

**`-i`, `--interactive`**
- **Description:** asks the user for confirmation before proceeding with any changes
- **Default Value:** off
- **Features:** [`backup`](#backup), [`delete`](#delete), [`move`](#move), [`recover`](#recover), [`rename`](#rename)
- **Example:** `./archivist delete -i`

**`-v`, `--verbose`**
- **Description:** prints every performed action on the terminal
- **Default Value:** off
- **Features:** [`backup`](#backup), [`delete`](#delete), [`move`](#move), [`recover`](#recover), [`rename`](#rename)
- **Example:** `./archivist delete -v`

---


### Commands
**Notes:**
- `SRC_DIR` → Most features need a source directory to perform an action. It's an optional argument and defaults to current `(.)` directory if not provided.
- `DST_DIR` → Some features also use a destination directory. It's a required argument, and if the path doesn't exist yet, it will be created.
- To prevent errors, it is recommended that both `SRC_DIR` and `DST_DIR` are provided with quotation marks (e.g. `"/folder1"`).

---


#### `backup`
**Description:** backs up the content from `SRC_DIR` to `DST_DIR`  
**Usage:** `./archivist backup [SRC_DIR] DST_DIR [FLAGS]`  
**Supported Common Flags:** `all`, `almost-all`, `contains`, `dry-run`, `extension`, `interactive`, `max-size`, `min-size`, `recursive`, `verbose`  
**Notes:**
- If recursive flag (`-R|--recursive`) is not provided, it will only backs up the files in the provided `SRC_DIR`
- This feature creates a hidden mark on `DST_DIR` that will be used by [`recover`](#recover) feature

**Example:** `./archivist backup "/folder1" "/folder2" -R -a -d`

---


#### `delete`
**Description:**  deletes content from `SRC_DIR`  
**Usage:** `./archivist delete [SRC_DIR] [FLAGS]`  
**Supported Common Flags:** `all`, `almost-all`, `contains`, `dry-run`, `extension`, `human-readable`, `interactive`, `max-size`, `min-size`, `recursive`, `type`, `verbose`  
**Notes:** if recursive flag (`-R|--recursive`) is not provided, it will only delete the files in the provided `SRC_DIR`  
**Example** `./archivist delete "/folder" -d -e txt,pdf`

---


#### `help`
**Description:** displays help message with all available commands  
**Usage:** `./archivist help`

---


#### `list`
**Description:** lists the content from `SRC_DIR`  
**Usage:** `./archivist list [SRC_DIR] [FLAGS]`  
**Supported Common Flags:** `all`, `almost-all`, `human-readable`, `sort`, `recursive`  
**Specific Flags:**

**`-D`, `--dir-first`**
- **Description:** makes subdirectories appear first
- **Default Value:** off
- **Example:** `./archivist list -D`

**`-I`, `--ignore-case`**
- **Description:** sorts directory's content with case-insensitive comparison functions
- **Default Value:** off
- **Example:** `./archivist list -I`  

**`-r`, `--reverse`**
- **Description:** changes the output list from ascending to descending order
- **Default Value:** off
- **Example:**  `./archivist list -r`

**Notes:** `sort` methods allowed:
- date → compares last modification date
- name → compares the ASCII value of each character
- size → compares the size of each file
- extension → compares extension of each file
- version → compares letters and numbers separately  

**Example:** `./archivist list "/folder" -s size -R --all`

---


#### `log`
**Description:** displays log history for previously executed commands  
**Usage:** `./archivist log [FLAGS]`  
**Specific Flags:**

**`-l`, `--limit`**
- **Description:** limits the size of the output
- **Example:**  `./archivist log -l 100`  

**`-C`, `--command`**
- **Description:** filters the output based on specified command
- **Example:**  `./archivist log -C delete`  

**`-E`, `--erase`**
- **Description:** erases the .log file, cleaning all previous records
- **Default Value:** off
- **Example:**  `./archivist log -E`

**Notes:**
- Only saves actions from destructive features: [`backup`](#backup), [`delete`](#delete), [`move`](#move), [`recover`](#recover) and [`rename`](#rename)
- Has no previous records for features: [`list`](#list), [`log`](#log), [`report`](#report), [`search`](#search) and [`tree`](#tree)

**Example:** `./archivist log --command move`

---


#### `move`
**Description:** moves files from `SRC_DIR` to `DST_DIR`  
**Usage:** `./archivist move [SRC_DIR] DST_DIR [FLAGS]`  
**Supported Common Flags:** `all`, `almost-all`, `contains`, `dry-run`, `extension`, `interactive`, `max-size`, `min-size`, `recursive`, `type`, `verbose`  
**Specific Flags:**

**`-f`, `--force`**
- **Description:** if a file already exists on `DST_DIR`, it will be overwritten
- **Default Value:** off
- **Note:** this flag is ignored if `-S|--skip` flag is also present
- **Example:**  `./archivist move "/folder1" "/folder2" -f`  

**`-S`, `--skip`**
- **Description:** if a file already exists on `DST_DIR`, it will be skipped (nothing will happen)
- **Default Value:** off
- **Note:** this flag takes precedence when provided with the flag `-f|--force`
- **Example:**  
`./archivist move "/folder1" "/folder2" -S`  
`./archivist move "/folder1" "/folder2" -f -S` → same result as above

**Notes:** when force and skip flags are not provided, if a file already exists on `DST_DIR` with the same name of moved file, this moved file will be automatically renamed to an incremental available name (e.g. `file.txt` → `file_1.txt`)

**Example:** `./archivist move "/folder1" "/folder2" --force -d -R -A`

---


#### `recover`
**Description:** recovers all content from `SRC_DIR` to `DST_DIR`  
**Usage:** `./archivist recover [SRC_DIR] DST_DIR [FLAGS]`  
**Supported Common Flags:** `dry-run`, `interactive`, `verbose`  
**Notes:** this feature only works with previously backed up directories, meaning that `SRC_DIR` must be backed up directory (a directory containing the hidden mark)  
**Example:** `./archivist recover "/folder1" "/folder2" -d -v`

---


#### `rename`
**Description:** renames all files from `SRC_DIR`  
**Usage:** `./archivist rename [SRC_DIR] [FLAGS]`  
**Supported Common Flags:** `all`, `almost-all`, `contains`, `dry-run`, `extension`, `interactive`, `max-size`, `min-size`, `recursive`, `sort`, `verbose`  
**Specific Flags:**

**`-n`, `--name`**
- **Description:** determines the name that will be used to rename all other files
- **Default Value:** `"file"` → if flag is not provided, the word `"file"` will be used as default
- **Example:** `./archivist rename "/folder" -n bab` 

**Notes:** `sort` methods allowed:
- date → compares last modification date
- name → compares the ASCII value of each character
- size → compares the size of each file
- extension → compares extension of each file
- version → compares letters and numbers separately  

**Example:** `./archivist rename "/folder" -R -d -e txt,pdf`

---


#### `report`
**Description:** displays the proportion of total number of files and total size for each extension on `SRC_DIR`  
**Usage:** `./archivist report [SRC_DIR] [FLAGS]`  
**Supported Common Flags:** `all`, `almost-all`, `extension`, `human-readable`, `sort`, `recursive`  
**Notes:** `sort` methods allowed:
- date → sorts by last modification date
- size → sorts by the total size of each extension
- quantity → sorts by total number of files of each extension

**Example:** `./archivist report "/folder" -e txt,pdf,jpeg -R`

---


#### `search`
**Description:** searches for a specific element on `SRC_DIR`  
**Usage:** `./archivist search [SRC_DIR] [FLAGS]`  
**Supported Common Flags:** `all`, `almost-all`, `contains`, `extension`, `max-size`, `min-size`, `recursive`, `type`  
**Notes:** by default it will only search for files. If user wants to search for a specific directory, use the flag `-t|--type` with the `directory` argument (e.g. `-t dir`)  
**Example:** `./archivist search /folder -R --min-size 50K --max-size 50G`

---


#### `tree`
**Description:** displays the content of `SRC_DIR` directory in a tree format. It displays hidden files by default  
**Usage:** `./archivist tree [SRC_DIR]`  
**Supported Common Flags:** `all`  
**Example:** `./archivist tree "/folder" -a`

---


## Files Overview
- **HEADER FILES** (`/include`)
    - [archivist.h](include/archivist.h): Main header
    - [backup.h](include/backup.h): Function prototypes and structures for [`backup`](#backup) command
    - [cli_opts.h](include/cli_opts.h): All flags and their respective bitmasks
    - [cli_parse.h](include/cli_parse.h): CLI arguments parse function prototypes
    - [commands.h](include/commands.h): Command aliases, structures and context's definition, and prototype for [`tree`](#tree) feature
    - [delete.h](include/delete.h): Function prototypes and structures for [`delete`](#delete) command
    - [error_code.h](include/error_code.h): All error codes used in the program
    - [help.h](include/help.h): Function prototypes for [`help`](#help) command
    - [list.h](include/list.h): Function prototypes and structures for [`list`](#list) command
    - [log.h](include/log.h): Constants and function prototypes and structures for [`log`](#log) command
    - [move.h](include/move.h): Function prototypes and structures for [`move`](#move) command
    - [recover.h](include/recover.h): Function prototypes and structures for [`recover`](#recover) command
    - [rename.h](include/rename.h): Function prototypes and structures for [`rename`](#rename) command
    - [report.h](include/report.h): Function prototypes and structures for [`report`](#report) command
    - [search_cmd.h](include/search_cmd.h): Function prototypes and structures for [`search`](#search) command
    - [utils_filter.h](include/utils_filter.h): Function prototypes for filtering utilities
    - [utils_sort.h](include/utils_sort.h): Function prototypes, aliases and structure for sorting utilities
    - [utils.h](include/utils.h): General utility function prototypes and structures
- **SOURCE FILES** (`/src`)
    - [main.c](src/main.c): Program's entry point
    - [backup.c](src/backup.c): Implementation of [`backup`](#backup) command
    - [cli_parse.c](src/cli_parse.c): Implementation of CLI arguments parser
    - [commands.c](src/commands.c): Command validation and routing logic
    - [delete.c](src/delete.c): Implementation of [`delete`](#delete) command
    - [help.c](src/help.c): Program's help message and command-specific [`help`](#help) flags
    - [list.c](src/list.c): Implementation of [`list`](#list) command
    - [log.c](src/log.c): Implementation of [`log`](#log) command
    - [move.c](src/move.c): Implementation of [`move`](#move) command
    - [recover.c](src/recover.c): Implementation of [`recover`](#recover) command
    - [rename.c](src/rename.c): Implementation of [`rename`](#rename) command
    - [report.c](src/report.c): Implementation of [`report`](#report) command
    - [search_cmd.c](src/search_cmd.c): Implementation of [`search`](#search) command
    - [tree.c](src/tree.c): Implementation of [`tree`](#tree) command
    - [utils_filter.c](src/utils_filter.c): Implementation of utility filtering utility functions
    - [utils_sort.c](src/utils_sort.c): Implementation of sorting utility functions (for `scandir()` and `qsort()`)
    - [utils.c](src/utils.c): Implementation of general utility functions
- **DEVELOPMENT DOCS**
    - [Makefile](Makefile): Building rules and compilation settings
    - [CHANGELOG.md](CHANGELOG.md): Version history and changes
    - [DEVLOG.md](DEVLOG.md): Development process and decisions
    - [TODO.md](TODO.md): Planned features and goals

    ---


## Design Choices
- **Makefile** to automate the compilation process: Instead of manually typing all compilation rules every time, I decided to learn how to create a proper Makefile, understanding its syntax, structure and logic.
- **Usage of `getopt_long()` to parse CLI arguments**: I wanted to automate the flag parsing process for each feature, improving UX by allowing any flags in any order.
- **Bitmasks and bitwise operators usage for flags**: I decided to aggregate the parsing and checking of common flags using bitmask. This removed a large boilerplate code and made flag handling cleaner and more efficient.
- **Commands, Context and Flags structures**: Used custom structs to group related data, improving code organization, readability and maintainability.
- **enum type for error codes**: Instead of hardcoding every error, I created a `typedef enum ErrorCode` in [`error_code.h`](include/error_code.h) to improve code's readability.
- **Logging system**: I decided to register all actions performed by destructive features ([`backup`](#backup), [`delete`](#delete), [`move`](#move), [`recover`](#recover) and [`rename`](#rename)), following best practices for CLI tools.

---


## Acknowledgments
- Special thanks to the **CS50x team**, including Professor **David Malan** and the course staff, for their excellent teaching and support throughout "[**CS50's Introduction to Computer Science**](https://pll.harvard.edu/course/cs50-introduction-computer-science)" course.
- I also used **Grok (xAI)** as a learning tool to assist during development of the program. It helped me with:
  - Understanding Makefile
  - Debugging CLI arguments parser
  - Bitmasks and bitwise operators
  - CRC32 hashing implementation
  - Minor issues like choosing the best data type and functions to use

  All core logic, architecture and implementation decisions were made by me. Grok was used strictly as a learning tool.

---


## Future Plans
Improve and polish existing features based on the items listed in the [`BACKLOG`](TODO.md#backlog-postponed) section of [`TODO.md`](TODO.md)

---


## License

MIT License

Copyright (c) 2026 Alexandre D. Ferrari

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
