# P2P File Sharing System

A **Peer-to-Peer (P2P) File Sharing System** developed as a Computer Networks mini project using **C, Socket Programming, SHA-256, and POSIX Threads (pthread)**.

The system allows multiple peers to discover each other, exchange file pieces, transfer data concurrently, verify file integrity, and recover from peer failures during transmission.

## Features

* Peer-to-peer communication
* Peer discovery and connection management
* File splitting into smaller pieces
* SHA-256 based file integrity verification
* Concurrent file transfer using `pthread`
* Simultaneous upload and download
* Peer failure recovery
* Timeout handling
* Corrupted piece detection and recovery
* Peer contribution tracking
* Support for large file offsets using 64-bit values

## Tech Stack

* **Language:** C
* **Networking:** TCP/IP Socket Programming
* **Concurrency:** POSIX Threads (`pthread`)
* **Integrity Verification:** SHA-256
* **Build:** Make

## Project Structure

```text
P2P-File-Sharing/
│
├── src/
│   ├── network/
│   ├── storage/
│   ├── transfer/
│   └── recovery/
│
├── tests/
├── docs/
├── Makefile
├── README.md
└── .gitignore
```

## How It Works

```text
        Peer Discovery
              ↓
       Connect to Peers
              ↓
       Exchange Piece Info
              ↓
        Request Pieces
              ↓
     Concurrent Transfer
              ↓
       SHA-256 Verification
              ↓
       Reconstruct File
```

If a peer becomes unavailable or a piece is corrupted, the system attempts to recover the missing piece from another available peer.

## Team

| Member      | Contribution                                      |
| ----------- | ------------------------------------------------- |
| **Shivam**  | Peer Discovery & Network Protocol                 |
| **Ayush**   | File Splitting, Piece Management & SHA-256        |
| **Sagar**   | Concurrent Upload/Download & pthreads             |
| **Aaditya** | Failure Recovery, Fairness, Integration & Testing |

## Project Goal

The goal of this project is to demonstrate the practical implementation of **P2P networking, concurrent communication, file transfer, data integrity, and fault tolerance** using C.

## Status

🚧 **Under Development**

---

### Computer Networks Mini Project

**P2P File Sharing System**
Built with C • Socket Programming • pthread • SHA-256

