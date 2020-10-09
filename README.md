# kbdxviewer
* Version: **0.1.6**
* Description: View KeePass2 `.kdbx` database files in various formats and ways
  as XML, CSV or in text/tree representation.
* Using: cryptkeyper/libcx9r from https://github.com/jhagmar/cryptkeyper
* After: https://github.com/luelista/kdbxviewer
* License: GPLv2
* Required: libgcrypt-dev libstfl-dev zlib1g-dev libexpat1-dev

## Usage
```
  kdbxviewer [-i|-t|-x|-c|-h|-V] [-A] [-p PW] [-u] [[-s|-S] STR] [-d KDBX]
Commands:
  -i          Interactive viewing (default if no search is used)
  -t          Output as Tree (default if search is used)
  -x          Output as XML
  -c          Output as CSV
  -h          Display this Help text
  -V          Display Version
Options:
  -A          Analyse / debug
  -p PW       Decrypt file KDBX using PW  (Never use on shared
                computers as PW can be seen in the process list!)
  -u          Display Password fields Unmasked
  [-s] STR    Select only entries with STR in the Title
  -S STR      Select only entries with STR in any field
  -d KDBX     Use KDBX as the path/filename for the Database
The configfile ~/.kdbxviewer is used for storing KDBX database filenames.
Website:      https://gitlab.com/pepa65/kdbxviewer
```
