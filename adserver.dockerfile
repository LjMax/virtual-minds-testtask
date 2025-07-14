FROM ubuntu:20.04 as builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY adserver.cpp CMakeLists.txt ./

# Build the application
RUN cmake . && make -j$(nproc)

# Runtime stage
FROM ubuntu:20.04

# Set noninteractive frontend to prevent prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libboost-system1.71.0 \
    libboost-thread1.71.0 \
    dnsutils \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the compiled binary from builder stage
COPY --from=builder /app/adserver .

# Expose port
EXPOSE 80

# Run the application
CMD ["./adserver"]
