# cpp-UDP-server

A lightweight UDP server prototype written in C++.

It accepts UDP packets, tracks connected clients, relays packets to active clients, and renders a live terminal UI using `ncurses`.

## Current Features

- UDP server bound to `0.0.0.0:7777`
- Non-blocking socket receive loop
- Per-client tracking by IP and port
- Client timeout cleanup (inactive clients removed after 10 seconds)
- Basic packet flag handling:
  - `1` = new client / ID assignment flow
  - `2` = disconnect client
- Broadcast-style forwarding of packet data to connected clients
- Real-time status UI in terminal via `ncurses`

## Project Structure

- `server.cpp`: main loop, socket setup, timing/thread logic, and terminal UI
- `serverHelper.h`: networking helpers, client list/state, packet handling

## Build

This project uses POSIX socket headers and `ncurses`, so it is intended for Linux or WSL.

Example build command:

```bash
g++ server.cpp -lncurses -o server
```

## Run

```bash
./server
```

The server starts on UDP port `7777` and displays connected clients and recent packet snapshots.

## Packet Notes

Current packet processing expects at least 15 bytes for broadcast handling.

- Byte `0`: packet flag
- Bytes `1-2`: 16-bit ID (little-endian) for ID-related flow

## Planned: Server Rewind

Planned feature: **Server Rewind**

Goal:

- Keep a rolling history of recent server state and/or packet events
- Allow operator-triggered rewind to a previous point in history

Planned behavior (high level):

- Record snapshots on each tick or event (join/leave/message)
- Use a bounded ring buffer to control memory use
- Add controls/API to step backward through snapshots
- Reconcile client state after rewind and re-broadcast authoritative state

Possible implementation approach:

- Add a `ServerSnapshot` model containing:
  - client list and metadata
  - recent packet/event log
  - timestamp or tick index
- Store snapshots in a fixed-capacity circular buffer
- Introduce rewind commands and safety rules (pause live updates while rewinding)

## Known Limitations

- Global mutable state shared across translation units
- Some packet-size assumptions are fixed in code
- Minimal protocol validation and error handling
- Linux/WSL-focused due to socket + ncurses usage

## License

No license file is currently included.
