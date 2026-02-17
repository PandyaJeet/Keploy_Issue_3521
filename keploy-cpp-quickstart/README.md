# Keploy C++ Quickstart – Todo CRUD API

A beginner-friendly guide to building, testing, and ensuring reliability of a **C++ REST API** using [Crow](https://crowcpp.org/), [PostgreSQL](https://www.postgresql.org/), and [Keploy](https://keploy.io/).

> **Keploy** is language-agnostic — it records real API traffic and replays it as regression tests, **without SDKs or manual instrumentation**.

---

## Table of Contents

- [Project Overview](#-project-overview)
- [Folder Structure](#-folder-structure)
- [Prerequisites](#-prerequisites)
- [Local Setup (Without Docker)](#-local-setup-without-docker)
- [Docker Compose Setup](#-docker-compose-setup)
- [Keploy Integration](#-keploy-integration)
  - [Record Mode](#-record-mode)
  - [Replay / Test Mode](#-replay--test-mode)
- [API Reference](#-api-reference)
- [Expected Outputs](#-expected-outputs)
- [Troubleshooting](#-troubleshooting)

---

## 📖 Project Overview

This project implements a **Todo CRUD API** in C++ with the following stack:

| Layer       | Technology                      |
|:----------- |:------------------------------- |
| Framework   | [Crow](https://crowcpp.org/) (header-only C++ HTTP framework) |
| Database    | [PostgreSQL 15](https://www.postgresql.org/) via `libpqxx` |
| Build       | [CMake](https://cmake.org/) ≥ 3.14 |
| Container   | Docker + Docker Compose |
| Testing     | [Keploy](https://keploy.io/) (record & replay) |

**What you will learn:**

- Build a full CRUD REST API in C++ with Crow
- Connect to PostgreSQL using `libpqxx`
- Containerise the app with Docker (multi-stage build)
- Use Keploy to record API calls and replay them as regression tests

---

## 📂 Folder Structure

```
keploy-cpp-quickstart/
├── src/
│   └── main.cpp             # Application logic — routes, DB connection, CRUD
├── keploy/                  # Keploy test-sets & mocks (generated at runtime)
├── CMakeLists.txt           # Build configuration (fetches Crow automatically)
├── Dockerfile               # Multi-stage Docker build
├── docker-compose.yml       # App + PostgreSQL services
├── keploy.yml               # Keploy configuration
├── .gitignore               # Ignored files
└── README.md                # This file
```

---

## 🔧 Prerequisites

| Tool | Version | Installation |
|:---- |:------- |:------------ |
| C++ compiler (g++) | ≥ 9 | `sudo apt install g++` |
| CMake | ≥ 3.14 | `sudo apt install cmake` |
| libpqxx | ≥ 6 | `sudo apt install libpqxx-dev` |
| Asio headers | — | `sudo apt install libasio-dev` |
| PostgreSQL | ≥ 13 | `sudo apt install postgresql` |
| Docker & Compose | latest | [docs.docker.com](https://docs.docker.com/get-docker/) |
| Keploy | latest | [keploy.io/docs/server/installation](https://keploy.io/docs/server/installation/) |

---

## 💻 Local Setup (Without Docker)

### 1. Install Dependencies

**Ubuntu / Debian:**

```bash
sudo apt update
sudo apt install -y g++ cmake libpqxx-dev libasio-dev \
                    postgresql postgresql-contrib pkg-config git
```

**macOS:**

```bash
brew install cmake libpqxx postgresql asio pkg-config
```

### 2. Set Up PostgreSQL

```bash
# Start PostgreSQL
sudo service postgresql start          # Linux
# brew services start postgresql       # macOS

# Create the database
sudo -u postgres psql -c "CREATE DATABASE keploy_db;"
```

### 3. Build the Application

```bash
cd keploy-cpp-quickstart

# Configure & build (Crow is fetched automatically via CMake)
cmake -B build
cmake --build build --parallel $(nproc)
```

### 4. Run the Application

```bash
DB_HOST=localhost ./build/app
```

You should see:

```
[db] Connected to PostgreSQL (attempt 1)
[db] Migration complete.
[app] Starting on port 8080 …
```

### 5. Test the Endpoints

```bash
# Health check
curl http://localhost:8080/health

# Create a todo
curl -X POST http://localhost:8080/todos \
  -H "Content-Type: application/json" \
  -d '{"task": "Learn Keploy"}'

# List all todos
curl http://localhost:8080/todos

# Get a single todo
curl http://localhost:8080/todos/1

# Update a todo
curl -X PUT http://localhost:8080/todos/1 \
  -H "Content-Type: application/json" \
  -d '{"task": "Master Keploy", "done": true}'

# Delete a todo
curl -X DELETE http://localhost:8080/todos/1
```

---

## 🐳 Docker Compose Setup

Docker Compose spins up **both the app and PostgreSQL** with a single command.

### `docker-compose.yml` highlights

- **`db` service**: PostgreSQL 15 with a `healthcheck` — the app won't start until the DB is ready.
- **`app` service**: Multi-stage build; `depends_on … condition: service_healthy`.
- **Persistent volume** `pgdata` keeps data across restarts.

### Run

```bash
cd keploy-cpp-quickstart

# Build & start
docker compose up --build
```

The app is available at **http://localhost:8080**.

To stop:

```bash
docker compose down          # keeps volume
docker compose down -v       # removes volume too
```

---

## 🐰 Keploy Integration

Keploy records your API traffic and converts it into test cases — **zero code changes required**.

### Install Keploy

```bash
curl --silent -O -L https://keploy.io/install.sh && source install.sh
```

Verify:

```bash
keploy --version
```

---

### 📼 Record Mode

In **Record** mode Keploy proxies traffic, captures HTTP requests/responses **and** database calls, and saves them as YAML test-sets.

```bash
keploy record \
  -c "docker compose up --build" \
  --container-name keploy-cpp-app
```

> **Note:** The `--container-name` must match the `container_name` in `docker-compose.yml`.

While Keploy is recording, fire some requests in **another terminal**:

```bash
# 1. Create todos
curl -X POST http://localhost:8080/todos \
  -H "Content-Type: application/json" \
  -d '{"task": "Buy groceries"}'

curl -X POST http://localhost:8080/todos \
  -H "Content-Type: application/json" \
  -d '{"task": "Read a book"}'

# 2. List todos
curl http://localhost:8080/todos

# 3. Get single todo
curl http://localhost:8080/todos/1

# 4. Update a todo
curl -X PUT http://localhost:8080/todos/1 \
  -H "Content-Type: application/json" \
  -d '{"task": "Buy organic groceries", "done": true}'

# 5. Delete a todo
curl -X DELETE http://localhost:8080/todos/2
```

Stop recording with **Ctrl + C**. Keploy creates files under `keploy/`:

```
keploy/
├── test-set-0/
│   ├── tests/
│   │   ├── test-1.yaml
│   │   ├── test-2.yaml
│   │   └── ...
│   └── mocks.yaml
```

---

### 🎬 Replay / Test Mode

Replay mode re-sends the recorded requests and **compares** live responses against the recorded ones. Database calls are mocked automatically.

```bash
keploy test \
  -c "docker compose up --build" \
  --container-name keploy-cpp-app
```

**What happens:**

1. Keploy starts the app and DB via Docker Compose.
2. It replays every recorded request from the test-set.
3. Each response is compared field-by-field with the recorded expectation.
4. A pass/fail summary is printed.

---

## 🔌 API Reference

| Method   | Endpoint       | Description           | Request Body                        | Success Response            |
|:-------- |:-------------- |:--------------------- |:----------------------------------- |:--------------------------- |
| `GET`    | `/health`      | Health + DB check     | —                                   | `200 {"status":"ok","database":"connected"}` |
| `GET`    | `/todos`       | List all todos        | —                                   | `200 [{"id":1,"task":"…","done":false}]` |
| `GET`    | `/todos/<id>`  | Get a single todo     | —                                   | `200 {"id":1,"task":"…","done":false}` |
| `POST`   | `/todos`       | Create a todo         | `{"task":"…"}`                      | `201 {"id":1,"task":"…","done":false}` |
| `PUT`    | `/todos/<id>`  | Update a todo         | `{"task":"…","done":true}` (partial)| `200 {"id":1,"task":"…","done":true}` |
| `DELETE` | `/todos/<id>`  | Delete a todo         | —                                   | `200 {"message":"Todo deleted","id":1}` |

---

## ✅ Expected Outputs

### Application Startup

```
[db] Connected to PostgreSQL (attempt 1)
[db] Migration complete.
[app] Starting on port 8080 …
```

### Keploy Record

```
🐰 Keploy has captured test cases for the user's]
  Total captured: 6
  Saved to: keploy/test-set-0
```

### Keploy Test (Replay)

```
🐰 Keploy Test Summary

  Test Set: test-set-0
  Total tests: 6
  Passed:      6
  Failed:      0

  Result: ALL PASSED ✅
```

If you introduce a regression (e.g. rename `"task"` to `"title"` in the response), the replay will **FAIL** and show the diff — catching the breaking change automatically.

---

## ❓ Troubleshooting

| Problem | Cause | Fix |
|:--------|:------|:----|
| `Failed to connect to DB` | Postgres not ready yet | The app retries 10 times automatically. If it still fails, check that Postgres is running and credentials match. |
| `Keploy not detecting container` | Wrong `--container-name` | Use `--container-name keploy-cpp-app` (must match `container_name` in `docker-compose.yml`). |
| `Port 8080 already in use` | Another process on 8080 | `lsof -i :8080` to find it, or change the port in `docker-compose.yml`. |
| `CMake can't find libpqxx` | Missing dev package | `sudo apt install libpqxx-dev pkg-config` |
| Docker build fails at Crow | Network issue during FetchContent | Ensure internet access; or pre-clone Crow into `Crow/` and adjust CMakeLists.txt. |

---

## 🚀 Next Steps

- **Break something on purpose** — change `"task"` to `"title"` in `main.cpp`, run `keploy test`, and watch it catch the regression.
- **Add more endpoints** — try adding pagination, search, or tags.
- **Try gRPC** — Keploy supports recording/replaying gRPC traffic as well.

---

## 📚 Resources

- [Keploy Documentation](https://keploy.io/docs/)
- [Crow C++ Documentation](https://crowcpp.org/master/)
- [libpqxx Documentation](https://pqxx.org/development/libpqxx/)
- [PostgreSQL Documentation](https://www.postgresql.org/docs/)

---

**Happy testing! 🐰**
