# B+Tree Key-Value Storage Engine

A compact, restartable key-value database written in modern C++17. It combines an in-memory B+ tree for fast ordered access with a binary write-ahead log (WAL) for persistence and recovery.

The project is intentionally small enough to understand end-to-end—from the interactive command loop, through the storage engine, to index balancing, binary log records, recovery, and compaction.

## Highlights

- **B+ tree index (order 16):** logarithmic key lookup, insertion, and deletion with node splitting, borrowing, and merging.
- **Ordered scans:** leaf nodes are linked in both directions, allowing `SCAN` to walk every key in sorted order.
- **Write-ahead logging:** mutations are appended as compact binary records before the in-memory index is updated.
- **Restart recovery:** the engine replays valid WAL records at startup to reconstruct the index.
- **WAL compaction:** collapses the complete mutation history into the current live key-value state and replaces the log through a temporary file.
- **Interactive CLI:** `PUT`, `GET`, `DEL`, `SCAN`, `COMPACT`, `HELP`, and `EXIT` are available from a simple database prompt.
- **Clean C++ design:** storage concerns are separated across the B+ tree, WAL, engine, CLI, and application entry point.

## Architecture

```text
CLI
 │  commands
 ▼
Engine ───────────────► B+ tree index (in memory)
 │
 ├─ PUT / DEL ────────► append binary WAL record
 │
 ├─ startup ──────────► recover WAL and replay operations
 │
 └─ COMPACT ──────────► write current live state to a compact WAL
```

The index stores string keys and values. The B+ tree uses a fixed maximum of 15 keys per node (order 16); leaf links make full scans efficient and naturally sorted. The WAL stores operation type, key length, value length, key bytes, and—for puts—value bytes.

## Requirements

- Linux or another POSIX-like environment
- GNU Make
- A C++17-compatible compiler, such as `g++`

## Build and run

```bash
make
./storage_engine
```

The default executable is `storage_engine`, and the default WAL path is `data/storage.wal`. `make` creates the `build/` and `data/` directories automatically.

To remove generated build artifacts:

```bash
make clean
```

To also remove the database WAL files and start with an empty local database:

```bash
make clean-all
```

## CLI reference

| Command | Description | Example |
| --- | --- | --- |
| `PUT <key> <value>` | Insert a key or replace its value | `PUT language C++` |
| `GET <key>` | Fetch a value | `GET language` |
| `DEL <key>` | Delete a key | `DEL language` |
| `SCAN` | Print all key-value pairs in sorted key order | `SCAN` |
| `COMPACT` | Rewrite the WAL using only the current live state | `COMPACT` |
| `HELP` | Show available commands | `HELP` |
| `EXIT` | Flush and close the engine | `EXIT` |

Example session:

```text
db> PUT user:1001 Ada Lovelace
OK
db> PUT user:1002 Grace Hopper
OK
db> GET user:1001
--> Ada Lovelace
db> SCAN
user:1001 -> Ada Lovelace
user:1002 -> Grace Hopper
db> DEL user:1001
OK
db> COMPACT
[Engine] WAL compaction completed successfull.
db> EXIT
```

Start the executable again and the remaining records are reconstructed from `data/storage.wal`.

## Persistence model

The authoritative working set is held in memory; the WAL is the persistence and recovery source. Each successful `PUT` or `DEL` is written and flushed to the WAL before the B+ tree is changed. During recovery, the engine reads complete records in order and replays them, stopping safely at an incomplete tail record.

Compaction keeps only the final value for each live key and removes keys that were subsequently deleted. This prevents an unbounded history of updates and deletes from accumulating in the WAL.

## Project structure

```text
src/
├── bptree.hpp / bptree.cpp   B+ tree index and balancing operations
├── wal.hpp / wal.cpp         Binary WAL, recovery, and compaction
├── engine.hpp / engine.cpp   Database API and WAL/index coordination
├── cli.hpp / cli.cpp         Interactive command-line interface
└── main.cpp                  Application entry point
Makefile                      Build and cleanup targets
```


## License

No license file is currently included in this repository.

