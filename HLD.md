# High Level Design (HLD)
## Do Not Disturb (DND) — Client-Server System in C

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [System Overview](#2-system-overview)
3. [Architecture](#3-architecture)
4. [File Structure](#4-file-structure)
5. [Component Design](#5-component-design)
6. [Data Flow](#6-data-flow)
7. [Communication Protocol](#7-communication-protocol)
8. [Data Storage Design](#8-data-storage-design)
9. [Concurrency & Thread Safety](#9-concurrency--thread-safety)
10. [Error Handling Strategy](#10-error-handling-strategy)
11. [Sequence Diagrams](#11-sequence-diagrams)
12. [Advantages](#12-advantages)
13. [Limitations](#13-limitations)
14. [Future Enhancements](#14-future-enhancements)

---

## 1. Introduction

The **Do Not Disturb (DND)** system is a telecom simulation built in C that demonstrates core concepts of:

- TCP socket-based client-server communication
- Multithreaded server architecture
- File-based persistent storage
- Mutex-protected shared state

Subscribers can activate Global or Selective DND modes to block incoming calls. The system handles multiple concurrent clients, persists preferences across restarts, and enforces DND rules in real time.

---

## 2. System Overview

The system has three logical layers:

```
┌─────────────────────────────────────────────────────────┐
│                     CLIENT LAYER                        │
│   Menu-driven terminal UI  ·  Sends commands over TCP   │
└────────────────────────┬────────────────────────────────┘
                         │  TCP Socket (port 8080)
┌────────────────────────▼────────────────────────────────┐
│                     SERVER LAYER                        │
│   Multithreaded  ·  Command parser  ·  Business logic   │
└────────────────────────┬────────────────────────────────┘
                         │  File I/O (mutex-protected)
┌────────────────────────▼────────────────────────────────┐
│                    DATABASE LAYER                       │
│        clients.csv  ·  sessions.csv  (CSV files)        │
└─────────────────────────────────────────────────────────┘
```

### Key Characteristics

| Property       | Implementation                                      |
|----------------|-----------------------------------------------------|
| Communication  | TCP sockets — reliable, ordered, connection-based   |
| Concurrency    | POSIX threads (`pthread`) — one thread per client   |
| Thread Safety  | `pthread_mutex_t` locks on all shared data          |
| Persistence    | CSV files written on every state change             |
| Scalability    | Supports up to 1000 subscribers, 1024 sessions      |

---

## 3. Architecture

### 3.1 Two-Tier Client-Server Architecture

```
  ┌──────────┐        TCP         ┌──────────────────────────────────┐
  │ Client 1 │ ◄────────────────► │                                  │
  └──────────┘                    │           SERVER                 │
  ┌──────────┐        TCP         │                                  │
  │ Client 2 │ ◄────────────────► │  Thread 1 │ Thread 2 │ Thread N  │
  └──────────┘                    │      ↓          ↓          ↓     │
  ┌──────────┐        TCP         │  ┌────────────────────────────┐  │
  │ Client N │ ◄────────────────► │  │   Shared In-Memory State   │  │
  └──────────┘                    │  │   (mutex-protected)        │  │
                                  └──────────────┬───────────────────┘
                                                 │ File I/O
                                  ┌──────────────▼───────────────────┐
                                  │  clients.csv  │  sessions.csv     │
                                  └──────────────────────────────────┘
```

### 3.2 Thread Model

```
main() [accept loop]
  │
  ├── accept() ──► malloc(client_fd)
  │                     │
  │               pthread_create() ──► client_thread()
  │                                         │
  │                                    handle_client()
  │                                         │
  │                              ┌──────────┴──────────┐
  │                              │  recv loop           │
  │                              │  parse command       │
  │                              │  call DND logic      │
  │                              │  send response       │
  │                              └──────────────────────┘
  │
  └── (back to accept() immediately)
```

Each thread is **detached** — it cleans up automatically when the client disconnects.

---

## 4. File Structure

```
DND/
├── Makefile                    ← Build automation
├── EXECUTION_STEPS.md          ← Step-by-step execution guide
├── HLD.md                      ← This document
│
├── src/                        ← All source files
│   ├── server.c                ← Server entry point, accept loop
│   ├── client.c                ← Client UI and command sender
│   ├── dnd_utils.c             ← Command parser, client thread handler
│   ├── dnd_functions.c         ← DND business logic (activate/deactivate/call)
│   ├── dnd_db.c                ← CSV database read/write
│   ├── sessions.c              ← Session lifecycle management
│   └── socket_utils.c          ← TCP socket creation utilities
│
├── include/                    ← Header files
│   ├── common.h                ← All system includes, color macros
│   ├── dnd.h                   ← Core types, constants, function declarations
│   ├── config.h                ← Buffer size constants
│   ├── dnd_utils.h             ← validate_number(), client_thread()
│   ├── sessions.h              ← session_entry_t, session function declarations
│   └── socket_utils.h          ← Socket utility declarations
│
├── data/                       ← Persistent storage
│   ├── clients.csv             ← Subscriber DND preferences
│   └── sessions.csv            ← Active client sessions
│
└── bin/                        ← Compiled binaries (generated by make)
    ├── server
    └── client
```

---

## 5. Component Design

### 5.1 Client (`client.c`)

**Responsibility:** Subscriber-facing terminal interface.

```
┌─────────────────────────────────────────┐
│               CLIENT                    │
│                                         │
│  main()                                 │
│   ├── create_and_connect_socket()       │
│   ├── send HELLO <number>               │
│   └── menu loop                         │
│        ├── 1. ACTIVATE [GLOBAL|SEL]     │
│        ├── 2. DEACTIVATE                │
│        ├── 3. MAKECALL <to>             │
│        └── 4. EXIT                      │
│                                         │
│  send_and_receive()                     │
│   ├── send(cmd)                         │
│   └── recv() → print response           │
└─────────────────────────────────────────┘
```

### 5.2 Server (`server.c`)

**Responsibility:** Accept connections, spawn threads.

```
┌─────────────────────────────────────────┐
│               SERVER                    │
│                                         │
│  main()                                 │
│   ├── load_db()                         │
│   ├── sessions_init()                   │
│   ├── create_listening_socket()         │
│   └── while(true)                       │
│        ├── accept()                     │
│        ├── sessions_add()               │
│        ├── pthread_create(client_thread)│
│        └── pthread_detach()             │
└─────────────────────────────────────────┘
```

### 5.3 Command Handler (`dnd_utils.c`)

**Responsibility:** Parse raw TCP data, dispatch to business logic.

```
handle_client(fd)
  │
  ├── HELLO    → sessions_set_subscriber()
  ├── ACTIVATE → validate → parse_type() → activate_dnd()
  ├── DEACTIVATE → validate → deactivate_dnd()
  ├── MAKECALL → validate → make_call()
  └── EXIT     → sessions_remove() → close(fd)
```

### 5.4 DND Logic (`dnd_functions.c`)

**Responsibility:** Core business rules.

```
activate_dnd()
  ├── lock db_mutex
  ├── find_or_create(number)
  ├── check already active → ERR
  ├── set active=1, type, blocked[]
  ├── save_db()
  └── unlock db_mutex

deactivate_dnd()
  ├── lock db_mutex
  ├── find_entry(number) → ERR if not found
  ├── check already inactive → ERR
  ├── set active=0, type=NONE, blocked_count=0
  ├── save_db()
  └── unlock db_mutex

make_call(from, to)
  ├── lock db_mutex
  ├── find_entry(to)
  │    ├── not found / inactive → ESTABLISHED
  │    ├── DND_GLOBAL           → BLOCKED_BY_GLOBAL
  │    └── DND_SELECTIVE        → check blocked list
  │         ├── from in list    → BLOCKED_BY_SELECTIVE
  │         └── from not in list→ ESTABLISHED
  └── unlock db_mutex
```

### 5.5 Database (`dnd_db.c`)

**Responsibility:** In-memory storage backed by CSV.

```
load_db()  → fopen(clients.csv, "r") → parse_line() × N → clients[]
save_db()  → fopen(clients.csv, "w") → write all clients[] entries
find_entry()      → linear search in clients[] by number
find_or_create()  → find_entry() or append new entry to clients[]
```

### 5.6 Sessions (`sessions.c`)

**Responsibility:** Track live connections.

```
sessions_init()           → clear g_sessions[], write CSV header
sessions_add(fd,ip,port)  → find free slot → fill → save_sessions_locked()
sessions_set_subscriber() → update subscriber name → save
sessions_remove(fd)       → clear slot → save
```

### 5.7 Socket Utilities (`socket_utils.c`)

**Responsibility:** Abstract TCP socket creation.

```
create_listening_socket(port)
  → socket() → bind() → listen() → return fd

create_and_connect_socket(ip, port)
  → socket() → inet_pton() → connect() → return fd
```

---

## 6. Data Flow

### 6.1 DFD Level 0 — Context Diagram

```
                    ┌─────────────────────┐
                    │                     │
  Subscriber ──────►│    DND SYSTEM       │──────► Subscriber
  (commands)        │                     │        (responses)
                    └──────────┬──────────┘
                               │
                        ┌──────▼──────┐
                        │  clients.csv│
                        │ sessions.csv│
                        └─────────────┘
```

### 6.2 DFD Level 1 — Server Functions

```
                        ┌─────────────┐
                        │   SERVER    │
                        └──────┬──────┘
              ┌────────────────┼────────────────┐
              ▼                ▼                ▼                ▼
        ┌──────────┐   ┌────────────┐   ┌──────────┐   ┌──────────┐
        │ ACTIVATE │   │ DEACTIVATE │   │ MAKECALL │   │   EXIT   │
        └────┬─────┘   └─────┬──────┘   └────┬─────┘   └────┬─────┘
             │               │               │               │
             ▼               ▼               ▼               ▼
        Update DB       Update DB       Check DB        Remove Session
        Save CSV        Save CSV        Return result   Close socket
```

### 6.3 DFD Level 2 — Activate DND Detail

```
Client Input
    │
    ▼
Validate number (10 digits, all numeric)
    │
    ├── FAIL → send "ERR Invalid subscriber number"
    │
    ▼
Parse DND type (GLOBAL / SELECTIVE)
    │
    ▼
[If SELECTIVE] Split blocked list by ',' or '|'
    │
    ▼
Lock db_mutex
    │
    ▼
find_or_create(number) in clients[]
    │
    ├── already active → unlock → send "ERR Already active"
    │
    ▼
Set active=1, type, blocked[]
    │
    ▼
save_db() → write clients.csv
    │
    ▼
Unlock db_mutex
    │
    ▼
send "OK Activated"
```

---

## 7. Communication Protocol

All messages are plain-text, newline-terminated (`\n`).

### Client → Server Commands

| Command | Format | Description |
|---------|--------|-------------|
| HELLO | `HELLO <number>` | Register subscriber for this session |
| ACTIVATE | `ACTIVATE <number> GLOBAL` | Activate Global DND |
| ACTIVATE | `ACTIVATE <number> SELECTIVE <n1\|n2>` | Activate Selective DND with block list |
| DEACTIVATE | `DEACTIVATE <number>` | Deactivate DND |
| MAKECALL | `MAKECALL <from> <to>` | Simulate a call |
| EXIT | `EXIT` | Disconnect gracefully |

### Server → Client Responses

| Response | Meaning |
|----------|---------|
| `OK HELLO` | Subscriber registered |
| `OK Activated` | DND activated successfully |
| `OK Deactivated` | DND deactivated successfully |
| `ESTABLISHED` | Call went through |
| `BLOCKED_BY_GLOBAL` | Callee has Global DND active |
| `BLOCKED_BY_SELECTIVE` | Caller is in callee's block list |
| `OK Bye` | Server acknowledged exit |
| `ERR <reason>` | Operation failed with reason |

---

## 8. Data Storage Design

### 8.1 `clients.csv` — Subscriber Preferences

```
number,active,type,blocked_list
1234567890,1,GLOBAL,
9876543210,1,SELECTIVE,1234567890|1111111111
5555555555,0,NONE,
```

| Field | Type | Description |
|-------|------|-------------|
| `number` | string (10 digits) | Subscriber phone number |
| `active` | 0 or 1 | DND currently active |
| `type` | NONE / GLOBAL / SELECTIVE | DND mode |
| `blocked_list` | pipe-separated numbers | Numbers blocked (SELECTIVE only) |

### 8.2 `sessions.csv` — Active Connections

```
fd,ip,port,subscriber,connected_at,updated_at
5,127.0.0.1,54321,1234567890,2025-01-13T10:00:00Z,2025-01-13T10:05:00Z
```

| Field | Description |
|-------|-------------|
| `fd` | Socket file descriptor |
| `ip` | Client IP address |
| `port` | Client port |
| `subscriber` | Registered subscriber number |
| `connected_at` | ISO 8601 connection timestamp |
| `updated_at` | ISO 8601 last activity timestamp |

### 8.3 In-Memory Structures

```c
// Subscriber record (dnd.h)
typedef struct {
    char number[32];
    int active;
    dnd_type_t type;              // DND_NONE | DND_GLOBAL | DND_SELECTIVE
    char blocked[100][32];        // up to 100 blocked numbers
    int blocked_count;
} client_entry_t;

// Session record (sessions.h)
typedef struct {
    int in_use;
    int fd;
    char ip[64];
    int port;
    char subscriber[64];
    time_t connected_at;
    time_t updated_at;
} session_entry_t;
```

---

## 9. Concurrency & Thread Safety

### 9.1 Mutex Usage

Two independent mutexes protect two independent shared resources:

| Mutex | Protects | Defined In |
|-------|----------|------------|
| `db_mutex` | `clients[]` array + `clients.csv` | `dnd_functions.c` |
| `g_sess_mx` | `g_sessions[]` array + `sessions.csv` | `sessions.c` |

### 9.2 Lock/Unlock Pattern

Every function that touches shared state follows this pattern:

```
pthread_mutex_lock(&mutex)
  ↓
  [read or modify shared data]
  ↓
  [write to CSV if needed]
  ↓
pthread_mutex_unlock(&mutex)
```

Early returns (on error) always unlock before returning — no deadlocks.

### 9.3 Thread Lifecycle

```
Server accept() → malloc(fd) → pthread_create() → pthread_detach()
                                      ↓
                               client_thread(fd)
                                      ↓
                               handle_client(fd)   ← recv loop
                                      ↓
                               sessions_remove(fd)
                               close(fd)
                               return NULL          ← thread auto-freed
```

---

## 10. Error Handling Strategy

| Error Condition | Detection Point | Response |
|-----------------|-----------------|----------|
| Invalid number format | `validate_number()` in `dnd_utils.c` | `ERR Invalid subscriber number` |
| DND already active | `activate_dnd()` — checks `e->active` | `ERR Already active, deactivate first` |
| DND already inactive | `deactivate_dnd()` — checks `!e->active` | `ERR Already deactivated` |
| Number not in DB | `find_entry()` returns NULL | `ERR Number not found` |
| Calling yourself | `strcmp(from, to)` in `dnd_utils.c` | `ERR Cannot call yourself` |
| Missing parameters | NULL token check after `strtok_r` | `ERR Missing params` |
| DB file open failure | `fopen()` return check | `[FATAL]` log + return -1 |
| Max clients reached | `client_count >= MAX_CLIENTS` | NULL from `find_or_create` |
| Client disconnected | `recv()` returns 0 or negative | Exit recv loop, clean up session |

---

## 11. Sequence Diagrams

### 11.1 Activate Global DND

```
Client                    Server                   clients.csv
  │                          │                          │
  │── ACTIVATE 1234567890 GLOBAL ──►│                   │
  │                          │                          │
  │                    validate_number()                │
  │                    parse_type("GLOBAL")             │
  │                    lock db_mutex                    │
  │                    find_or_create()                 │
  │                    set active=1, type=GLOBAL        │
  │                    save_db() ──────────────────────►│
  │                    unlock db_mutex                  │
  │                          │                          │
  │◄── OK Activated ─────────│                          │
```

### 11.2 Make Call — Blocked by Global DND

```
Caller (A)               Server                   clients.csv
  │                          │                          │
  │── MAKECALL A B ─────────►│                          │
  │                          │                          │
  │                    validate A, B                    │
  │                    lock db_mutex                    │
  │                    find_entry(B) ─────────────────►│
  │                    B.active=1, type=GLOBAL          │
  │                    unlock db_mutex                  │
  │                          │                          │
  │◄── BLOCKED_BY_GLOBAL ────│                          │
```

### 11.3 Make Call — Established

```
Caller (A)               Server                   clients.csv
  │                          │                          │
  │── MAKECALL A B ─────────►│                          │
  │                          │                          │
  │                    find_entry(B) ─────────────────►│
  │                    B not found / B.active=0         │
  │                    unlock db_mutex                  │
  │                          │                          │
  │◄── ESTABLISHED ──────────│                          │
```

### 11.4 Client Exit

```
Client                    Server                  sessions.csv
  │                          │                          │
  │── EXIT ─────────────────►│                          │
  │                          │                          │
  │◄── OK Bye ───────────────│                          │
  │                    sessions_remove(fd) ────────────►│
  │                    close(fd)                        │
  │                    thread exits                     │
  │                          │                          │
```

---

## 12. Advantages

- **Lightweight** — No external libraries; pure C with POSIX APIs
- **Concurrent** — Each client handled in its own thread; no client blocks another
- **Persistent** — All DND preferences survive server restarts via CSV
- **Modular** — Each `.c` file has a single clear responsibility
- **Thread-safe** — All shared state protected by mutexes; no race conditions
- **Extensible** — New commands can be added to `handle_client()` without touching other modules
- **Debuggable** — Verbose `[DEBUG]`/`[INFO]`/`[WARN]` logs on server side

---

## 13. Limitations

- **No authentication** — Any client can activate/deactivate DND for any number
- **Linear search** — `find_entry()` does O(n) scan; degrades with large subscriber counts
- **Single CSV file** — `save_db()` rewrites the entire file on every change; not efficient at scale
- **No TLS** — Communication is unencrypted plain text over TCP
- **No signal handling** — Server has no graceful shutdown (Ctrl+C leaves sessions.csv dirty)
- **In-memory only** — If server crashes mid-write, CSV may be partially written
- **10-digit number only** — `validate_number()` rejects international formats

---

## 14. Future Enhancements

| Enhancement | Description |
|-------------|-------------|
| Authentication | Token or PIN-based subscriber verification |
| Hash map for DB | Replace linear search with hash table for O(1) lookups |
| Append-only writes | Write only changed records instead of full CSV rewrite |
| TLS encryption | Wrap TCP sockets with OpenSSL for secure communication |
| Signal handling | `SIGINT` handler to call `sessions_shutdown()` on exit |
| Time-based DND | Schedule DND activation/deactivation by time window |
| REST API layer | Expose DND operations over HTTP for web/mobile clients |
| Admin console | Server-side CLI to view active sessions and subscriber states |
