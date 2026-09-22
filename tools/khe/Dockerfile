# Dockerfile for building Jeffrey Kingston's KHE timetabling suite & HSEval
# Multi-stage build for minimal image size

FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    make \
    gcc \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY Makefile ./
COPY khe-2025_12_04/ ./khe-2025_12_04/

RUN make build

# Final lightweight runner image
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libc6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/bin/hseval /usr/local/bin/hseval
COPY --from=builder /build/bin/khe /usr/local/bin/khe

# Domyślnie uruchamia hseval z czytaniem z stdin
ENTRYPOINT ["hseval"]
CMD ["-tc", "/dev/stdin"]
