# 🌐 Peer-to-Peer File Sharing Network

### A Small-Scale BitTorrent-Inspired P2P File Sharing System

**Built with C • TCP Sockets • POSIX Threads • SHA-256 • OpenSSL**

<p align="center">
  <img src="https://img.shields.io/badge/Language-C-blue?style=for-the-badge&logo=c" alt="C">
  <img src="https://img.shields.io/badge/Networking-TCP-orange?style=for-the-badge" alt="TCP">
  <img src="https://img.shields.io/badge/Concurrency-POSIX%20Threads-green?style=for-the-badge" alt="POSIX Threads">
  <img src="https://img.shields.io/badge/Integrity-SHA--256-purple?style=for-the-badge" alt="SHA-256">
  <img src="https://img.shields.io/badge/Build-Make-red?style=for-the-badge" alt="GNU Make">
</p>

<p align="center">
  <a href="#-overview">Overview</a> •
  <a href="#-features">Features</a> •
  <a href="#-architecture">Architecture</a> •
  <a href="#-how-it-works">Workflow</a> •
  <a href="#-getting-started">Getting Started</a> •
  <a href="#-testing">Testing</a> •
  <a href="#-demo-scenarios">Demos</a>
</p>

---

## 📌 Overview

This project implements a small-scale **Peer-to-Peer (P2P) file-sharing system inspired by BitTorrent**, using C and TCP sockets.

Instead of relying on a central server to distribute an entire file, the system divides files into smaller pieces that can be transferred independently between peers. A downloader can retrieve different pieces from multiple peers concurrently and reconstruct the original file after validating the received data.

The system also supports dynamic peer information exchange, piece availability tracking, automatic manifest updates, concurrent transfers, SHA-256 verification, and recovery from failed or corrupted transfers.


### Project Objectives

- Implement peer-to-peer communication using TCP sockets.
- Divide files into manageable pieces for independent transfer.
- Allow multiple peers to contribute pieces to a download.
- Download multiple pieces concurrently using POSIX threads.
- Track which peers advertise particular pieces.
- Notify connected peers when a file manifest becomes available or changes.
- Verify piece integrity using SHA-256 hashes.
- Recover from connection failures, corrupted data, and mid-transfer disconnections.
- Reconstruct the original file from downloaded pieces.
- Track peer upload and download contributions.
- Test networking, storage, concurrency, and recovery through unit and integration tests.

The project is implemented in C using POSIX sockets, POSIX threads, GNU Make, and OpenSSL.

---

## ✨ Features

| Feature | Status |
|---|:---:|
| TCP peer-to-peer communication | ✅ |
| Multiple peers in the network | ✅ |
| Peer discovery and peer-list exchange | ✅ |
| Peer information management | ✅ |
| File splitting into pieces | ✅ |
| Piece metadata and SHA-256 hashes | ✅ |
| File manifest exchange | ✅ |
| Automatic manifest update notifications | ✅ |
| Dynamic piece-availability updates | ✅ |
| Stale peer-to-piece mapping cleanup | ✅ |
| Peer-aware piece selection | ✅ |
| Rarest-first piece scheduling | ✅ |
| Concurrent piece downloads | ✅ |
| Multiple upload/download workers | ✅ |
| SHA-256 integrity verification | ✅ |
| Corrupted-piece detection and rejection | ✅ |
| Peer failure recovery | ✅ |
| Mid-transfer dropout recovery | ✅ |
| Transfer timeout handling | ✅ |
| File reconstruction | ✅ |
| Upload/download contribution tracking | ✅ |
| Fairness history persistence | ✅ |
| Unit and integration tests | ✅ |

---

## 🛠️ Technology Stack

| Technology | Purpose |
|---|---|
| C | Core implementation |
| TCP / POSIX sockets | Reliable peer communication |
| POSIX threads | Concurrent networking and transfers |
| OpenSSL | SHA-256 hashing |
| GNU Make | Compilation and test automation |
| GCC | C compilation |
| Linux / WSL Ubuntu | Development and execution environment |

---

## 🏗️ Architecture

The project is divided into modules for application control, protocol handling, networking, storage, transfers, scheduling, and recovery.

```text
p2p-file-sharing/
│
├── src/
│   ├── app/
│   │   ├── main.c
│   │   ├── p2p.c
│   │   └── p2p.h
│   │
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
│   │   ├── file_io.h
│   │   ├── piece_scheduler.c
│   │   └── piece_scheduler.h
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
│   ├── test_mid_transfer_dropout.c
│   ├── test_discovery.c
│   └── test_discovery_serialization.c
│
├── Makefile
├── README.md
└── .gitignore
```

*The tree highlights the main source modules and test files. Additional generated files and local test data may be present.*

### Module Responsibilities

| Module | Responsibility |
|---|---|
| `app/` | Application startup, command-line interface, and P2P orchestration |
| `common/` | Protocol message definitions, serialization, and shared types |
| `network/` | TCP peer connections, listeners, and peer discovery |
| `storage/` | File splitting, piece storage, hashes, availability, and scheduling |
| `transfer/` | Piece and block transfer operations |
| `recovery/` | Failure handling, timeouts, recovery, and fairness tracking |
| `tests/` | Unit tests and integration tests |

---

## 🔄 How It Works

```text
                  Original File
                       |
                       v
                Split into Pieces
                       |
          +------------+------------+
          |            |            |
          v            v            v
       Piece 0      Piece 1      Piece 2
          |            |            |
          v            v            v
        Peer 1       Peer 2       Peer 3
          |            |            |
          +------------+------------+
                       |
                       v
              Concurrent Downloads
                       |
                       v
              SHA-256 Verification
                       |
                       v
               File Reconstruction
                       |
                       v
                 Complete File
```

### Typical Workflow

1. A peer starts its TCP listener and initializes its P2P state.
2. Peers connect and exchange peer information.
3. A peer shares a file, which is divided into pieces.
4. The system creates a manifest containing metadata and expected hashes.
5. Known peers are notified when the manifest becomes available or changes.
6. Downloaders use piece-availability information to identify suitable sources.
7. Multiple workers request and transfer pieces concurrently.
8. Received pieces are checked against their expected SHA-256 hashes.
9. Failed or corrupted transfers can be retried using another available peer.
10. The downloaded pieces are reconstructed into the original file and verified.

---

## 📦 File Splitting and Manifests

The original file is divided into smaller pieces that can be transferred independently.

Each piece has metadata such as:

- **Piece ID:** Identifies the piece.
- **File offset:** Indicates where the piece belongs in the original file.
- **Piece size:** Records the size of the piece.
- **SHA-256 hash:** Provides an expected integrity value.
- **Availability information:** Tracks peers advertising the piece.

Example:

```text
Original File
    |
    +-- Piece 0
    +-- Piece 1
    +-- Piece 2
    +-- Piece 3
```

The manifest describes the pieces available from a peer. It allows other peers to discover piece metadata without downloading the complete file first.

### Manifest Updates

The project supports notifying known peers when a peer shares a file after connections have already been established.

The update uses the custom protocol message `MSG_MANIFEST_UPDATE`, followed by the file manifest.

This avoids requiring peers to disconnect and reconnect just to receive updated manifest information.

---

## 👥 Peer Management and Discovery

Each peer is identified by information such as:

```text
Peer ID
IP Address
Listening Port
```

The networking and discovery modules support:

- Starting a TCP listener.
- Accepting incoming peer connections.
- Connecting to known peers.
- Exchanging peer lists.
- Maintaining peer information.
- Updating existing peer records.
- Validating peer information.
- Supporting dynamically sized peer collections.

Peer discovery and peer-list exchange help participants learn about other peers in the network.

**Scope note:** This project does not implement a production-scale distributed hash table (DHT) or a centralized tracker service.

---

## 📡 Automatic Manifest Updates

One of the newer features is the ability to notify already-connected peers when a file manifest becomes available or changes.

### The Problem

Without a manifest-update mechanism, a peer that connects before another peer shares a file might not automatically receive the newly shared file's manifest. Reconnecting would otherwise be one way to obtain updated information.

### The Solution

When a peer shares a file, the application calls `p2p_announce_manifest()`.

The announcing peer:

1. Takes a snapshot of its known peer list.
2. Connects to each eligible known peer.
3. Sends a `MSG_MANIFEST_UPDATE` notification containing its peer ID.
4. Sends its file manifest.
5. Closes the notification connection and reports the result.

The receiving peer:

1. Recognizes the manifest-update message in its listener.
2. Validates the message format and claimed peer ID against its known peer records.
3. Receives and parses the file manifest.
4. Updates the peer-to-piece availability mappings.
5. Reports that the manifest was updated.

### Benefits

- Peers can discover newly shared files without reconnecting.
- Piece availability can be refreshed dynamically.
- Existing peer connections do not have to be restarted just to exchange a manifest.
- Old availability mappings for the announcing peer are removed when a replacement manifest is processed.

### Important Implementation Detail

Manifest-update notifications use a separate TCP connection to the receiving peer's listening port. They do not replace the existing peer connection.

The implementation validates the claimed peer ID against known peers, but this is not cryptographic peer authentication. The project remains an academic implementation rather than a security-hardened public file-sharing service.

---

## 🧩 Piece Availability

The availability system maintains a mapping between pieces and the peers that advertise them.

Example:

```text
Piece 0 -> Peer 1, Peer 2
Piece 1 -> Peer 3
Piece 2 -> Peer 2
```

When a downloader needs a piece, the availability table helps identify a peer that may supply it.

### Dynamic Availability Refresh

When a new manifest arrives, the receiving peer updates the announcing peer's piece mappings.

The refresh process:

1. Parses and validates the received manifest.
2. Removes the announcing peer from its old piece-availability mappings.
3. Adds the peer to the mappings for pieces in the new manifest.
4. Replaces the stored remote-manifest information.

This prevents the availability table from continuing to advertise pieces that were present in an earlier manifest but are absent from the replacement manifest.

Other peers advertising the same pieces remain in the availability table.

### Failure Handling

If a selected peer becomes unavailable, recovery can attempt another peer that advertises the requested piece.

---

## 🎯 Rarest-First Piece Scheduling

The project includes a piece scheduler that supports rarest-first ordering.

The principle is to prioritize pieces advertised by fewer peers, helping the downloader obtain less widely available pieces earlier.

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

This example illustrates the scheduling principle; actual selection depends on the availability information supplied to the scheduler.

The scheduler tests cover:

- Rarest-first ordering.
- Stable tie ordering.
- Unavailable-piece ordering.
- Empty input.
- Invalid arguments.

Rarest-first scheduling is a piece-selection strategy; it is not a complete BitTorrent choking or unchoking algorithm.

---

## ⚡ Concurrent Transfers

The project uses POSIX threads to perform multiple piece downloads concurrently.

Example:

```text
Worker 1 -> Download Piece 0 -> Peer 1
Worker 2 -> Download Piece 1 -> Peer 2
Worker 3 -> Download Piece 2 -> Peer 3
Worker 4 -> Download Piece 3 -> Peer 1
```

The integration tests exercise three-peer concurrent downloads and four-worker transfers.

### Benefits

- Multiple pieces can be transferred at the same time.
- Different peers can contribute different pieces.
- A failed transfer can be recovered without necessarily restarting the complete download.
- Piece integrity is checked before corrupted data is accepted.

Concurrency does not guarantee that every transfer will be faster: actual performance depends on network conditions, peer availability, and file size.

---

## 🔐 SHA-256 Integrity Verification

Each piece has an expected SHA-256 hash.

After receiving a piece, the system computes its hash and compares it with the expected value.

```text
             Received Piece
                   |
                   v
             Calculate Hash
                   |
                   v
          Compare SHA-256 Hashes
                   |
             +-----+-----+
             |           |
             v           v
           Match      Mismatch
             |           |
             v           v
           Accept      Reject
                         |
                         v
                 Attempt Recovery
```

If a piece fails integrity verification, it can be rejected and another available peer can be tried.

The project also verifies reconstructed files in its integration tests.

**Note:** SHA-256 verification detects data corruption relative to the expected hash. It does not authenticate a peer or protect the entire network against malicious participants.

---

## ♻️ Failure Recovery

A peer can fail, become unreachable, or disconnect during a transfer.

The recovery components handle scenarios such as:

- Connection failures.
- Peer unavailability.
- Transfer timeouts.
- Failed piece transfers.
- Corrupted pieces.
- Mid-transfer disconnections.

Example:

```text
Downloader
    |
    v
  Peer 1
    |
    X Connection Failed
    |
    v
  Peer 2
    |
    v
Piece Recovered
```

When an attempt fails, the recovery logic can try an alternate peer capable of supplying the requested piece.

### Timeout Handling

Timeout support helps detect stalled or unsuccessful operations and allows recovery logic to respond instead of waiting indefinitely for an expected result.

### Mid-Transfer Dropout Recovery

The integration test `test_mid_transfer_dropout` simulates a peer disconnecting during an active piece transfer.

The test checks that the incomplete transfer can be recovered from another peer, the pieces are reconstructed, and the final SHA-256 verification succeeds.

---

## 💥 Mid-Transfer Dropout Demo

Run:

```bash
./tests/test_mid_transfer_dropout
```

The test simulates the following scenario:

```text
Piece 0 is available from Peer 1 and Peer 2
                    |
                    v
             Request Piece 0
                    |
                    v
                  Peer 1
                    |
                    v
             Sends First Block
                    |
                    X Connection Drops
                    |
                    v
                 Recovery
                    |
                    v
                  Peer 2
                    |
                    v
             Piece 0 Recovered
                    |
                    v
          Reconstruct Complete File
                    |
                    v
           SHA-256 Verification
```

The integration test verifies that all required pieces are downloaded, the file is reconstructed, and the final hash matches.

---

## 🚨 Corruption Recovery

The project includes an integration test that simulates a peer supplying corrupted piece data.

Run:

```bash
./tests/test_corruption_recovery
```

The scenario is:

```text
Peer 1 sends corrupted data
              |
              v
       SHA-256 Mismatch
              |
              v
        Reject Piece
              |
              v
     Request from Peer 2
              |
              v
       Verify Correct Piece
              |
              v
            Accept
```

This prevents a piece that fails its expected hash check from being accepted as valid data during reconstruction.

---

## ⚖️ Fairness and Contribution Tracking

The project tracks peer upload and download contributions.

Example:

```text
Peer 1 -> Uploaded: 3 | Downloaded: 1
Peer 2 -> Uploaded: 1 | Downloaded: 3
Peer 3 -> Uploaded: 0 | Downloaded: 1
```

The fairness module supports contribution tracking and ratio calculations. The project also includes tests for saving and restoring fairness history and handling missing fairness files.

This provides a foundation for examining how peers contribute resources to the network.

**Scope note:** Contribution tracking is not the same as a complete BitTorrent tit-for-tat choking and unchoking mechanism.

---

## 🚀 Getting Started

### Requirements

The project is intended for Linux environments, including Ubuntu and WSL Ubuntu.

Required tools and libraries:

- GCC.
- GNU Make.
- OpenSSL development libraries.
- POSIX sockets.
- POSIX threads.

Install the dependencies on Ubuntu:

```bash
sudo apt update
sudo apt install build-essential libssl-dev
```

Verify the compiler and build tools:

```bash
gcc --version
make --version
```

### Clone the Repository

```bash
git clone https://github.com/shivam-ch/p2p-file-sharing.git
cd p2p-file-sharing
```

### Build

Compile the project:

```bash
make
```

This produces the `p2p` executable when compilation succeeds.

Clean generated build and test binaries:

```bash
make clean
```

### Run the Test Suite

```bash
make test
```

To perform a clean build before testing:

```bash
make clean
make
make test
```

---

## 🖥️ Running the P2P Application

The application accepts a peer ID and listening port.

General form:

```bash
./p2p <peer_id> <port>
```

For example, open separate terminals and start three peers:

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

Use distinct peer IDs and listening ports. The ports must be available on the machine.

These examples assume the peers are running on the same machine and that the application is configured to use the appropriate local peer addresses. For separate machines, use reachable IP addresses and ensure that the required ports are accessible.

### Interactive CLI

Once the application starts, it provides an interactive `p2p>` prompt.

The CLI includes commands for operations such as:

| Command | Purpose |
|---|---|
| `help` | Display available commands |
| `status` | Display local node status |
| `connect` | Connect to another peer |
| `peers` | Display known peers |
| `share` | Share a local file |
| `download` | Download a shared file |
| `fairness` | Display contribution information |
| `exit` | Exit the application |

Use `help` in the running application to see the exact syntax and arguments supported by the current build.

File paths containing spaces should be entered using the quoting supported by the CLI.

### Sharing and Manifest Updates

A typical workflow is:

1. Start the peers.
2. Establish peer connections.
3. Share a file using the CLI.
4. Allow the sharing peer to announce its manifest to known peers.
5. Use the receiving peers' updated availability information to request the file.
6. Verify the downloaded output.

The automatic manifest-update feature is especially useful when a peer shares a file after its other peers have already connected.

---

## 🧪 Testing

The project contains unit tests and integration tests for protocol handling, storage, availability, concurrency, networking, and recovery.

Run the complete suite:

```bash
make test
```

### Test Coverage

| Test | Purpose |
|---|---|
| `test_protocol` | Protocol definitions and message handling |
| `test_file_io` | File input/output |
| `test_piece` | Piece storage, status, and SHA-256 verification |
| `test_piece_block` | Piece-block serialization |
| `test_transfer` | Concurrent transfer logic |
| `test_recovery` | Peer failure recovery |
| `test_timeout` | Timeout detection and reset |
| `test_fairness` | Contribution tracking and fairness history |
| `test_availability` | Piece availability and peer selection |
| `test_piece_scheduler` | Rarest-first ordering and scheduler edge cases |
| `test_tcp_transfer` | TCP piece transfer |
| `test_tcp_concurrent_transfer` | Concurrent TCP transfers |
| `test_peer_aware_download` | Availability-based piece downloading |
| `test_recovery_transfer` | Recovery from an unavailable peer |
| `test_corruption_recovery` | Corrupted-piece detection and recovery |
| `test_three_peer_download` | Concurrent full-file download from three peers |
| `test_mid_transfer_dropout` | Recovery from a mid-transfer peer disconnection |
| `test_discovery` | Peer discovery, validation, and peer-list management |
| `test_discovery_serialization` | Peer-list serialization and deserialization |

The test suite also covers dynamic peer collections, duplicate peer updates, peer-list merging, concurrent downloads, file reconstruction, and final SHA-256 verification.

A successful run ends with:

```text
========================================
ALL TESTS PASSED
========================================
```

The test suite verifies the tested code paths. It does not imply that every possible network failure, malicious peer, or deployment environment has been tested.

---

## 🎬 Demo Scenarios

### 1. Three-Peer Concurrent Download

Run:

```bash
./tests/test_three_peer_download
```

Demonstrates:

```text
Piece 0 -> Peer 1
Piece 1 -> Peer 2
Piece 2 -> Peer 3
         |
         v
Concurrent Downloads
         |
         v
File Reconstruction
         |
         v
SHA-256 Verification
```

The test verifies that pieces can be supplied by three peers concurrently and that the reconstructed file passes final integrity verification.

### 2. Peer Dropout During Transfer

Run:

```bash
./tests/test_mid_transfer_dropout
```

Demonstrates:

- A peer disconnecting during a piece transfer.
- Recovery from an alternate peer.
- Concurrent downloading of multiple pieces.
- Complete file reconstruction.
- Final SHA-256 verification.

### 3. Corrupted Piece Recovery

Run:

```bash
./tests/test_corruption_recovery
```

Demonstrates:

- Detection of corrupted piece data.
- Rejection after a SHA-256 mismatch.
- Recovery from an alternate peer.
- Verification of the recovered piece.

### 4. Automatic Manifest Update

For a manual test:

1. Start three peers on separate terminals.
2. Connect the peers so they know about each other.
3. Share a file from one peer after the connections have been established.
4. Check that the announcing peer reports manifest notifications.
5. Check that the receiving peers report the updated manifest.
6. Download the shared file from another peer and verify the result.

This demonstrates that an already-connected peer can receive a new manifest without reconnecting.

The manual workflow is a demonstration scenario; the existing automated suite does not necessarily test every aspect of the CLI's manifest-announcement path.

---

## 📊 Test Coverage Summary

The project includes tests covering:

- Protocol definitions and serialization.
- File I/O and piece storage.
- TCP communication.
- Concurrent transfers.
- Peer discovery and peer-list management.
- Piece availability and scheduling.
- Peer-aware downloading.
- SHA-256 integrity verification.
- Corruption detection and recovery.
- Connection failures and timeouts.
- Mid-transfer peer dropout.
- File reconstruction.
- Fairness and contribution tracking.

Run the full suite with:

```bash
make test
```

---

## 📁 Important Commands

| Command | Description |
|---|---|
| `make` | Build the application |
| `make clean` | Remove generated build and test binaries |
| `make test` | Run the complete test suite |
| `./p2p <peer_id> <port>` | Start a peer |
| `./tests/test_three_peer_download` | Run the three-peer download test |
| `./tests/test_mid_transfer_dropout` | Run the dropout recovery test |
| `./tests/test_corruption_recovery` | Run the corruption recovery test |

---

## 🔮 Future Improvements

Potential improvements include:

- More scalable peer discovery.
- A distributed hash table (DHT) or tracker integration.
- Stronger peer authentication and message validation.
- More comprehensive manifest synchronization.
- Bitfield-based piece availability exchange.
- More advanced connection management.
- Bandwidth measurement and optimization.
- Improved retry policies and backoff.
- More extensive automated tests for manifest changes.
- Larger-scale distributed testing.
- A complete BitTorrent-style choking and unchoking mechanism.
- More robust handling of peers sharing different files or different versions of a file.
- Better observability, structured logging, and transfer statistics.

These are potential future directions, not claims that the corresponding functionality is already implemented.

---

## ⚠️ Project Scope and Limitations

This is a **small-scale academic implementation inspired by BitTorrent**, designed to demonstrate practical concepts in computer networks and systems programming.

The project focuses on:

- Peer-to-peer TCP communication.
- Piece-based file distribution.
- Concurrent downloads.
- Peer discovery and information exchange.
- File manifest exchange and updates.
- Piece availability and scheduling.
- SHA-256 integrity verification.
- Recovery from transfer failures.
- File reconstruction.
- Contribution tracking.

It is **not a complete production implementation of BitTorrent**.

In particular, the project does not claim to provide a production-scale DHT, a complete tracker infrastructure, cryptographic peer identity, or the full BitTorrent choking and unchoking system.

The current manifest-update design also relies on known peer records and the project's existing manifest and availability model. It should not be treated as a secure, general-purpose file-sharing service for untrusted networks.

---

## 👨‍💻 Team

### Peer-to-Peer File Sharing Network

| Contributors |
|---|
| **Shivam Kumar Chaurasia** |
| **Ayush Pathak** |
| **Sagar Raj Sharma** |
| **Aaditya Thakur** |

---

## 🎓 Academic Project

Developed as an academic project to demonstrate practical concepts in:

- Computer Networks.
- Peer-to-Peer Systems.
- TCP Socket Programming.
- Concurrent Programming.
- File Systems.
- Data Integrity.
- Failure Recovery.
- Network Protocol Design.

---

<p align="center">

## ⭐ Like the Project?

If this project helped you understand P2P networking, file sharing, or concurrent TCP programming, consider giving the repository a star!

**Built with C ❤️ and TCP**

</p>