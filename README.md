# kbdxviewer
**KDBX Viewer 0.0.2 - *Dump KeePass2 .kdbx databases in various formats* **

Interactively access or dump KeePass2 `.kdbx` database files as XML, CSV or as
a tree representation

* Using: **cryptkeyper/libcx9r**
* Based on: **max-weller.github.io/kdbx-viewer**
* License: GPLv2

## Usage
```
kbdxviewer [-v] [-t|-x|-c|-i] [-p PW] [-u] [-s|-S STR] KDBX

### Commands
    -t        Dump the KDBX database as a Tree
    -x        Dump the KDBX database in XML format
    -c        Dump the KDBX database in CSV format
    -i        Interactive querying of the KDBX database

### Options
    -p *PW*     Decrypt file *KDBX* using *PW*  (Never use on shared computers
              as PW can be seen in the process list!)
    -s *STR*    Show database entries with *STR* in the Title
    -S *STR*    Show database entries with *STR* in any field
    -u        Display Password fields Unmasked
    -h/-?     Display this Help text

