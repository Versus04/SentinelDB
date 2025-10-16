# SentinelDB

A lightweight, persistent key-value database written in C++ with TCP networking support. SentinelDB provides a simple Redis-like interface for storing and retrieving data with built-in persistence through write-ahead logging (WAL) and snapshot capabilities.

## Features

- 🔑 **Key-Value Storage**: Fast in-memory hash map for storing string key-value pairs
- 💾 **Persistence**: Write-ahead logging (WAL) ensures data durability across restarts
- 📸 **Snapshots**: Create point-in-time backups of your database
- 🌐 **TCP Server**: Network-accessible via TCP on port 8080
- 🚀 **Simple Protocol**: Redis-like command interface
- ⚡ **Fast**: O(1) average time complexity for GET, SET, and DEL operations

## Architecture

```
Client 1 ──┐
Client 2 ──┤── TCP Server (Port 8080) ── SentinelDB ── data.log
Client 3 ──┘                                      └── snapshot.db
```

### Components

- **SentinelDB Class**: Core database engine with in-memory storage
- **TCP Server**: Single-threaded network server accepting client connections
- **Write-Ahead Log (WAL)**: Persistent log file (`data.log`) for crash recovery
- **Snapshot System**: Point-in-time database backups (`snapshot.db`)

## Building

### Prerequisites

- **Windows**: MinGW-w64 or Visual Studio with C++11 support
- **Winsock2**: Included in Windows SDK

### Compilation

```bash
# Navigate to the src directory
cd src

# Compile with g++
g++ main.cpp -o sentinel -lws2_32

# Run the server
./sentinel
```

## Usage

### Starting the Server

```bash
./sentinel
```

You should see:
```
Socket created successfully
Server listening on port 8080...
```

### Connecting to the Server

Use any TCP client tool:

**Using ncat:**
```bash
ncat localhost 8080
```

**Using telnet:**
```bash
telnet localhost 8080
```

**Using PuTTY:**
- Host: `localhost`
- Port: `8080`
- Connection type: Raw or Telnet

## Commands

### SET - Store a Key-Value Pair
```
SET key value
```
**Response:** `+OK`

**Example:**
```
SET name John
+OK
SET age 25
+OK
```

### GET - Retrieve a Value
```
GET key
```
**Response:** `value` or `(nil)` if key doesn't exist

**Example:**
```
GET name
John
GET nonexistent
(nil)
```

### DEL - Delete a Key
```
DEL key
```
**Response:** `+OK`

**Example:**
```
DEL name
+OK
```

### SAVE - Create a Snapshot
```
SAVE
```
**Response:** `+Snapshot saved`

Creates a snapshot file (`snapshot.db`) with the current database state.

### EXIT - Disconnect
```
EXIT
```
**Response:** `BYE`

Closes the client connection and shuts down the server.

## Persistence

### Write-Ahead Log (WAL)
- **File:** `data.log`
- **Purpose:** Records all SET and DEL operations
- **Format:** Text-based log entries
  ```
  SET key1 value1
  SET key2 value2
  DEL key1
  ```
- **Recovery:** On startup, the database replays the log to restore state

### Snapshots
- **File:** `snapshot.db`
- **Purpose:** Point-in-time backup of all key-value pairs
- **Created:** Manually via the `SAVE` command
- **Format:** Plain text with one key-value pair per line
  ```
  key1 value1
  key2 value2
  ```

## Protocol

SentinelDB uses a simple text-based protocol similar to Redis:

### Request Format
```
COMMAND [arguments]\r\n
```

### Response Format
- **Success:** `+OK\r\n` or `+message\r\n`
- **Error:** `-ERR message\r\n`
- **Value:** `value\r\n`
- **Nil:** `(nil)\r\n`

## Example Session

```
$ ncat localhost 8080

SET username alice
+OK

SET email alice@example.com
+OK

GET username
alice

GET email
alice@example.com

DEL email
+OK

GET email
(nil)

SAVE
+Snapshot saved

EXIT
BYE
```

## File Structure

```
SentinelDB/
├── src/
│   └── main.cpp          # Main source code
├── data/                 # Data directory (created at runtime)
├── data.log              # Write-ahead log (created at runtime)
├── snapshot.db           # Snapshot file (created on SAVE)
└── README.md             # This file
```

## Technical Details

### Database Implementation
- **Storage:** `std::unordered_map<std::string, std::string>`
- **Time Complexity:**
  - GET: O(1) average
  - SET: O(1) average
  - DEL: O(1) average
  - SAVE: O(n) where n is the number of keys

### Network Implementation
- **Protocol:** TCP/IP
- **Port:** 8080
- **Socket Library:** Winsock2 (Windows)
- **Threading Model:** Single-threaded (handles one client at a time)
- **Buffer Size:** 1024 bytes

### Persistence Strategy
- **WAL (Write-Ahead Logging):** Every write operation is immediately flushed to disk
- **Snapshot:** Manual snapshots via SAVE command
- **Recovery:** On startup, replays the WAL to restore database state

## Limitations

- **Single-threaded:** Handles one client connection at a time
- **Windows-only:** Uses Winsock2 (Windows-specific)
- **No authentication:** No security or access control
- **String values only:** Values with spaces are not fully supported in current implementation
- **No persistence for GET:** Read operations are not logged
- **No compaction:** WAL file grows indefinitely without manual cleanup

## Future Enhancements

- [ ] Multi-client support (threading or async I/O)
- [ ] Cross-platform compatibility (Linux/macOS support)
- [ ] Authentication and access control
- [ ] Support for complex data types (lists, sets, hashes)
- [ ] WAL compaction and rotation
- [ ] Automatic snapshots at regular intervals
- [ ] Binary protocol for better performance
- [ ] Pub/Sub messaging
- [ ] Replication and high availability

## Learning Outcomes

This project demonstrates:
- ✅ **TCP socket programming** in C++ (Winsock2)
- ✅ **Client-server architecture**
- ✅ **Database persistence** (WAL and snapshots)
- ✅ **In-memory data structures** (hash maps)
- ✅ **Protocol design** (text-based command protocol)
- ✅ **File I/O** in C++
- ✅ **Network programming** concepts

## License

This is a learning project. Feel free to use and modify as needed.

## Author

Built as a learning exercise in C++ networking and database systems.

## Acknowledgments

Inspired by Redis and other key-value stores.
