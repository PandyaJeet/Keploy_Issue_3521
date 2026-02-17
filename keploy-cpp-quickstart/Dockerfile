# ── Stage 1: Build ──────────────────────────────────────────────────────
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        g++ cmake make git ca-certificates \
        libpqxx-dev libpq-dev libasio-dev pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel "$(nproc)"

# ── Stage 2: Runtime ───────────────────────────────────────────────────
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        libpqxx-6.4 libpq5 curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/app .

EXPOSE 8080

HEALTHCHECK --interval=5s --timeout=3s --retries=5 \
    CMD curl -fs http://localhost:8080/health || exit 1

CMD ["./app"]
