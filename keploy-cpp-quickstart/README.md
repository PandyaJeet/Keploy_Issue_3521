# Keploy C++ Quickstart

A comprehensive guide to building, testing, and ensuring the reliability of a C++ backend using [Crow](https://crowcpp.org/), [PostgreSQL](https://www.postgresql.org/), and [Keploy](https://keploy.io/).

## 📖 Project Overview

This project demonstrates a production-grade setup for a C++ backend service. It solves the problem of "how do I test my C++ microservice efficiently?" by introducing **Keploy**, an open-source testing platform that generates test cases from API traffic.

**What you will learn:**
- How to build a REST API with **C++ (Crow)**.
- How to integrate **PostgreSQL** using `libpqxx`.
- How to containerize the application with **Docker**.
- How to use **Keploy** to record API calls and replay them as regression tests.

---

## 📂 Folder Structure

```
keploy-cpp-quickstart/
├── src/
│   └── main.cpp           # Main application logic (API endpoints, DB connection)
├── Crow/                  # Crow framework (header-only C++ web framework)
├── Dockerfile             # Instructions to build the container image
├── docker-compose.yml     # Defines the App and Database services
├── CMakeLists.txt         # Build configuration for CMake
├── keploy.yml             # Keploy configuration file
├── README.md              # This documentation
└── .gitignore             # Files to ignore in Git
```

---

## 🛠️ Local Setup (Without Docker)

You can run the application directly on your machine for development.

### 1. Prerequisites

You need `cmake`, a C++ compiler, and `libpqxx` (PostgreSQL C++ client) installed.

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install -y g++ cmake libpqxx-dev libasio-dev postgresql postgresql-contrib
```

**MacOS:**
```bash
brew install cmake libpqxx postgresql
```

### 2. Setup Database

Start your local PostgreSQL service and create a user/database:
```bash
# Start Postgres (Linux)
sudo service postgresql start

# Login to Postgres
sudo -u postgres psql

# Inside SQL shell:
CREATE USER postgres WITH PASSWORD 'postgres';
CREATE DATABASE keploy_db;
GRANT ALL PRIVILEGES ON DATABASE keploy_db TO postgres;
\q
```

### 3. Build & Run

```bash
mkdir build
cd build
cmake ..
make

# Run the app (using localhost for DB)
DB_HOST=localhost ./app
```

### 4. Test the API

**Create a Todo:**
```bash
curl -X POST http://localhost:8080/todos \
  -d '{"task": "Learn Keploy"}' \
  -H "Content-Type: application/json"
```

**Get Todos:**
```bash
curl http://localhost:8080/todos
```

**Expected Output:**
```json
[{"id":1,"task":"Learn Keploy"}]
```

---

## 🐳 Docker Setup (Recommended)

This sets up the Application and Database automatically.

**File:** `docker-compose.yml`

```yaml
services:
  app:
    build: .
    ports:
      - "8080:8080"
    depends_on:
      - db
    environment:
      - DB_HOST=db  # Points to the service name 'db'

  db:
    image: postgres:15
    environment:
      POSTGRES_USER: postgres
      POSTGRES_PASSWORD: postgres
      POSTGRES_DB: keploy_db
```

### Run with Docker Compose

```bash
docker compose up --build
```

---

## 🐰 Keploy Integration

Keploy acts as a middleware that records your API traffic and converts it into test cases.

### 📼 1. Record Mode

In Record mode, Keploy acts as a proxy, capturing network calls.

```bash
# Start Keploy in Record Mode
keploy record \
  --cmd-type docker-compose \
  -c "docker compose up" \
  --container-name app
```

**Steps to Record:**
1. Run the command above. Keploy will start your containers.
2. In a separate terminal, make API calls:

```bash
# Add a Task
curl -X POST http://localhost:8080/todos \
  -d '{"task": "Master Keploy"}' \
  -H "Content-Type: application/json"

# Retrieve Tasks
curl http://localhost:8080/todos
```

3. Keploy captures these requests and saves them as test cases (`test-set-0.yaml`) and mocks (`mocks.yaml`) in the `keploy/` directory.
4. Stop Keploy with `Ctrl+C`.

### 🎬 2. Replay Mode (Testing)

Now, let's verify if the application still works as expected. Keploy will replay the recorded requests and compare the responses.

```bash
# Start Keploy in Test Mode
keploy test \
  --cmd-type docker-compose \
  -c "docker compose up" \
  --container-name app
```

**What happens:**
- Keploy starts the app.
- It replays the recorded API calls.
- It compares the *actual* response from the app with the *recorded* response.
- It matches database interactions using the recorded mocks (no live DB needed in theory, but for this setup we spin up the whole stack).

---

## ✅ expected Output

**During Replay:**

```
Test Run Summary:
  Total tests: 2
  Passed: 2
  Failed: 0
  Time taken: 1.2s

<INFO> Test run complete
```

If you modify the code (e.g., change the response format) and run `keploy test` again, it will report a **FAIL**, catching the regression.

---

## 🔌 API Specification

| Method | Endpoint | Description | Request Body | Response |
| :--- | :--- | :--- | :--- | :--- |
| `POST` | `/todos` | Create a new todo item | `{"task": "..."}` | `201 Inserted` |
| `GET` | `/todos` | List all todo items | N/A | `[{"id": 1, "task": "..."}]` |

---

## ❓ Common Errors & Fixes

**1. `Failed to connect to DB`**
- **Cause:** Database container isn't ready when App starts.
- **Fix:** In Docker, we use `depends_on`, but sometimes Postgres takes a few seconds longer. Restarting the app container usually fixes this. In production, use a retry logic code.

**2. `Keploy not detecting container`**
- **Cause:** The `--container-name` flag doesn't match the service name in docker-compose.
- **Fix:** Ensure you use `--container-name app` because our service is named `app` in `docker-compose.yml`.

**3. `Port 8080 already in use`**
- **Cause:** Another service is running on port 8080.
- **Fix:** Stop other services or change the port mapping in `docker-compose.yml` (e.g., `"8081:8080"`).

---

## 🔒 Next Steps

Try changing the code in `src/main.cpp` (e.g., change "task" key to "title") and run `keploy test`. See how Keploy catches the breaking change!
