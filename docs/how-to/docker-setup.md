# Docker Development Setup

This guide covers using Docker for QLever development, both as an end-user runtime and as a development environment.

## Table of Contents

1. [End-User Docker Setup](#end-user-docker-setup)
2. [Development Container Setup](#development-container-setup)
3. [Docker Compose Multi-Container](#docker-compose-multi-container)
4. [Troubleshooting](#troubleshooting)

---

## End-User Docker Setup

### Quick Start (30 seconds)

```bash
# Pull the latest QLever image
docker pull adfreiburg/qlever:latest

# Run QLever with sample data
docker run -p 7023:7023 \
  -v $(pwd)/qlever-data:/data \
  adfreiburg/qlever:latest

# QLever is now running on http://localhost:7023
```

### Using with Custom RDF Data

```bash
# Prepare your data
mkdir -p qlever-data
cp your-data.nt qlever-data/

# Run container with data volume
docker run -p 7023:7023 \
  -v $(pwd)/qlever-data:/data \
  adfreiburg/qlever:latest \
  qlever-build-index /data/your-data.nt

# Start QLever server
docker run -p 7023:7023 \
  -v $(pwd)/qlever-data:/data \
  adfreiburg/qlever:latest \
  qlever-server
```

### Docker Image Variants

```bash
# Latest stable release
docker pull adfreiburg/qlever:latest

# Specific version
docker pull adfreiburg/qlever:v0.2.10

# Latest development build
docker pull adfreiburg/qlever:master

# Commit-specific build
docker pull adfreiburg/qlever:commit-a1b2c3d
```

### Production Configuration

For production use, consider:

```bash
docker run -d \
  --name qlever-server \
  -p 7023:7023 \
  -v /persistent/data:/data \
  --memory 4g \
  --cpus 2 \
  adfreiburg/qlever:latest

# Check logs
docker logs -f qlever-server

# Stop gracefully
docker stop qlever-server
```

---

## Development Container Setup

### Building from Source

```bash
# Clone repository
git clone https://github.com/seanchatmangpt/qlever.git
cd qlever

# Build development image
docker build -t qlever:dev -f Dockerfile .

# Run development container
docker run -it \
  -v $(pwd):/home/qlever \
  -w /home/qlever \
  qlever:dev \
  /bin/bash
```

### Development Workflow

Inside the container:

```bash
# Install dependencies
./scripts/setup-dev-env.sh

# Build
make build

# Run tests
make test

# Run specific test
ctest -R "QueryPlannerTest" -j$(nproc)
```

### Mounting Source for Live Editing

```bash
docker run -it \
  -v $(pwd):/home/qlever \
  -w /home/qlever \
  qlever:dev \
  /bin/bash

# Inside container:
# make build  # Compiles your local changes
# make test   # Runs tests
```

### X11 Forwarding for GUI Tools (Optional)

For running GUI debuggers (CLion with Docker toolchain):

```bash
# Linux
docker run -it \
  -v $(pwd):/home/qlever \
  -w /home/qlever \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  qlever:dev \
  /bin/bash

# macOS (requires XQuartz)
docker run -it \
  -v $(pwd):/home/qlever \
  -w /home/qlever \
  -e DISPLAY=host.docker.internal:0 \
  qlever:dev \
  /bin/bash
```

---

## Docker Compose Multi-Container

### Complete Stack (QLever + Redis + Postgres)

Create `docker-compose.yml`:

```yaml
version: '3.8'

services:
  qlever:
    build: .
    container_name: qlever-server
    ports:
      - "7023:7023"
    volumes:
      - ./qlever-data:/data
      - ./indexes:/indexes
    environment:
      - QLEVER_LOG_LEVEL=INFO
      - QLEVER_MAX_MEMORY=4G
      - QLEVER_CACHE_SIZE=1G
    depends_on:
      - redis
      - postgres
    networks:
      - qlever-network

  redis:
    image: redis:7-alpine
    container_name: qlever-redis
    ports:
      - "6379:6379"
    volumes:
      - redis-data:/data
    networks:
      - qlever-network

  postgres:
    image: postgres:15-alpine
    container_name: qlever-postgres
    environment:
      POSTGRES_DB: qlever
      POSTGRES_USER: qlever
      POSTGRES_PASSWORD: secret
    ports:
      - "5432:5432"
    volumes:
      - postgres-data:/var/lib/postgresql/data
    networks:
      - qlever-network

volumes:
  redis-data:
  postgres-data:

networks:
  qlever-network:
    driver: bridge
```

### Start Stack

```bash
# Build and start all services
docker-compose up -d

# Check logs
docker-compose logs -f qlever

# Stop all services
docker-compose down

# Rebuild after code changes
docker-compose up --build -d
```

### Development Compose File

For development with live code updates:

Create `docker-compose.dev.yml`:

```yaml
version: '3.8'

services:
  qlever-dev:
    build:
      context: .
      dockerfile: Dockerfile.dev
    container_name: qlever-dev
    volumes:
      - .:/home/qlever:delegated
      - /home/qlever/build  # Exclude build dir from sync
    working_dir: /home/qlever
    command: /bin/bash
    tty: true
    stdin_open: true
    networks:
      - qlever-network

networks:
  qlever-network:
    driver: bridge
```

Usage:

```bash
# Start development container
docker-compose -f docker-compose.dev.yml run --rm qlever-dev

# Inside container, run your build/test commands
# make build && make test
```

---

## Dockerfile Reference

### Standard Dockerfile (Multi-stage)

```dockerfile
# Stage 1: Build
FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
  cmake \
  ninja-build \
  g++ \
  clang \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

RUN mkdir -p build && cd build && \
  cmake -GNinja -DCMAKE_BUILD_TYPE=Release .. && \
  cmake --build .

# Stage 2: Runtime
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
  libicu76 \
  libboost-all-dev \
  && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/build/src/server/ServerMain /usr/local/bin/qlever-server
COPY --from=builder /build/build/src/index/IndexBuilderMain /usr/local/bin/qlever-build-index

EXPOSE 7023

ENTRYPOINT ["qlever-server"]
CMD ["--help"]
```

### Development Dockerfile

Create `Dockerfile.dev`:

```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  git \
  clang \
  clang-format \
  clang-tools \
  gdb \
  vim \
  curl \
  python3 \
  python3-pip \
  && rm -rf /var/lib/apt/lists/*

# Install development tools
RUN pip install pre-commit pyaml pyicu

WORKDIR /home/qlever

# Copy and install dependencies
COPY scripts/setup-dev-env.sh .
RUN ./scripts/setup-dev-env.sh

ENTRYPOINT ["/bin/bash"]
```

---

## Docker Networking

### Service Discovery

Services can communicate using service names as hostnames:

```bash
# Inside qlever container, connect to redis
redis-cli -h redis -p 6379

# Inside application code
redis_client = redis.Redis(host='redis', port=6379)
postgres_conn = psycopg2.connect(host='postgres', dbname='qlever')
```

### Expose Ports

```yaml
services:
  qlever:
    ports:
      - "7023:7023"  # Host:Container
    expose:
      - "7023"       # Internal only (for service-to-service)
```

---

## Docker Volumes

### Named Volumes vs Bind Mounts

```yaml
# Named volume (managed by Docker, persistent)
volumes:
  - postgres-data:/var/lib/postgresql/data

# Bind mount (direct host directory)
volumes:
  - /absolute/host/path:/container/path
  - ./relative/path:/container/path
```

### Volume Delegation (macOS Performance)

For better macOS performance, use `delegated` flag:

```yaml
volumes:
  - .:/home/qlever:delegated
```

---

## Performance Tuning

### Memory & CPU Limits

```bash
docker run -d \
  --name qlever \
  --memory 4g \
  --memory-reservation 3g \
  --cpus 4 \
  adfreiburg/qlever:latest
```

### Build Cache Optimization

```dockerfile
# Place frequently-changing commands at end
RUN apt-get install dependencies  # Changes less often
COPY . .                           # Changes frequently
RUN make build                     # Rebuilds if code changed
```

---

## Troubleshooting

### Container Won't Start

```bash
# Check logs
docker logs container-name

# Inspect image
docker inspect adfreiburg/qlever:latest

# Run interactively for debugging
docker run -it adfreiburg/qlever:latest /bin/bash
```

### Out of Memory

```bash
# Increase memory limit
docker update --memory 8g container-name

# Check current usage
docker stats container-name
```

### Slow on macOS

- Use `delegated` flag for volume mounts
- Check `docker stats` for resource limits
- Consider using Lima (lightweight VM) instead of Docker Desktop

### Network Issues

```bash
# Check service connectivity inside container
docker exec container-name ping redis
docker exec container-name redis-cli -h redis ping
```

### Rebuild Required After Code Changes

```bash
# Rebuild image
docker-compose build --no-cache

# Or for single service
docker-compose build --no-cache qlever
```

---

## Security Best Practices

1. **Don't run as root**
   ```dockerfile
   RUN useradd -m -u 1000 qlever
   USER qlever
   ```

2. **Use secret management for credentials**
   ```yaml
   environment:
     POSTGRES_PASSWORD_FILE: /run/secrets/db_password
   secrets:
     db_password:
       file: ./db_password.txt
   ```

3. **Scan for vulnerabilities**
   ```bash
   docker scan adfreiburg/qlever:latest
   trivy image adfreiburg/qlever:latest
   ```

4. **Keep images minimal**
   - Use multi-stage builds
   - Clean up after installations
   - Remove dev dependencies from runtime image

---

## Additional Resources

- [Docker Documentation](https://docs.docker.com/)
- [Docker Compose Reference](https://docs.docker.com/compose/compose-file/)
- [Best Practices for Writing Dockerfiles](https://docs.docker.com/develop/dev-best-practices/)

For development help, see [CONTRIBUTING.md](../CONTRIBUTING.md) or [Quick Start](quick-start.md).
