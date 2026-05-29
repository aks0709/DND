# DND System — Step-by-Step Execution Flow

---

## 1. Build the Project

Run from the project root (`DND/`):

```bash
make
```

What happens internally:
- `gcc` compiles `server.c + dnd_db.c + sessions.c + dnd_utils.c + socket_utils.c + dnd_functions.c` → `bin/server`
- `gcc` compiles `client.c + socket_utils.c` → `bin/client`
- `bin/` and `data/` directories are created if missing

---

## 2. Start the Server

```bash
./bin/server          # uses default port 8080
./bin/server 9090     # or specify a custom port
```

### What happens inside server startup:

**Step 2.1 — Load Database (`load_db`)**
- Opens `data/clients.csv`
- Reads each line, calls `parse_line()` to populate the in-memory `clients[]` array
- Each entry stores: subscriber number, active flag, DND type (NONE/GLOBAL/SELECTIVE), blocked list
- Mutex (`db_mutex`) is locked during the entire load

**Step 2.2 — Initialize Sessions (`sessions_init`)**
- Clears the in-memory `g_sessions[]` array
- Writes a fresh header to `data/sessions.csv`

**Step 2.3 — Create Listening Socket (`create_listening_socket`)**
- Creates a TCP socket (`AF_INET, SOCK_STREAM`)
- Binds to `INADDR_ANY` on the given port
- Calls `listen()` with backlog of 10
- Server is now ready to accept connections

**Step 2.4 — Accept Loop (infinite)**
- Calls `accept()` — blocks until a client connects
- On connection: extracts client IP and port via `inet_ntop` / `ntohs`
- Calls `sessions_add()` to record the new session in `g_sessions[]` and write to `sessions.csv`
- Allocates `int *client_fd` on heap, spawns a new `pthread` running `client_thread()`
- Calls `pthread_detach()` so the thread cleans itself up on exit
- Goes back to `accept()` immediately (non-blocking for other clients)

---

## 3. Connect a Client

```bash
./bin/client 127.0.0.1 8080
```

### What happens inside client startup:

**Step 3.1 — Create Socket & Connect (`create_and_connect_socket`)**
- Creates TCP socket
- Converts IP string to binary via `inet_pton`
- Calls `connect()` to establish TCP connection with server

**Step 3.2 — Enter Subscriber Number**
- User types their 10-digit subscriber number
- Client sends: `HELLO <number>\n`
- Server receives it in `handle_client()`, calls `sessions_set_subscriber()` to update the session record
- Server replies: `OK HELLO\n`

---

## 4. Client Menu Loop

The client displays a menu and waits for user input. Each choice sends a formatted command string to the server.

---

### Option 1 — Activate DND

**Client sends:**
- Global: `ACTIVATE <number> GLOBAL\n`
- Selective: `ACTIVATE <number> SELECTIVE <num1|num2>\n`

**Server processing (`handle_client` → `activate_dnd`):**

1. Tokenizes the command to extract number, type, blocked list
2. Validates the subscriber number (must be exactly 10 digits)
3. Calls `parse_type()` to convert string → `dnd_type_t` enum
4. For SELECTIVE: splits blocked list by `,` or `|` into `blocked_arr[]`
5. Calls `activate_dnd()`:
   - Locks `db_mutex`
   - Calls `find_or_create()` — finds existing entry or creates a new one in `clients[]`
   - Checks if already active → returns error if so
   - Sets `e->active = 1`, `e->type`, copies blocked numbers
   - Calls `save_db()` → writes entire `clients[]` back to `data/clients.csv`
   - Unlocks `db_mutex`
6. Updates session subscriber via `sessions_set_subscriber()`
7. Sends `OK Activated\n` back to client

**Client displays:** `SERVER: OK Activated`

---

### Option 2 — Deactivate DND

**Client sends:** `DEACTIVATE <number>\n`

**Server processing (`handle_client` → `deactivate_dnd`):**

1. Validates the number
2. Calls `deactivate_dnd()`:
   - Locks `db_mutex`
   - Calls `find_entry()` — returns error if not found
   - Checks if already inactive → returns error if so
   - Sets `e->active = 0`, `e->type = DND_NONE`, `e->blocked_count = 0`
   - Calls `save_db()` to persist changes
   - Unlocks `db_mutex`
3. Sends `OK Deactivated\n`

**Client displays:** `SERVER: OK Deactivated`

---

### Option 3 — Make Call

**Client sends:** `MAKECALL <from> <to>\n`

**Server processing (`handle_client` → `make_call`):**

1. Validates both numbers
2. Checks `from != to` (cannot call yourself)
3. Calls `make_call()`:
   - Locks `db_mutex`
   - Calls `find_entry(to)` to look up the callee
   - If callee not found or DND not active → result = `ESTABLISHED`
   - If callee has `DND_GLOBAL` → result = `BLOCKED_BY_GLOBAL`
   - If callee has `DND_SELECTIVE`:
     - Calls `is_blocked_selective()` — checks if `from` is in callee's blocked list
     - If yes → result = `BLOCKED_BY_SELECTIVE`
     - If no → result = `ESTABLISHED`
   - Unlocks `db_mutex`
4. Appends `\n` to result and sends back to client

**Client displays one of:**
- `SERVER: ESTABLISHED`
- `SERVER: BLOCKED_BY_GLOBAL`
- `SERVER: BLOCKED_BY_SELECTIVE`

---

### Option 4 — Exit

**Client sends:** `EXIT\n`

**Server processing:**
1. Sends `OK Bye\n`
2. Breaks out of the `recv` loop in `handle_client()`
3. Calls `sessions_remove(client_fd)` — clears the session from `g_sessions[]` and rewrites `sessions.csv`
4. Calls `close(client_fd)` — closes the socket
5. Thread returns `NULL` and exits (auto-cleaned due to `pthread_detach`)

**Client side:**
- Prints `Disconnected from server.`
- Calls `close(sock)` and exits

---

## 5. Concurrent Client Handling

Each client gets its own thread. The flow for N simultaneous clients:

```
Client A connects → Thread A spawned → handles A independently
Client B connects → Thread B spawned → handles B independently
Client C connects → Thread C spawned → handles C independently
```

- All threads share `clients[]` (protected by `db_mutex`)
- All threads share `g_sessions[]` (protected by `g_sess_mx`)
- No client blocks another

---

## 6. Data Persistence Flow

Every state change writes to disk immediately:

```
activate_dnd()   → save_db()   → data/clients.csv  (updated)
deactivate_dnd() → save_db()   → data/clients.csv  (updated)
sessions_add()   → save_sessions_locked() → data/sessions.csv (updated)
sessions_remove()→ save_sessions_locked() → data/sessions.csv (updated)
```

On next server restart, `load_db()` reads `clients.csv` and restores all subscriber preferences.

---

## 7. File-to-Function Responsibility Map

| File               | Responsibility                                              |
|--------------------|-------------------------------------------------------------|
| `server.c`         | Entry point, accept loop, thread spawning                   |
| `client.c`         | Menu UI, send commands, display responses                   |
| `dnd_utils.c`      | `handle_client()`, `client_thread()`, number validation     |
| `dnd_functions.c`  | `activate_dnd()`, `deactivate_dnd()`, `make_call()`         |
| `dnd_db.c`         | `load_db()`, `save_db()`, `find_entry()`, `find_or_create()`|
| `sessions.c`       | Session tracking: add, remove, update, persist to CSV       |
| `socket_utils.c`   | TCP socket creation for both client and server              |

---

## 8. Error Handling Summary

| Scenario                        | Response Sent to Client              |
|---------------------------------|--------------------------------------|
| DND already active              | `ERR Already active, deactivate first` |
| DND already inactive            | `ERR Already deactivated`            |
| Number not in DB (deactivate)   | `ERR Number not found`               |
| Invalid number format           | `ERR Invalid subscriber number`      |
| Calling yourself                | `ERR Cannot call yourself`           |
| Unknown command                 | `ERR Unknown command`                |
| Missing parameters              | `ERR Missing params`                 |
