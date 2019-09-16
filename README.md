# kbdxviewer
* Version: **0.1.1**
* Description: Interactively access or dump KeePass2 `.kdbx` database files
  as XML, CSV or in text/tree representation.
* Using: cryptkeyper/libcx9r from https://github.com/jhagmar/cryptkeyper
* Based on: https://max-weller.github.io/kdbx-viewer
* License: GPLv2

## Usage
```
kbdxviewer [-i|-t|-x|-c] [-p PW] [-u] [[-s|-S] STR] [-V|-h] [KDBX]

  Commands:
    -i          Interactive viewing (default if no search is used)
    -t          Output as Tree (default if search is used)
    -x          Output as XML
    -c          Output as CSV

  Options:
    -p PW       Decrypt file KDBX using PW  (Never use on shared computers
                as PW can be seen in the process list!)
    -u          Display Password fields Unmasked
    [-s] STR    Show database entries with STR in the Title
    -S STR      Show database entries with STR in any field
    -V          Display Version
    -h          Display this Help text
The configfile ~/.kdbxviewer is used for reading and storing KDBX files.
Website:        https://gitlab.com/pepa65/kdbxviewer
```
