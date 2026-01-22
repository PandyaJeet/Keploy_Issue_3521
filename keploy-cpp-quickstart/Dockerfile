FROM ubuntu:22.04

WORKDIR /app

RUN apt update && apt install -y \
    g++ \
    cmake \
    libpqxx-dev \
    libasio-dev \
    git \
    curl \
    pkg-config

COPY . .

RUN mkdir build && cd build && cmake .. && make

EXPOSE 8080

CMD ["./build/app"]
