# 🌉 LegacyBridge

> A zero-footprint C++17 ETL pipeline that streams real-time data from legacy Point-of-Sale systems directly into modern Cloud BI dashboards.

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.10%2B-064F8C.svg)](https://cmake.org/)
[![nlohmann/json](https://img.shields.io/badge/nlohmann%2Fjson-v3.11.3-informational.svg)](https://github.com/nlohmann/json)
[![cpp-httplib](https://img.shields.io/badge/cpp--httplib-v0.15.3-informational.svg)](https://github.com/yhirose/cpp-httplib)
[![OpenSSL](https://img.shields.io/badge/OpenSSL-enabled-brightgreen.svg)](https://www.openssl.org/)
[![Supabase](https://img.shields.io/badge/Backend-Supabase-3ECF8E.svg)](https://supabase.com/)
[![Metabase](https://img.shields.io/badge/Analytics-Metabase-509EE3.svg)](https://www.metabase.com/)

---

## 📋 Table of Contents

- [The Problem](#-the-problem)
- [The Solution](#-the-solution)
- [Architecture Deep Dive](#-architecture-deep-dive)
- [Tech Stack](#-tech-stack)
- [Project Structure](#-project-structure)
- [Getting Started](#-getting-started)
- [Configuration](#-configuration)
- [Running LegacyBridge](#-running-legacybridge)
- [How It Works End-to-End](#-how-it-works-end-to-end)
- [Engineering Decisions](#-engineering-decisions)

---

## 🔴 The Problem

Physical retail stores — including restaurants and food & beverage venues — depend on legacy Point-of-Sale hardware that locks transactional data inside **local CSV log files**. This creates a hard wall between real-time floor operations and modern business intelligence.

The obvious fix — replacing the hardware — is **prohibitively expensive** and operationally disruptive. Store owners are left blind: no live sales dashboards, no real-time per-table or per-category signals, no cross-venue analytics.

---

## ✅ The Solution

LegacyBridge is a **lightweight C++ agent** that runs silently alongside existing POS infrastructure. It:

1. **Monitors** a local CSV log (`mock_pos.csv`) for newly appended order rows, polling every 2 seconds
2. **Parses** each new row — `Timestamp`, `Table_ID`, `Item_Name`, `Category`, `Price` — into a structured `OrderInfo` record
3. **Queues** records into a thread-safe buffer shared between the file watcher and the cloud uploader
4. **Streams** them securely over HTTPS to Supabase's REST API (`/rest/v1/orders`)
5. **Retries** automatically on network failure, ensuring no order is silently dropped

The result: live sales data flows into **Metabase BI dashboards** in near-real-time — with no hardware replacement and no operational downtime.

---

## 🏗️ Architecture Deep Dive

LegacyBridge is built around a **decoupled Producer-Consumer architecture**. `POSWatcher` and `CloudUploader` run on independent threads, communicating exclusively through a shared `Buffer` — isolating file I/O from network I/O.

```
┌─────────────────────────────────────────────────────────────────────┐
│                          LegacyBridge Agent                         │
│                                                                     │
│   ┌──────────────────┐    Buffer (utils.h)     ┌─────────────────┐  │
│   │   POSWatcher     │ ──── std::mutex ───────▶│  CloudUploader  │  │
│   │   (Producer)     │   std::condition_var    │   (Consumer)    │  │
│   │                  │                         │                 │  │
│   │  seekg / tellg   │      OrderInfo          │  cpp-httplib    │  │
│   │  2s poll cycle   │      queue<T>           │  Retry (5x/3s)  │  │
│   └────────┬─────────┘                         └──────┬──────────┘  │
│            │                                          │             │
└────────────┼──────────────────────────────────────────┼─────────────┘
             │                                          │
             ▼                                          ▼
      [ mock_pos.csv ]                    [ Supabase REST API ]
      Timestamp, Table_ID,                /rest/v1/orders
      Item_Name, Category, Price         (PostgreSQL + Pooling)
                                                        │
                                                        ▼
                                             [ Metabase Dashboard ]
                                              (Live BI Analytics)
```

### Thread & Data Flow

| Stage | Component | Responsibility |
|---|---|---|
| **Watch** | `POSWatcher` | Tails `mock_pos.csv` for new rows via `seekg`/`tellg`; skips header |
| **Parse** | `POSWatcher::parseLine` | Tokenizes CSV row into an `OrderInfo` struct |
| **Buffer** | `Buffer` (`utils.h/.cpp`) | Thread-safe queue using `std::mutex` + `std::condition_variable` |
| **Upload** | `CloudUploader::sendToCloudWithRetry` | Serializes to JSON; POSTs to Supabase with up to 5 retries |
| **Persist** | Supabase / PostgreSQL | Stores rows in the `orders` table via REST API |
| **Visualize** | Metabase | Renders live dashboards connected to the Supabase database |

---

## 🛠️ Tech Stack

| Layer | Technology | Detail |
|---|---|---|
| **Core Engine** | C++17 | `std::thread`, `std::mutex`, `std::condition_variable` |
| **Build System** | CMake 3.10+ with `FetchContent` | Auto-fetches all C++ dependencies at configure time |
| **JSON** | nlohmann/json `v3.11.3` | Serializes `OrderInfo` to JSON payload |
| **HTTP Client** | cpp-httplib `v0.15.3` | HTTPS POST to Supabase REST API |
| **TLS/SSL** | OpenSSL (`CPPHTTPLIB_OPENSSL_SUPPORT`) | Secure transport; required system dependency |
| **Cloud DB** | Supabase (PostgreSQL) | REST API target at `/rest/v1/orders` |
| **Analytics** | Metabase | BI dashboards connected to the Supabase database |
| **Simulation** | Python 3 | Mock POS log generator (`mock_pos.py`) |

---

## 📁 Project Structure

```
LegacyBridge/
├── CMakeLists.txt        # Build config: FetchContent for json & httplib, links OpenSSL
├── main.cpp              # Entry point: constructs Buffer, POSWatcher, CloudUploader; spawns threads
├── POSWatchdog.h         # POSWatcher class declaration (file path, buffer ref, last_pos)
├── POSWatchdog.cpp       # Implements 2s poll loop (seekg/tellg) and parseLine()
├── CloudUploader.h       # CloudUploader class declaration (buffer ref, Supabase URL & key)
├── CloudUploader.cpp     # Implements start() consumer loop and sendToCloudWithRetry()
├── utils.h               # OrderInfo struct + Buffer class declaration
├── utils.cpp             # Buffer::push() and Buffer::pop() implementations
└── mock_pos.py           # Simulates a live restaurant POS: writes CSV rows every 1–4 seconds
```

### Key Data Structures (`utils.h`)

```cpp
// Represents a single parsed POS transaction
struct OrderInfo {
    string timestamp;   // e.g. "2025-01-15 14:32:07"
    int    table_id;    // 1–15
    string item_name;   // e.g. "Shakshuka"
    string category;    // "Food", "Alcohol", or "Beverage"
    float  price;       // In ILS (e.g. 48.0)
};

// Thread-safe queue shared between POSWatcher (producer) and CloudUploader (consumer)
class Buffer {
    queue<OrderInfo>    q;
    mutex               mtx;
    condition_variable  cv;
public:
    void push(OrderInfo info);  // Locks, pushes, notifies consumer
    bool pop(OrderInfo& info);  // Blocks until an item is available
};
```

---

## 🚀 Getting Started

### Prerequisites

- A C++17-compliant compiler (`g++ 8+` or `clang++ 7+`)
- `CMake 3.10+`
- `OpenSSL` development libraries
- `Python 3` (for the mock POS simulator)

**Install OpenSSL (if needed):**

```bash
# Ubuntu / Debian
sudo apt-get install libssl-dev

# macOS (Homebrew)
brew install openssl
```

### Build

All C++ dependencies (`nlohmann/json v3.11.3`, `cpp-httplib v0.15.3`) are resolved **automatically** via CMake's `FetchContent` during the configure step — no manual installs required.

```bash
# 1. Clone the repository
git clone https://github.com/your-username/LegacyBridge.git
cd LegacyBridge

# 2. Create an isolated build directory
mkdir build && cd build

# 3. Configure — dependencies are fetched automatically here
cmake ..

# 4. Compile
cmake --build .
```

The `LegacyBridge` executable will be available inside `build/` on success.

---

## ⚙️ Configuration

Before running, open `main.cpp` and update the two required values:

**1. CSV file path** — must point to the exact location where `mock_pos.py` writes its output on your machine:

```cpp
// main.cpp
POSWatcher watcher("/absolute/path/to/your/mock_pos.csv", order_buffer);
```

**2. Supabase credentials** — your project's domain and API key:

```cpp
// main.cpp
string supabase_domain = "your-project-ref.supabase.co";
string supabase_key    = "your-supabase-anon-or-service-key";
```

Your credentials can be found in the Supabase dashboard under:
**Project Settings → API → Project URL & API Keys**

> **🔒 Security Warning:** Credentials are currently hardcoded as string literals. Do **not** commit live keys to source control. For any shared or production deployment, load secrets from environment variables or a secrets manager instead.

### Supabase Table Schema

The uploader POSTs to `/rest/v1/orders`. Ensure this table exists in your Supabase project before running:

```sql
CREATE TABLE orders (
    id         BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    timestamp  TEXT,
    table_id   INTEGER,
    item_name  TEXT,
    category   TEXT,
    price      REAL
);
```

---

## ▶️ Running LegacyBridge

LegacyBridge requires **two terminals** running simultaneously.

### Terminal 1 — Start the Mock POS Simulator

`mock_pos.py` simulates a restaurant POS by appending a randomized order row to `mock_pos.csv` every 1–4 seconds. The menu includes food items (Steak Tartare, Bibimbap Bulgogi, Beef Stir-fry, Shakshuka, Hummus Plate), alcoholic drinks (Goldstar Beer, Arak), and beverages (Cola) — all priced in ILS, across 15 tables.

```bash
# From the repository root
python3 mock_pos.py
```

Expected output:
```
Started writing mock POS data to mock_pos.csv...
Press Ctrl+C to stop.

[2025-01-15 14:32:07] Added order: Table 4 ordered Shakshuka (48 ILS)
[2025-01-15 14:32:10] Added order: Table 11 ordered Goldstar Beer (28 ILS)
[2025-01-15 14:32:13] Added order: Table 2 ordered Bibimbap Bulgogi (72 ILS)
```

### Terminal 2 — Start the LegacyBridge Agent

```bash
# From the build directory
./LegacyBridge
```

Expected output (normal operation):
```
Starting LegacyBridge Service...
Starting POS Watcher on: /path/to/mock_pos.csv
[Uploader] Started. Waiting for data...
[Watcher] Queued new order: Shakshuka (Table 4)
[Uploader] SUCCESS: Sent Shakshuka to cloud.
[Watcher] Queued new order: Goldstar Beer (Table 11)
[Uploader] SUCCESS: Sent Goldstar Beer to cloud.
```

Expected output (on network failure):
```
[Uploader] ERROR 0. Attempt 1/5 failed. Retrying in 3s...
[Uploader] ERROR 0. Attempt 2/5 failed. Retrying in 3s...
[Uploader] SUCCESS: Sent Shakshuka to cloud.
```

Once records are flowing into Supabase, open your **Metabase dashboard** to see live sales data populate in real time.

---

## 🔄 How It Works End-to-End

```
mock_pos.py                  POSWatcher                  CloudUploader         Supabase
     │                            │                             │                  │
     │─── appends CSV row ───────▶│                             │                  │
     │                            │── seekg(last_pos)           │                  │
     │                            │── getline() delta read      │                  │
     │                            │── skip "Timestamp" header   │                  │
     │                            │── parseLine() → OrderInfo   │                  │
     │                            │── Buffer::push()            │                  │
     │                            │   lock_guard(mtx)           │                  │
     │                            │   q.push(info)              │                  │
     │                            │   cv.notify_one() ─────────▶│                  │
     │                            │── last_pos = tellg()        │                  │
     │                            │── sleep(2s)                 │                  │
     │                            │                             │── cv.wait(lock)  │
     │                            │                             │── q.pop()        │
     │                            │                             │── json dump      │
     │                            │                             │── SSLClient POST▶│
     │                            │                             │                  │── INSERT row
     │                            │                             │◀── HTTP 201 ─────│
     │                            │                             │                  │
     │                            │                        [on failure]            │
     │                            │                             │── sleep(3s)      │
     │                            │                             │── retry (max 5x)▶│
```

---

## 🧠 Engineering Decisions

### 1. 🗂️ Delta-Only File Watching (`POSWatcher`)

`POSWatcher` never re-reads the full CSV on each poll cycle. It stores the last-read byte offset in `last_pos` (`std::streampos`, initialized to `0`) and calls `seekg(last_pos)` at the start of each 2-second iteration, reading only lines that have been appended since the previous pass. After draining new lines, `tellg()` advances the bookmark. A `file.clear()` call resets any EOF flags before the seek, which is essential for the re-open-per-cycle pattern used here.

The header row is explicitly skipped by checking for the string `"Timestamp"`, ensuring it is never passed to `parseLine()` regardless of when the file is first opened.

```cpp
// POSWatchdog.cpp
file.seekg(last_pos);                                    // Jump to last known position

string line;
while (getline(file, line)) {
    if (line.find("Timestamp") != string::npos) continue; // Skip CSV header
    if (line.empty()) continue;
    OrderInfo info = parseLine(line);
    q.push(info);
}

file.clear();
last_pos = file.tellg();                                 // Advance the bookmark
```

**Why it matters:** Re-scanning a growing log file on every 2-second poll wastes CPU and RAM on aging POS hardware. Delta reads keep resource usage near-zero regardless of log file size.

---

### 2. 🔒 Thread-Safe Buffer (`utils.h` / `utils.cpp`)

The `Buffer` class is the sole communication channel between `POSWatcher` (producer) and `CloudUploader` (consumer), each running on their own `std::thread` spawned in `main.cpp`. `push()` uses a `std::lock_guard` for scoped, exception-safe locking and calls `cv.notify_one()` to wake the consumer. `pop()` uses `std::unique_lock` with a `cv.wait()` predicate, blocking the consumer thread entirely when the queue is empty — eliminating CPU busy-waiting.

```cpp
// utils.cpp
void Buffer::push(OrderInfo info) {
    std::lock_guard<std::mutex> lock(mtx);
    q.push(std::move(info));
    cv.notify_one();                              // Wake the blocked consumer
}

bool Buffer::pop(OrderInfo& info) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return !q.empty(); }); // Sleep until data arrives
    info = std::move(q.front());
    q.pop();
    return true;
}
```

**Why it matters:** Without this, the watcher and uploader would either block each other on a single thread or race on an unprotected shared data structure, risking data corruption and dropped records.

---

### 3. 🌐 Retry Mechanism with Fixed Delay (`CloudUploader`)

`sendToCloudWithRetry()` wraps every HTTPS POST in a retry loop capped at **5 attempts**. On any failure — whether a connection error (status `0`) or an unexpected HTTP status — it logs the attempt number and status code, waits a fixed **3 seconds**, and retries. Both HTTP `200` and `201` are treated as success, accommodating Supabase's `201 Created` response on `INSERT`. If all 5 attempts are exhausted, a `CRITICAL` log line is emitted.

```cpp
// CloudUploader.cpp
while (!success && current_try < max_retries) {
    current_try++;
    auto res = cli.Post("/rest/v1/orders", headers, payload, "application/json");

    if (res && (res->status == 201 || res->status == 200)) {
        success = true;
    } else {
        int status = res ? res->status : 0;
        std::cerr << "[Uploader] ERROR " << status << ". Attempt "
                  << current_try << "/" << max_retries << " failed. Retrying in 3s..." << endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

if (!success) {
    std::cerr << "[Uploader] CRITICAL: Failed to send data after "
              << max_retries << " attempts. Data dropped." << endl;
}
```

**Why it matters:** Physical store Wi-Fi is unreliable. Without retry logic, any transient network blip would silently drop an order and corrupt BI metrics downstream in Metabase.

---

*Built to bridge the gap between the hardware stores can't replace and the data infrastructure they deserve.*

---

👨‍💻 Author
Etay De-Beer - B.Sc. Computer Science Student.
