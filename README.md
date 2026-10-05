<div align="center">

# 🌐 Peer-to-Peer File Sharing Network

### A Small-Scale BitTorrent-Inspired P2P File Sharing System

**Built in C • TCP Sockets • POSIX Threads • SHA-256 • OpenSSL**

<p>
  <img src="https://img.shields.io/badge/Language-C-blue?style=for-the-badge&logo=c" alt="C">
  <img src="https://img.shields.io/badge/Networking-TCP-orange?style=for-the-badge" alt="TCP">
  <img src="https://img.shields.io/badge/Concurrency-POSIX%20Threads-green?style=for-the-badge" alt="Pthreads">
  <img src="https://img.shields.io/badge/Integrity-SHA--256-purple?style=for-the-badge" alt="SHA-256">
  <img src="https://img.shields.io/badge/Build-Make-red?style=for-the-badge" alt="Make">
</p>

<p>
  <a href="#-overview">Overview</a> •
  <a href="#-features">Features</a> •
  <a href="#-architecture">Architecture</a> •
  <a href="#-getting-started">Getting Started</a> •
  <a href="#-testing">Testing</a> •
  <a href="#-demo-scenarios">Demo</a>
</p>

⭐ **If you find this project interesting, consider giving it a star!**

</div>

---

# 📌 Overview

This project is a **small-scale Peer-to-Peer (P2P) file-sharing system inspired by BitTorrent**.

Instead of downloading an entire file from a single server, the file is divided into smaller pieces. Different peers can provide different pieces, allowing the downloader to retrieve multiple pieces **concurrently**.

The system also handles:

- Peer communication
- Piece availability
- Concurrent transfers
- SHA-256 integrity verification
- Corrupted pieces
- Peer failures
- Mid-transfer peer disconnections
- File reconstruction
- Upload/download contribution tracking

The project is implemented from scratch using **C, TCP sockets, POSIX threads, and OpenSSL**.

---

# ✨ Features

| Feature | Status |
|---|:---:|
| 🌐 TCP Peer-to-Peer Communication | ✅ |
| 📦 File Splitting into Pieces | ✅ |
| 🔄 Concurrent Piece Downloads | ✅ |
| 👥 Multi-Peer File Sharing | ✅ |
| 📋 Peer Information Management | ✅ |
| 🧩 Piece Availability Tracking | ✅ |
| 🔐 SHA-256 Integrity Verification | ✅ |
| 🚨 Corrupted Piece Detection | ✅ |
| ♻️ Peer Failure Recovery | ✅ |
| 💥 Mid-Transfer Dropout Recovery | ✅ |
| ⏱️ Transfer Timeout Handling | ✅ |
| 🧱 File Reconstruction | ✅ |
| ⚖️ Upload/Download Contribution Tracking | ✅ |
| 🧪 Automated Unit & Integration Tests | ✅ |

---

# 🛠️ Technology Stack

<div align="center">

| Technology | Purpose |
|---|---|
| **C** | Core implementation |
| **TCP / POSIX Sockets** | Peer communication |
| **POSIX Threads** | Concurrent transfers |
| **OpenSSL** | SHA-256 hashing |
| **GNU Make** | Build and test automation |
| **Linux / WSL Ubuntu** | Development environment |

</div>

---

# 🏗️ Architecture

The project is organized into separate modules for networking, storage, transfer, and recovery.

```text
p2p-file-sharing/
│
├── src/
│   ├── common/
│   │   ├── protocol.c
│   │   ├── protocol.h
│   │   └── types.h
│   │
│   ├── network/
│   │   ├── peer.c
│   │   ├── peer.h
│   │   ├── discovery.c
│   │   └── discovery.h
│   │
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
│   │   └── file_io.h
│   │
│   ├── transfer/
│   │   ├── transfer.c
│   │   └── transfer.h
│   │
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
│
├── tests/
│   ├── test_protocol.c
│   ├── test_file_io.c
│   ├── test_piece.c
│   ├── test_piece_block.c
│   ├── test_transfer.c
│   ├── test_recovery.c
│   ├── test_timeout.c
│   ├── test_fairness.c
│   ├── test_availability.c
│   ├── test_tcp_transfer.c
│   ├── test_tcp_concurrent_transfer.c
│   ├── test_peer_aware_download.c
│   ├── test_recovery_transfer.c
│   ├── test_corruption_recovery.c
│   ├── test_three_peer_download.c
│   └── test_mid_transfer_dropout.c
│
├── Makefile
├── README.md
└── .gitignore
```

---

# 🔄 How It Works

```text
                 ┌──────────────────┐
                 │   Original File  │
                 └────────┬─────────┘
                          │
                          ▼
                 ┌──────────────────┐
                 │ Split into       │
                 │     Pieces       │
                 └────────┬─────────┘
                          │
             ┌────────────┼────────────┐
             ▼            ▼            ▼
          Piece 0      Piece 1      Piece 2
             │            │            │
             ▼            ▼            ▼
          Peer 1        Peer 2        Peer 3
             │            │            │
             └────────────┼────────────┘
                          │
                          ▼
                Concurrent Download
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

---

# 📦 File Splitting

The original file is divided into fixed-size pieces.

Each piece contains information such as:

- Piece ID
- File offset
- Piece size
- Expected SHA-256 hash
- Availability/status information

Example:

```text
Original File
│
├── Piece 0
├── Piece 1
├── Piece 2
└── Piece 3
```

The pieces can be downloaded independently and later combined to reconstruct the original file.

---

# 👥 Peer Management

Each peer is represented using information such as:

```text
Peer ID
IP Address
Port
```

The networking module provides functionality for:

- Starting a TCP listener
- Accepting peer connections
- Connecting to another peer
- Maintaining peer information

The communication protocol includes messages for:

- `HELLO`
- Peer lists
- Piece requests
- Piece responses
- Piece blocks

---

# 🧩 Piece Availability

The availability system tracks which peers have particular pieces.

Example:

```text
Piece 0 → Peer 1, Peer 2
Piece 1 → Peer 3
Piece 2 → Peer 2
```

When a piece is requested, the downloader can use this information to find a peer capable of supplying it.

If the selected peer fails, another peer advertising the same piece can be attempted.

---

# ⚡ Concurrent Transfers

Multiple pieces can be downloaded simultaneously using POSIX threads.

Example:

```text
Thread 1 ──► Download Piece 0 ──► Peer 1
Thread 2 ──► Download Piece 1 ──► Peer 2
Thread 3 ──► Download Piece 2 ──► Peer 3
```

This allows the downloader to obtain different pieces from multiple peers at the same time.

---

# 🔐 SHA-256 Integrity Verification

Every piece has an expected SHA-256 hash.

After receiving a piece:

```text
             Received Piece
                    │
                    ▼
            Calculate SHA-256
                    │
                    ▼
          Compare with expected
                    │
             ┌──────┴──────┐
             ▼             ▼
           Match       Mismatch
             │             │
             ▼             ▼
           Accept        Reject
                           │
                           ▼
                    Recover from
                    another peer
```

This ensures corrupted or modified pieces are not used during reconstruction.

The integration tests also verify the final reconstructed file.

---

# ♻️ Failure Recovery

If a peer becomes unavailable, the downloader can attempt another peer that has the requested piece.

```text
Downloader
     │
     ▼
  Peer 1
     │
     X Connection Failed
     │
     ▼
  Peer 2
     │
     ▼
Piece Recovered
```

The recovery mechanism handles:

- Connection failures
- Peer failures
- Timeouts
- Failed piece transfers
- Mid-transfer disconnections

---

# 💥 Mid-Transfer Dropout

One of the main integration tests demonstrates a peer disconnecting **during an active piece transfer**.

Example:

```text
Piece 0 → Peer 1, Peer 2

Downloader
     │
     ├──────── Request Piece 0
     │
     ▼
  Peer 1
     │
     ├──────── First Block
     │
     X──────── Connection Drops
     │
     ▼
  Recovery
     │
     ▼
  Peer 2
     │
     └──────── Piece 0
```

The incomplete piece is discarded and Piece 0 is recovered from Peer 2.

The complete file is then reconstructed and verified.

---

# 🚨 Corruption Recovery

The system also tests a peer sending corrupted data.

```text
Peer 1
   │
   │ Corrupted Piece
   ▼
Downloader
   │
   │ SHA-256 mismatch
   ▼
Reject Piece
   │
   ▼
Peer 2
   │
   │ Correct Piece
   ▼
Verify
   │
   ▼
Accept
```

This prevents corrupted data from reaching the final reconstructed file.

---

# ⚖️ Fairness

The project tracks peer upload/download contributions.

Example:

```text
Peer 1 → Uploaded: 3 | Downloaded: 1
Peer 2 → Uploaded: 1 | Downloaded: 3
```

This provides a foundation for identifying peer contribution and addressing free-riding.

> **Note:** The current implementation provides contribution tracking rather than a complete BitTorrent tit-for-tat choking/unchoking algorithm.

---

# 🚀 Getting Started

## Requirements

Ubuntu / WSL Ubuntu with:

- GCC
- GNU Make
- OpenSSL development libraries
- POSIX sockets
- POSIX threads

Install dependencies:

```bash
sudo apt update
sudo apt install build-essential libssl-dev
```

Verify:

```bash
gcc --version
make --version
```

---

# 📥 Clone the Repository

```bash
git clone https://github.com/shivam-ch/p2p-file-sharing
cd p2p-file-sharing
```

---

# 🔨 Build

Build the project:

```bash
make
```

Clean generated files:

```bash
make clean
```

---

# 🧪 Testing

Run the complete test suite:

```bash
make clean
make test
```

The suite includes:

| Test | Purpose |
|---|---|
| `test_protocol` | Protocol definitions |
| `test_file_io` | File I/O |
| `test_piece` | Piece storage & verification |
| `test_piece_block` | Piece block serialization |
| `test_transfer` | Concurrent transfers |
| `test_recovery` | Peer failure recovery |
| `test_timeout` | Timeout handling |
| `test_fairness` | Contribution tracking |
| `test_availability` | Piece availability |
| `test_tcp_transfer` | TCP P2P transfer |
| `test_tcp_concurrent_transfer` | Concurrent TCP transfers |
| `test_peer_aware_download` | Availability-based downloading |
| `test_recovery_transfer` | Recovery from unavailable peer |
| `test_corruption_recovery` | Corrupted piece recovery |
| `test_three_peer_download` | Three-peer full-file download |
| `test_mid_transfer_dropout` | Mid-transfer dropout recovery |

Successful execution ends with:

```text
========================================
ALL TESTS PASSED
========================================
```

---

# 🎬 Demo Scenarios

## 1️⃣ Three-Peer Concurrent Download

Run:

```bash
./tests/test_three_peer_download
```

Demonstrates:

```text
Piece 0 → Peer 1
Piece 1 → Peer 2
Piece 2 → Peer 3
        ↓
Concurrent Downloads
        ↓
File Reconstruction
        ↓
SHA-256 Verification
```

---

## 2️⃣ Peer Dropout During Transfer

Run:

```bash
./tests/test_mid_transfer_dropout
```

Demonstrates:

```text
Peer 1
  │
  ├── Piece 0
  │
  X── Drops during transfer
  │
  ▼
Peer 2
  │
  └── Recovers Piece 0
```

Expected result:

```text
3-peer mid-transfer dropout recovery test passed.
Peer 1 dropped during Piece 0 transfer.
Piece 0 was recovered from Peer 2.
All 3 pieces were downloaded concurrently.
Full file reconstruction passed.
Final SHA-256 verification passed.
```

---

## 3️⃣ Corrupted Piece Recovery

Run:

```bash
./tests/test_corruption_recovery
```

Demonstrates:

```text
Corrupted Piece
      ↓
SHA-256 Failure
      ↓
Piece Rejected
      ↓
Alternative Peer
      ↓
Correct Piece
      ↓
Verification Passed
```

---

# 📊 Test Coverage

The project contains both **unit tests** and **integration tests** covering:

- Protocol handling
- File operations
- Piece management
- Piece serialization
- TCP communication
- Concurrent transfers
- Peer availability
- Peer-aware downloading
- Failure recovery
- Timeouts
- Corruption detection
- Mid-transfer dropout
- SHA-256 verification
- File reconstruction
- Contribution tracking

Run everything with:

```bash
make test
```

---

# 📁 Important Commands

| Command | Description |
|---|---|
| `make` | Build project |
| `make test` | Run complete test suite |
| `make clean` | Remove generated files |
| `./tests/test_three_peer_download` | Run 3-peer demo |
| `./tests/test_mid_transfer_dropout` | Run dropout demo |
| `./tests/test_corruption_recovery` | Run corruption recovery demo |

---

# 🔮 Future Improvements

Possible improvements include:

- 🔍 More scalable peer discovery
- 📡 Dynamic peer management
- 🧩 Bitfield-based availability exchange
- 🎯 Rarest-piece-first selection
- ⚖️ More advanced choking/unchoking
- 🌐 Larger peer networks
- 🚀 Improved connection management
- 📈 Better bandwidth management
- 🧪 Large-scale distributed testing

---

# ⚠️ Project Scope

This is a **small-scale academic implementation inspired by BitTorrent**.

It focuses on demonstrating:

- Peer-to-peer communication
- Piece-based file distribution
- Concurrent downloads
- Piece availability
- SHA-256 integrity verification
- Failure recovery
- Peer dropout recovery
- Contribution tracking

It is **not intended to be a complete production implementation of the BitTorrent protocol**.

The current project does not implement a full production-scale DHT, tracker infrastructure, or complete BitTorrent tit-for-tat choking/unchoking system.

---

# 👨‍💻 Team

### Peer-to-Peer File Sharing Network

| Contributors |
|---|
| **Shivam Kumar Chaurasia** |
| **Ayush Pathak** |
| **Sagar Raj Sharma** |
| **Aaditya Thakur** |

---

# 🎓 Academic Project

Developed as an academic project to demonstrate practical concepts in:

- Computer Networks
- Peer-to-Peer Systems
- TCP Socket Programming
- Concurrent Programming
- File Systems
- Data Integrity
- Failure Recovery

---

<div align="center">

## ⭐ Like the Project?

If this project helped you understand P2P networking, file sharing, or concurrent TCP programming:

### ⭐ Give this repository a star!

It helps support the project and makes it easier for others to discover it.

---

**Built with C ❤️ and TCP**

</div>
