# Keploy C++ Quickstart

<p align="center">
  <img src="https://keploy.io/docs/img/keploy-logo-dark.svg" alt="Keploy Logo" width="200"/>
</p>

A production-ready **C++ Todo CRUD API** demonstrating [Keploy](https://keploy.io/) integration for automated API testing — **zero SDK, zero manual instrumentation**.

## What is Keploy?

[Keploy](https://keploy.io/) is an open-source, language-agnostic testing platform that:

- **Records** real API traffic (HTTP requests/responses + database calls)
- **Replays** them as regression tests
- **Compares** actual vs recorded responses — catching bugs automatically

No code changes or SDKs required.

## Tech Stack

| Component | Technology |
|:----------|:-----------|
| Language  | C++ 17 |
| Framework | [Crow](https://crowcpp.org/) (header-only HTTP framework) |
| Database  | [PostgreSQL 15](https://www.postgresql.org/) via `libpqxx` |
| Build     | [CMake](https://cmake.org/) ≥ 3.14 |
| Container | Docker + Docker Compose |
| Testing   | [Keploy](https://keploy.io/) |

## Features

- Full CRUD REST API (`GET`, `POST`, `PUT`, `DELETE`)
- Health check endpoint (`/health`)
- PostgreSQL with connection retry logic
- Multi-stage Docker build (small runtime image)
- Docker Compose with health checks
- Keploy record & replay — end-to-end

## Quick Start

### With Docker (Recommended)

```bash
cd keploy-cpp-quickstart
docker compose up --build
```

The API is now available at **http://localhost:8080**.

### Without Docker

```bash
cd keploy-cpp-quickstart

# Install dependencies (Ubuntu)
sudo apt install -y g++ cmake libpqxx-dev libasio-dev postgresql pkg-config

# Start PostgreSQL & create DB
sudo service postgresql start
sudo -u postgres psql -c "CREATE DATABASE keploy_db;"

# Build & run
cmake -B build && cmake --build build --parallel $(nproc)
DB_HOST=localhost ./build/app
```

## Keploy — Record & Replay

### Install Keploy

```bash
curl --silent -O -L https://keploy.io/install.sh && source install.sh
```

### Record API Traffic

```bash
keploy record \
  -c "docker compose up --build" \
  --container-name keploy-cpp-app
```

Make API calls in another terminal, then stop with `Ctrl+C`.

### Replay as Tests

```bash
keploy test \
  -c "docker compose up --build" \
  --container-name keploy-cpp-app
```

Keploy replays recorded requests and compares responses — any regression is caught automatically.

## API Endpoints

| Method   | Endpoint       | Description       |
|:---------|:---------------|:------------------|
| `GET`    | `/health`      | Health check      |
| `GET`    | `/todos`       | List all todos    |
| `GET`    | `/todos/<id>`  | Get a todo        |
| `POST`   | `/todos`       | Create a todo     |
| `PUT`    | `/todos/<id>`  | Update a todo     |
| `DELETE` | `/todos/<id>`  | Delete a todo     |

## Project Structure

```
keploy-cpp-quickstart/
├── src/main.cpp           # Application code
├── CMakeLists.txt         # Build config (auto-fetches Crow)
├── Dockerfile             # Multi-stage Docker build
├── docker-compose.yml     # App + PostgreSQL
├── keploy.yml             # Keploy configuration
├── keploy/                # Generated test-sets & mocks
└── README.md              # Detailed guide
```

## Documentation

See [keploy-cpp-quickstart/README.md](keploy-cpp-quickstart/README.md) for the full step-by-step guide including:

- Detailed local setup
- Docker Compose configuration
- Keploy record & replay walkthrough
- Expected outputs
- Troubleshooting

## Contributing

Contributions are welcome! Please open an issue or submit a pull request.

## License

This project is open source and available under the [MIT License](LICENSE).