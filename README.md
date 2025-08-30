# Multithreaded Chat Server & Client (C, POSIX Sockets, Threads)

## Overview
This project implements a simple **TCP chat server** and **client** in C using POSIX sockets and threads.  
- **Server** handles multiple clients concurrently, supports message broadcasting and private messaging via `@username`.  
- **Client** connects to the server, uses separate read/write threads, and includes auto-reconnect logic.  

## Build
```bash
gcc server.c -o server -lpthread
gcc client.c -o client -lpthread
```

## Run
Start the server:
```bash
./server
```

Launch one or more clients:
```bash
./client
```

### Usage
- When you connect to the server, you must first enter your **username**:
- Type any message → broadcast to all clients.  
- Use `@username message` → send private message.  
- Type `exit` → disconnect client.  

## Features
- Multi-client support (up to 10).  
- Thread-safe broadcasting with mutex.  
- Private messaging via username.  
- Client auto-reconnect on connection loss.  
- Graceful signal handling (SIGINT, SIGTERM, SIGPIPE).  

---

## Notes
This code serves as a functional prototype for a threaded chat application.  
For production-grade use, additional improvements such as proper resource cleanup, authentication, username uniqueness, and robust error handling are strongly recommended.
