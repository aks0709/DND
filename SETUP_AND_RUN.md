# 🚀 DND Project — Setup & Run Guide

Everything you need to go from zero to running the DND system — WSL install, VS Code setup, build, and multi-terminal simulation.

---

## 📋 Table of Contents

1. [Install WSL (Windows Subsystem for Linux)](#1-install-wsl)
2. [Set Up Linux Environment](#2-set-up-linux-environment)
3. [Set Up VS Code with WSL](#3-set-up-vs-code-with-wsl)
4. [Get the Project into WSL](#4-get-the-project-into-wsl)
5. [Build the Project](#5-build-the-project)
6. [Run — Single Terminal Test](#6-run--single-terminal-test)
7. [Run — Multi-Terminal Simulation](#7-run--multi-terminal-simulation)
8. [Run Inside VS Code](#8-run-inside-vs-code)
9. [Common Errors & Fixes](#9-common-errors--fixes)

---

## 1. Install WSL

> **Why WSL?** This project uses POSIX sockets, pthreads, and GNU Make — none of which work natively on Windows. WSL gives you a real Linux kernel on Windows.

### Step 1 — Open PowerShell as Administrator

Press `Win + S` → search **PowerShell** → right-click → **Run as Administrator**

### Step 2 — Install WSL with Ubuntu

```powershell
wsl --install
```

This installs WSL 2 + Ubuntu automatically. Wait for it to finish.

### Step 3 — Restart your PC

```powershell
Restart-Computer
```

### Step 4 — Finish Ubuntu setup

After restart, Ubuntu opens automatically. Set your username and password:

```
Enter new UNIX username: yourname
Enter new UNIX password: ****
```

> Remember this password — you'll need it for `sudo` commands.

### Step 5 — Verify WSL is working

```powershell
wsl --list --verbose
```

You should see:

```
  NAME      STATE           VERSION
* Ubuntu    Running         2
```

---

## 2. Set Up Linux Environment

Open **Ubuntu** (search it in Start Menu) and run these commands one by one.

### Update package list

```bash
sudo apt update && sudo apt upgrade -y
```

### Install GCC compiler

```bash
sudo apt install gcc -y
```

### Install Make

```bash
sudo apt install make -y
```

### Install Git

```bash
sudo apt install git -y
```

### Verify everything installed correctly

```bash
gcc --version
make --version
git --version
```

Expected output (versions may differ):

```
gcc (Ubuntu 11.4.0) 11.4.0
GNU Make 4.3
git version 2.34.1
```

---

## 3. Set Up VS Code with WSL

### Step 1 — Install VS Code on Windows

Download from: https://code.visualstudio.com/

### Step 2 — Install the WSL Extension

Open VS Code → press `Ctrl + Shift + X` → search **WSL** → click **Install**

> Publisher: Microsoft. Extension ID: `ms-vscode-remote.remote-wsl`

### Step 3 — Install the C/C++ Extension

In VS Code Extensions → search **C/C++** → click **Install**

> Publisher: Microsoft. Extension ID: `ms-vscode.cpptools`

### Step 4 — Connect VS Code to WSL

Press `Ctrl + Shift + P` → type:

```
WSL: Connect to WSL
```

Hit Enter. VS Code reopens connected to your Linux environment. You'll see **WSL: Ubuntu** in the bottom-left corner.

---

## 4. Get the Project into WSL

You have two options depending on where your project files are.

### Option A — Project is on Windows (e.g. Desktop)

Your Windows files are accessible inside WSL at `/mnt/c/`. Copy the project into your Linux home directory for best performance:

```bash
cp -r /mnt/c/Users/<your-windows-username>/OneDrive\ -\ Capgemini/Desktop/DND ~/DND
cd ~/DND
```

> Replace `<your-windows-username>` with your actual Windows username (e.g. `akum1183`).

Full command for this machine:

```bash
cp -r "/mnt/c/Users/akum1183/OneDrive - Capgemini/Desktop/DND" ~/DND
cd ~/DND
```

### Option B — Clone from GitHub

```bash
cd ~
git clone https://github.com/<your-username>/DND.git
cd DND
```

### Verify project files are there

```bash
ls
```

Expected output:

```
Makefile  README.md  HLD.md  EXECUTION_STEPS.md  src/  include/  data/  bin/
```

---

## 5. Build the Project

From inside the `DND/` directory:

```bash
make
```

Expected output:

```
gcc -Wall -Wextra -pthread -g -Iinclude -o bin/server src/server.c src/dnd_db.c src/sessions.c src/dnd_utils.c src/socket_utils.c src/dnd_functions.c -lpthread
gcc -Wall -Wextra -pthread -g -Iinclude -o bin/client src/client.c src/socket_utils.c
```

Verify binaries were created:

```bash
ls bin/
```

```
client  server
```

To rebuild from scratch:

```bash
make clean && make
```

---

## 6. Run — Single Terminal Test

This is the quickest way to verify everything works before opening multiple terminals.

### Terminal 1 — Start the server

```bash
cd ~/DND
./bin/server
```

Output:

```
[INFO] Loading database from data/clients.csv
[INFO] Loaded 21 client entries
DND Server listening on port 8080...
```

Leave this terminal open. The server is now running.

### Terminal 2 — Connect a client

Open a **new Ubuntu terminal** (or new tab) and run:

```bash
cd ~/DND
./bin/client 127.0.0.1 8080
```

Output:

```
Connected to DND Server at 127.0.0.1:8080
Enter your subscriber number: 
```

Type any 10-digit number, e.g.:

```
1234567890
```

The menu appears:

```
╔════════════════════════════════════╗
║           📋 DND MENU              ║
╠════════════════════════════════════╣
║ 1. Activate DND                    ║
║ 2. Deactivate DND                  ║
║ 3. Make Call                       ║
║ 4. Exit                            ║
╚════════════════════════════════════╝
Enter your choice:
```

---

## 7. Run — Multi-Terminal Simulation

This simulates multiple subscribers connecting at the same time — the core feature of the system.

### You need 4 terminals total:
- **Terminal 1** — Server
- **Terminal 2** — Subscriber A (caller)
- **Terminal 3** — Subscriber B (will activate DND)
- **Terminal 4** — Subscriber C (optional, third concurrent user)

---

### Terminal 1 — Start the server

```bash
cd ~/DND
./bin/server
```

---

### Terminal 2 — Connect Subscriber A

```bash
cd ~/DND
./bin/client 127.0.0.1 8080
```

Enter number: `1111111111`

---

### Terminal 3 — Connect Subscriber B

```bash
cd ~/DND
./bin/client 127.0.0.1 8080
```

Enter number: `2222222222`

Now activate **Global DND** on Subscriber B:

```
Enter your choice: 1
Enter DND mode (GLOBAL/SELECTIVE): GLOBAL
SERVER: OK Activated
```

---

### Terminal 2 — Subscriber A calls Subscriber B

Back in Terminal 2 (Subscriber A):

```
Enter your choice: 3
Enter receiver number: 2222222222
SERVER: BLOCKED_BY_GLOBAL
```

Call is blocked because B has Global DND active.

---

### Terminal 3 — Subscriber B switches to Selective DND

First deactivate:

```
Enter your choice: 2
SERVER: OK Deactivated
```

Then activate Selective DND blocking only A:

```
Enter your choice: 1
Enter DND mode (GLOBAL/SELECTIVE): SELECTIVE
Enter numbers to block (comma or | separated): 1111111111
SERVER: OK Activated
```

---

### Terminal 4 — Connect Subscriber C

```bash
cd ~/DND
./bin/client 127.0.0.1 8080
```

Enter number: `3333333333`

Subscriber C calls Subscriber B:

```
Enter your choice: 3
Enter receiver number: 2222222222
SERVER: ESTABLISHED
```

C's call goes through — only A is blocked.

---

### Terminal 2 — Subscriber A calls Subscriber B again

```
Enter your choice: 3
Enter receiver number: 2222222222
SERVER: BLOCKED_BY_SELECTIVE
```

A is blocked by Selective DND.

---

### Exit all clients cleanly

In each client terminal (2, 3, 4):

```
Enter your choice: 4
Disconnected from server.
```

Stop the server with `Ctrl + C`.

---

## 8. Run Inside VS Code

### Open the project in VS Code (WSL mode)

In your Ubuntu terminal:

```bash
cd ~/DND
code .
```

VS Code opens with the project, connected to WSL.

### Open the integrated terminal

Press `` Ctrl + ` `` to open the terminal panel inside VS Code.

The terminal is already a Linux bash shell — run all commands directly here.

### Split terminals in VS Code for multi-client simulation

1. Open terminal: `` Ctrl + ` ``
2. Split terminal: click the **Split Terminal** icon (or press `Ctrl + Shift + 5`)
3. Repeat to get 4 terminal panes side by side

Each pane is an independent Linux shell. Run the server in one and clients in the others — same commands as Section 7.

### Recommended VS Code layout for this project

```
┌─────────────────────┬─────────────────────┐
│                     │  Terminal 1: Server  │
│   Source Editor     ├─────────────────────┤
│   (src/ files)      │  Terminal 2: Client A│
│                     ├─────────────────────┤
│                     │  Terminal 3: Client B│
└─────────────────────┴─────────────────────┘
```

---

## 9. Common Errors & Fixes

### `make: command not found`

```bash
sudo apt install make -y
```

### `gcc: command not found`

```bash
sudo apt install gcc -y
```

### `bind: Address already in use`

The port 8080 is still occupied from a previous run. Kill it:

```bash
fuser -k 8080/tcp
```

Then restart the server.

### `./bin/server: No such file or directory`

You haven't built yet, or `make` failed. Run:

```bash
make clean && make
```

### `connect: Connection refused`

The server is not running. Start it in Terminal 1 first before connecting any client.

### `data/clients.csv: No such file or directory`

The `data/` directory is missing. Run:

```bash
mkdir -p data
touch data/clients.csv data/sessions.csv
```

Then restart the server.

### Permission denied on `./bin/server` or `./bin/client`

```bash
chmod +x bin/server bin/client
```

### WSL can't find project files copied from Windows

Use the exact path with spaces escaped:

```bash
cp -r "/mnt/c/Users/akum1183/OneDrive - Capgemini/Desktop/DND" ~/DND
```

---

## Quick Reference Card

```
# One-time setup
sudo apt update && sudo apt install gcc make git -y

# Build
cd ~/DND && make

# Run server (Terminal 1)
./bin/server

# Run server on custom port (Terminal 1)
./bin/server 9090

# Connect client (Terminal 2, 3, 4...)
./bin/client 127.0.0.1 8080

# Clean build
make clean && make

# Kill port if stuck
fuser -k 8080/tcp

# Open in VS Code from WSL
code .
```
