# kbdxviewer
* Version: **0.1.0**
* Description: Interactively access or dump KeePass2 `.kdbx` database files
  as XML, CSV or in text/tree representation.
* Using: cryptkeyper/libcx9r from https://github.com/jhagmar/cryptkeyper
* Based on: https://max-weller.github.io/kdbx-viewer
* License: GPLv2

## Usage
```
kbdxviewer [-i|-t|-x|-c] [-p PW] [-u] [[-s|-S] STR] [-v|-V|-h] [KDBX]

  Commands:
    -i          Interactive viewing (default if no -s/-S is used)
    -t          Output as Tree (default if -s/-S is used)
    -x          Output as XML
    -c          Output as CSV

  Options:
    -p PW       Decrypt file KDBX using PW  (Never use on shared computers
                as PW can be seen in the process list!)
    [-s] STR    Show database entries with STR in the Title
    -S STR      Show database entries with STR in any field
    -u          Display Password fields Unmasked
    -V          Display Version
    -v          More Verbose/debug output
    -h          Display this Help text
The configfile ~/.kdbxviewer is used for reading and storing KDBX files.
Website:        https://gitlab.com/pepa65/kdbxviewer
```
