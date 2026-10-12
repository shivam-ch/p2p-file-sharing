 <div align="center">

# Peer-to-Peer File Sharing Network

### A Small-Scale BitTorrent-Inspired File Sharing System

A distributed file-sharing project built in C, demonstrating concurrent transfers, peer discovery, data integrity, and failure recovery.

<p>
  <img src="https://img.shields.io/badge/Language-C-00599C?style=flat-square&logo=c&logoColor=white" alt="Language: C">
  <img src="https://img.shields.io/badge/Networking-TCP-2563EB?style=flat-square" alt="TCP Networking">
  <img src="https://img.shields.io/badge/Concurrency-POSIX_Threads-16A34A?style=flat-square" alt="POSIX Threads">
  <img src="https://img.shields.io/badge/Integrity-SHA--256-7C3AED?style=flat-square" alt="SHA-256">
  <img src="https://img.shields.io/badge/Build-GNU_Make-DC2626?style=flat-square" alt="GNU Make">
</p>

**C · TCP Sockets · POSIX Threads · OpenSSL · GNU Make**

⭐ **If you find this project interesting, consider giving it a star!**

[Overview](#overview) · [Features](#features) · [Architecture](#architecture) · [Getting Started](#getting-started) · [Testing](#testing) · [Team](#team)

</div>

---

## Overview

This project implements a small-scale **peer-to-peer (P2P) file-sharing system inspired by BitTorrent**. It allows peers to exchange file pieces directly over TCP rather than relying on a single server to supply every part of a file.

Files are divided into independently transferable pieces. Multiple peers can contribute pieces to a download, allowing concurrent transfers, integrity verification, and recovery from unsuccessful transfers.

### Key Objectives

- Build peer-to-peer communication using TCP sockets.
- Split files into pieces for independent transfer.
- Maintain peer information and piece availability.
- Support dynamic file-manifest updates between known peers.
- Download pieces concurrently using POSIX threads.
- Verify piece integrity using SHA-256.
- Recover from peer failures, corrupted data, and mid-transfer disconnections.
- Reconstruct downloaded files and track peer contributions.

## Features

| Category | Capabilities |
|---|---|
| **Networking** | TCP connections, peer listeners, peer discovery, peer-list exchange |
| **File Distribution** | File splitting, piece metadata, manifest exchange |
| **Dynamic Updates** | Manifest-update notifications and availability refresh |
| **Scheduling** | Peer-aware selection and rarest-first piece ordering |
| **Concurrency** | Multiple workers and concurrent piece downloads |
| **Integrity** | SHA-256 verification and corrupted-piece rejection |
| **Recovery** | Connection failure handling, timeouts, retries, dropout recovery |
| **Storage** | Piece storage, file I/O, file reconstruction |
| **Fairness** | Upload/download contribution tracking and history persistence |
| **Testing** | Unit tests and multi-peer integration tests |

## Technology Stack

| Technology | Purpose |
|---|---|
| C | Core application and systems programming |
| TCP / POSIX Sockets | Peer communication |
| POSIX Threads | Concurrent operations |
| OpenSSL | SHA-256 hashing |
| GCC | Compilation |
| GNU Make | Build and test automation |
| Linux / Ubuntu / WSL | Development environment |

---

## Architecture

The codebase separates application control, networking, storage, transfers, and recovery into modules.

```text
p2p-file-sharing/
├── src/
│   ├── app/
│   │   ├── main.c
│   │   ├── p2p.c
│   │   └── p2p.h
│   ├── common/
│   │   ├── protocol.c
│   │   ├── protocol.h
│   │   └── types.h
│   ├── network/
│   │   ├── peer.c
│   │   ├── peer.h
│   │   ├── discovery.c
│   │   └── discovery.h
│   ├── storage/
│   │   ├── piece.c
│   │   ├── piece.h
│   │   ├── sha256.c
│   │   ├── sha256.h
│   │   ├── file_splitter.c
│   │   ├── file_splitter.h
│   │   ├── availability.c
│   │   ├── availability.h
│   │   ├── file_io.c
│   │   ├── file_io.h
│   │   ├── piece_scheduler.c
│   │   └── piece_scheduler.h
│   ├── transfer/
│   │   ├── transfer.c
│   │   └── transfer.h
│   └── recovery/
│       ├── failure.c
│       ├── failure.h
│       ├── timeout.c
│       ├── timeout.h
│       ├── fairness.c
│       ├── fairness.h
│       ├── recovery.c
│       ├── recovery.h
│       ├── recovery_transfer.c
│       └── recovery_transfer.h
├── tests/
├── Makefile
├── README.md
└── .gitignore
```

### Module Responsibilities

| Module | Responsibility |
|---|---|
| `app/` | Application startup, interactive CLI, P2P coordination |
| `common/` | Protocol definitions and shared data types |
| `network/` | TCP connections, listeners, peer discovery |
| `storage/` | File splitting, piece metadata, hashes, availability, scheduling |
| `transfer/` | Piece and block transfer operations |
| `recovery/` | Failure handling, timeouts, recovery, fairness tracking |
| `tests/` | Unit and integration testing |

---

## How It Works

<div align="center">

```text
            ┌─────────────────────┐
            │     Original File    │
            └──────────┬──────────┘
                       │
                       ▼
            ┌─────────────────────┐
            │    Split into       │
            │      Pieces         │
            └──────────┬──────────┘
                       │
             ┌─────────┼─────────┐
             ▼         ▼         ▼
          Piece 0   Piece 1   Piece 2
             │         │         │
             ▼         ▼         ▼
           Peer 1    Peer 2    Peer 3
             └─────────┼─────────┘
                       │
                       ▼
            Concurrent Downloads
                       │
                       ▼
              SHA-256 Verification
                       │
                       ▼
             File Reconstruction
                       │
                       ▼
                Complete File
```

</div>

### Download Workflow

1. Peers start their TCP listeners and exchange peer information.
2. A peer shares a file and generates piece metadata and hashes.
3. The file manifest is sent to known peers.
4. Peers use availability information to identify potential piece sources.
5. Multiple workers request and transfer pieces concurrently.
6. Each received piece is checked against its expected SHA-256 hash.
7. Failed or corrupted transfers can be retried using another peer.
8. The pieces are reconstructed into the original file and verified.

---

## File Splitting and Manifests

Each file is represented as a collection of pieces that can be transferred independently.

Piece metadata includes:

- Piece ID
- File offset
- Piece size
- Expected SHA-256 hash
- Availability information

A manifest communicates piece metadata to other peers without requiring them to download the complete file first.

### Automatic Manifest Updates

The application supports notifying known peers when a file is shared after peer connections have already been established.

The announcing peer uses `p2p_announce_manifest()` to:

1. Take a snapshot of its known peer list.
2. Connect to eligible peers.
3. Send a `MSG_MANIFEST_UPDATE` notification containing its peer ID.
4. Send the file manifest.
5. Report notification failures when they occur.

The receiving peer processes the update through its TCP listener and refreshes the announcing peer's availability mappings.

**Why it matters:** Peers can receive newly shared file information without disconnecting and reconnecting.

### Dynamic Availability Refresh

When a replacement manifest is processed, the receiver removes the announcing peer's old piece mappings and adds the mappings from the new manifest.

This helps prevent stale availability information from remaining in the table while preserving mappings for other peers.

> **Implementation note:** The current design uses the existing peer and manifest model. It is intended for an academic P2P system, not as a secure or fully general-purpose BitTorrent implementation.

---

## Peer Discovery and Availability

Each peer is represented by information such as:

```text
Peer ID
IP Address
Listening Port
```

The networking modules support:

- Starting a TCP listener.
- Accepting incoming connections.
- Connecting to known peers.
- Exchanging peer lists.
- Maintaining and updating peer records.
- Validating peer information.
- Supporting dynamically sized peer collections.

The availability table maps piece IDs to peers that advertise those pieces.

Example:

```text
Piece 0 -> Peer 1, Peer 2
Piece 1 -> Peer 3
Piece 2 -> Peer 2
```

When a selected peer fails, recovery logic can attempt another peer advertising the required piece.

## Rarest-First Piece Scheduling

The piece scheduler supports rarest-first ordering, which prioritizes pieces available from fewer peers.

Example:

```text
Piece 0 -> 4 available peers
Piece 1 -> 1 available peer
Piece 2 -> 3 available peers

Rarest-first priority:
1. Piece 1
2. Piece 2
3. Piece 0
```

Scheduler tests cover rarest-first ordering, stable tie ordering, unavailable pieces, empty inputs, and invalid arguments.

Rarest-first scheduling is a piece-selection strategy; it does not implement the complete BitTorrent choking and unchoking algorithm.

---

## Concurrent Transfers

POSIX threads allow multiple piece downloads to run concurrently.

```text
Worker 1 -> Piece 0 -> Peer 1
Worker 2 -> Piece 1 -> Peer 2
Worker 3 -> Piece 2 -> Peer 3
Worker 4 -> Piece 3 -> Peer 1
```

The integration tests cover multi-peer downloads and four-worker concurrent transfers.

Concurrent transfers can improve resource utilization, although actual performance depends on peer availability, file size, and network conditions.

## Data Integrity and Recovery

### SHA-256 Verification

Every piece has an expected SHA-256 hash. The receiver calculates the hash of the received piece and compares it with the expected value.

```text
Received Piece
      │
      ▼
Calculate SHA-256
      │
      ▼
Compare Hashes
      │
   ┌──┴───┐
   ▼      ▼
 Match  Mismatch
   │      │
   ▼      ▼
 Accept  Reject
          │
          ▼
    Try Recovery
```

A piece that fails verification can be rejected, and another available peer can be tried.

SHA-256 verifies data against the expected hash; it does not authenticate peers or protect against every form of malicious activity.

### Failure Recovery

The recovery components address scenarios including:

- Connection failures.
- Peer unavailability.
- Transfer timeouts.
- Failed piece transfers.
- Corrupted piece data.
- Mid-transfer disconnections.

### Mid-Transfer Dropout

The `test_mid_transfer_dropout` integration test simulates a peer disconnecting during an active transfer. It checks recovery from another peer, file reconstruction, and final SHA-256 verification.

### Corruption Recovery

The `test_corruption_recovery` test simulates a peer supplying corrupted data. The receiver detects the hash mismatch, rejects the piece, and attempts recovery from another peer.

## Fairness and Contribution Tracking

The fairness module tracks peer upload and download contributions and supports contribution-ratio calculations.

Example:

```text
Peer 1 -> Uploaded: 3 | Downloaded: 1
Peer 2 -> Uploaded: 1 | Downloaded: 3
Peer 3 -> Uploaded: 0 | Downloaded: 1
```

The project includes tests for contribution tracking, ratio calculations, and fairness-history persistence.

**Scope:** Contribution tracking is not a complete BitTorrent tit-for-tat choking and unchoking implementation.

---

## Getting Started

### Requirements

- Linux, Ubuntu, or WSL Ubuntu
- GCC
- GNU Make
- OpenSSL development libraries
- POSIX sockets and threads

### 1. Install Dependencies

```bash
sudo apt update
sudo apt install build-essential libssl-dev
```

### 2. Clone the Repository

```bash
git clone https://github.com/shivam-ch/p2p-file-sharing.git
cd p2p-file-sharing
```

### 3. Build the Project

```bash
make
```

This builds the `p2p` executable when compilation succeeds.

### 4. Run the Tests

```bash
make test
```

For a clean build followed by testing:

```bash
make clean
make
make test
```

### 5. Start Peers

Run each peer in a separate terminal:

**Terminal 1**

```bash
./p2p 1 5001
```

**Terminal 2**

```bash
./p2p 2 5002
```

**Terminal 3**

```bash
./p2p 3 5003
```

Use distinct peer IDs and available ports. For peers running on different machines, use reachable IP addresses and ensure the required ports are accessible.

### Interactive CLI

After startup, the application provides a `p2p>` prompt.

| Command | Purpose |
|---|---|
| `help` | Show supported commands |
| `status` | Display local node status |
| `connect` | Connect to another peer |
| `peers` | List known peers |
| `share` | Share a local file |
| `download` | Download a shared file |
| `fairness` | Display contribution information |
| `exit` | Exit the application |

Use `help` in the running application for the exact command syntax and arguments.

---

## Testing

The project contains unit and integration tests for networking, storage, protocol handling, scheduling, concurrent transfers, and recovery.

Run the complete test suite:

```bash
make test
```

### Test Coverage

| Test | Coverage |
|---|---|
| `test_protocol` | Protocol definitions and message handling |
| `test_file_io` | File input/output |
| `test_piece` | Piece storage and hash verification |
| `test_piece_block` | Piece-block serialization |
| `test_transfer` | Concurrent transfer logic |
| `test_recovery` | Peer failure recovery |
| `test_timeout` | Timeout handling |
| `test_fairness` | Contribution tracking and history |
| `test_availability` | Piece availability and peer selection |
| `test_piece_scheduler` | Rarest-first scheduling |
| `test_tcp_transfer` | TCP piece transfer |
| `test_tcp_concurrent_transfer` | Concurrent TCP transfers |
| `test_peer_aware_download` | Availability-based downloading |
| `test_recovery_transfer` | Recovery from unavailable peers |
| `test_corruption_recovery` | Corrupted-piece recovery |
| `test_three_peer_download` | Three-peer concurrent download |
| `test_mid_transfer_dropout` | Mid-transfer dropout recovery |
| `test_discovery` | Peer discovery and validation |
| `test_discovery_serialization` | Peer-list serialization |

A successful run ends with:

```text
========================================
ALL TESTS PASSED
========================================
```

The automated suite validates its tested scenarios; it does not establish correctness under every possible network condition or malicious-peer scenario.

---

## Demo Scenarios

### Three-Peer Concurrent Download

```bash
./tests/test_three_peer_download
```

Demonstrates concurrent piece downloads from three peers, file reconstruction, and final SHA-256 verification.

### Mid-Transfer Dropout Recovery

```bash
./tests/test_mid_transfer_dropout
```

Demonstrates recovery after a peer disconnects during a piece transfer.

### Corrupted-Piece Recovery

```bash
./tests/test_corruption_recovery
```

Demonstrates hash-mismatch detection, rejection of corrupted data, and recovery from an alternate peer.

### Automatic Manifest Updates

For a manual demonstration:

1. Start three peers.
2. Connect the peers so they know about each other.
3. Share a file after connections have been established.
4. Observe the manifest notification messages.
5. Check that the receiving peers report the updated manifest.
6. Download the shared file and verify the result.

This demonstrates the intended workflow for notifying already-connected peers about newly shared files.

---

## Important Commands

| Command | Description |
|---|---|
| `make` | Build the application |
| `make clean` | Remove generated build and test binaries |
| `make test` | Run the complete test suite |
| `./p2p <peer_id> <port>` | Start a peer |
| `./tests/test_three_peer_download` | Run the three-peer demo |
| `./tests/test_mid_transfer_dropout` | Run the dropout demo |
| `./tests/test_corruption_recovery` | Run the corruption demo |

---

## Future Improvements

Potential extensions include:

- Scalable discovery using a DHT or tracker.
- Stronger peer authentication and message validation.
- More comprehensive manifest synchronization.
- Bitfield-based availability exchange.
- Improved connection management and retry backoff.
- Bandwidth measurement and optimization.
- Larger-scale distributed testing.
- A complete BitTorrent-style choking and unchoking mechanism.
- Better support for peers sharing different files or file versions.
- Structured logging and more detailed transfer statistics.

These are possible future directions, not claims that the functionality is already implemented.

## Project Scope and Limitations

This is a **small-scale academic implementation inspired by BitTorrent**, designed to demonstrate practical concepts in computer networks and systems programming.

The project focuses on peer-to-peer TCP communication, piece-based distribution, concurrent transfers, peer discovery, manifest updates, integrity verification, recovery, and contribution tracking.

It is not a complete production implementation of BitTorrent. It does not claim to provide a production-scale DHT, complete tracker infrastructure, cryptographic peer identity, or the full choking and unchoking system.

The manifest-update mechanism uses the existing peer and availability model and should not be treated as a secure file-sharing service for untrusted networks.

---

## Team

<div align="center">

**Peer-to-Peer File Sharing Network**

| Contributors |
|---|
| Shivam Kumar Chaurasia |
| Ayush Pathak |
| Sagar Raj Sharma |
| Aaditya Thakur |

</div>

## Academic Project

Developed to demonstrate practical concepts in:

- Computer Networks
- Peer-to-Peer Systems
- TCP Socket Programming
- Concurrent Programming
- File Systems
- Data Integrity
- Failure Recovery
- Network Protocol Design

<div align="center">

---

### Built with C and TCP

If you find this project useful for learning about networking, distributed systems, or concurrent programming, consider giving the repository a star.

**[View the Repository](https://github.com/shivam-ch/p2p-file-sharing)**

</div>