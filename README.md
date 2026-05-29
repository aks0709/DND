# 📵 Do Not Disturb (DND) — C Client-Server System

A telecom-inspired **Do Not Disturb** simulation built entirely in C, featuring a multithreaded TCP server, real-time call filtering, and file-based persistent storage.

---

## 📌 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Build & Run](#build--run)
- [Usage](#usage)
- [DND Modes](#dnd-modes)
- [Protocol](#protocol)
- [Data Storage](#data-storage)
- [Concurrency & Thread Safety](#concurrency--thread-safety)
- [Error Handling](#error-handling)
- [Limitations](#limitations)
- [Future Enhancements](#future-enhancements)
- [Documentation](#documentation)

---

## Overview

The DND system simulates a telecom call-blocking service where subscribers can:

- Activate **Global DND** — block all incoming calls
- Activate **Selective DND** — block calls from specific numbers only
- **Deactivate** DND at any time
- **Simulate calls** between subscribers and see real-time block/allow decisions

Built to demonstrate core systems programming concepts:

- TCP socket-based client-server communication
- Multithreaded server with one thread per client
- Mutex-protected shared state across concurrent threads
- CSV file-based persistence that survives server restarts

---

## Features

- **Concurrent clients** — multiple subscribers connect and operate simultaneously
- **Two DND modes** — Global (block all) and Selective (block specific numbers)
- **Real-time call decisions** — ESTABLISHED / BLOCKED\_BY\_GLOBAL / BLOCKED\_BY\_SELECTIVE
- **Persistent storage** — DND preferences saved to CSV, restored on server restart
- **Session tracking** — active connections logged to `sessions.csv` with timestamps
- **Input validation** — 10-digit number enforcement, self-call prevention
- **Colored terminal UI** — menu-driven client interface with ANSI colors
- **Zero external dependencies** — pure C with POSIX APIs only

---

## Architecture

```
┌──────────┐              ┌─────────────────────────────────┐
│ Client 1 │◄────TCP─────►│                                 │
└──────────┘              │           SERVER                │
┌──────────┐              │                                 │
│ Client 2 │◄────TCP─────►│  Thread 1 │ Thread 2 │Thread N  │
└──────────┘              │      ↓          ↓         ↓     │
┌──────────┐              │  ┌─────────────────────────┐    │
│ Client N │◄────TCP─────►│  │  Shared In-Memory State │    │
└──────────┘              │  │   (mutex-protected)     │    │
                          └──────────────┬──────────────────┘
                                         │ File I/O
                          ┌──────────────▼──────────────────┐
                          │  clients.csv  │  sessions.csv    │
                          └─────────────────────────────────┘
```

The server spawns a **detached thread** for every accepted connection. All threads share the subscriber database protected by `pthread_mutex_t`.

---

## Project Structure

```
DND/
├── Makefile
├── README.md
├── HLD.md                  ← Full High Level Design document
├── EXECUTION_STEPS.md      ← Detailed step-by-step execution guide
│
├── src/
│   ├── server.c            ← Entry point, accept loop, thread spawning
│   ├── client.c            ← Terminal UI, sends commands, prints responses
│   ├── dnd_utils.c         ← Command parser, client thread handler
│   ├── dnd_functions.c     ← activate_dnd, deactivate_dnd, make_call
│   ├── dnd_db.c            ← load_db, save_db, find_entry, find_or_create
│   ├── sessions.c          ← Session lifecycle, sessions.csv management
│   └── socket_utils.c      ← TCP socket creation for client and server
│
├── include/
│   ├── common.h            ← All system includes, ANSI color macros
│   ├── dnd.h               ← Core types (dnd_type_t, client_entry_t), constants
│   ├── config.h            ← Buffer size constants
│   ├── dnd_utils.h         ← validate_number(), client_thread() declarations
│   ├── sessions.h          ← session_entry_t, session function declarations
│   └── socket_utils.h      ← Socket utility declarations
│
├── data/
│   ├── clients.csv         ← Subscriber DND preferences (persistent)
│   └── sessions.csv        ← Active session log (reset on server start)
│
└── bin/                    ← Compiled binaries (created by make)
    ├── server
    └── client
```

---

## Prerequisites

| Requirement | Details |
|-------------|---------|
| OS | Linux / macOS (POSIX-compliant) |
| Compiler | GCC with C11 support |
| Libraries | POSIX threads (`-lpthread`) — standard on Linux/macOS |
| Build tool | GNU Make |

---

## Build & Run

### 1. Clone the repository

```bash
git clone https://github.com/<your-username>/DND.git
cd DND
```

### 2. Build

```bash
make
```

This compiles both `bin/server` and `bin/client`.

To clean binaries:

```bash
make clean
```

### 3. Start the server

```bash
./bin/server          # default port 8080
./bin/server 9090     # custom port
```

### 4. Connect a client (in a new terminal)

```bash
./bin/client 127.0.0.1 8080
```

Open multiple terminals to simulate concurrent subscribers.

---

## Usage

After connecting, enter your **10-digit subscriber number**. The menu appears:

```
╔════════════════════════════════════╗
║           📋 DND MENU              ║
╠════════════════════════════════════╣
║ 1. Activate DND                    ║
║ 2. Deactivate DND                  ║
║ 3. Make Call                       ║
║ 4. Exit                            ║
╚════════════════════════════════════╝
```

### Activate Global DND
```
Enter your choice: 1
Enter DND mode (GLOBAL/SELECTIVE): GLOBAL
SERVER: OK Activated
```

### Activate Selective DND
```
Enter your choice: 1
Enter DND mode (GLOBAL/SELECTIVE): SELECTIVE
Enter numbers to block (comma or | separated): 9876543210|1111111111
SERVER: OK Activated
```

### Make a Call
```
Enter your choice: 3
Enter receiver number: 9876543210
SERVER: BLOCKED_BY_GLOBAL
```

```
Enter your choice: 3
Enter receiver number: 5555555555
SERVER: ESTABLISHED
```

### Deactivate DND
```
Enter your choice: 2
SERVER: OK Deactivated
```

---

## DND Modes

| Mode | Behavior |
|------|----------|
| **GLOBAL** | All incoming calls are blocked regardless of caller |
| **SELECTIVE** | Only calls from numbers in the blocked list are rejected |
| **NONE** (default) | All calls go through |

### Call Decision Logic

```
make_call(from → to)
  ├── to not found or DND inactive  →  ESTABLISHED
  ├── to has GLOBAL DND             →  BLOCKED_BY_GLOBAL
  └── to has SELECTIVE DND
       ├── from is in blocked list  →  BLOCKED_BY_SELECTIVE
       └── from is not in list      →  ESTABLISHED
```

---

## Protocol

All messages are plain-text, newline-terminated over TCP.

### Client → Server

| Command | Format |
|---------|--------|
| Register | `HELLO <number>` |
| Activate Global | `ACTIVATE <number> GLOBAL` |
| Activate Selective | `ACTIVATE <number> SELECTIVE <n1\|n2\|n3>` |
| Deactivate | `DEACTIVATE <number>` |
| Make Call | `MAKECALL <from> <to>` |
| Exit | `EXIT` |

### Server → Client

| Response | Meaning |
|----------|---------|
| `OK HELLO` | Subscriber registered |
| `OK Activated` | DND activated |
| `OK Deactivated` | DND deactivated |
| `ESTABLISHED` | Call connected |
| `BLOCKED_BY_GLOBAL` | Callee has Global DND |
| `BLOCKED_BY_SELECTIVE` | Caller is in callee's block list |
| `OK Bye` | Graceful disconnect acknowledged |
| `ERR <reason>` | Operation failed |

---

## Data Storage

### `data/clients.csv`

Stores subscriber DND preferences. Persists across server restarts.

```
number,active,type,blocked_list
1234567890,1,GLOBAL,
9876543210,1,SELECTIVE,1234567890|1111111111
5555555555,0,NONE,
```

| Field | Description |
|-------|-------------|
| `number` | 10-digit subscriber number |
| `active` | `1` = DND on, `0` = DND off |
| `type` | `GLOBAL`, `SELECTIVE`, or `NONE` |
| `blocked_list` | Pipe-separated numbers (SELECTIVE only) |

### `data/sessions.csv`

Tracks currently connected clients. Reset on every server start.

```
fd,ip,port,subscriber,connected_at,updated_at
5,127.0.0.1,54321,1234567890,2025-01-13T10:00:00Z,2025-01-13T10:05:00Z
```

---

## Concurrency & Thread Safety

Two independent mutexes protect two independent shared resources:

| Mutex | Protects |
|-------|----------|
| `db_mutex` | `clients[]` array + `clients.csv` writes |
| `g_sess_mx` | `g_sessions[]` array + `sessions.csv` writes |

Every function that touches shared state locks before access and unlocks before every return path — including error returns — preventing deadlocks.

Thread lifecycle:

```
accept() → malloc(fd) → pthread_create() → pthread_detach()
                               ↓
                        client_thread(fd)
                               ↓
                        handle_client()  ← recv loop
                               ↓
                        sessions_remove()
                        close(fd)
                        return NULL      ← auto-freed
```

---

## Error Handling

| Scenario | Response |
|----------|----------|
| Invalid number (not 10 digits) | `ERR Invalid subscriber number` |
| DND already active | `ERR Already active, deactivate first` |
| DND already inactive | `ERR Already deactivated` |
| Number not found in DB | `ERR Number not found` |
| Calling yourself | `ERR Cannot call yourself` |
| Missing command parameters | `ERR Missing params` |
| Unknown command | `ERR Unknown command` |

---

## Limitations

- No authentication — any client can modify any subscriber's DND
- `find_entry()` is O(n) linear scan — not suitable for very large datasets
- `save_db()` rewrites the entire CSV on every change
- No TLS — communication is unencrypted plain text
- No graceful shutdown — `Ctrl+C` does not clean up `sessions.csv`
- Only 10-digit numbers supported — no international format

---

## Future Enhancements

| Enhancement | Description |
|-------------|-------------|
| Authentication | PIN or token-based subscriber verification |
| Hash map DB | O(1) lookups replacing linear scan |
| Append-only writes | Write only changed records to CSV |
| TLS support | OpenSSL wrapper for encrypted communication |
| Signal handling | `SIGINT` handler for graceful server shutdown |
| Time-based DND | Schedule DND by time window |
| REST API | HTTP interface for web/mobile clients |
| Admin console | Server-side CLI to inspect sessions and subscriber state |

---

## Documentation

| Document | Description |
|----------|-------------|
| [`HLD.md`](HLD.md) | Full High Level Design — architecture, data flow, sequence diagrams, LLD |
| [`EXECUTION_STEPS.md`](EXECUTION_STEPS.md) | Step-by-step internal execution walkthrough |

---

## Tech Stack

![C](https://img.shields.io/badge/Language-C-blue?style=flat&logo=c)
![POSIX](https://img.shields.io/badge/API-POSIX-lightgrey?style=flat)
![Threads](https://img.shields.io/badge/Concurrency-pthreads-orange?style=flat)
![TCP](https://img.shields.io/badge/Network-TCP%20Sockets-green?style=flat)
![CSV](https://img.shields.io/badge/Storage-CSV%20Files-yellow?style=flat)
