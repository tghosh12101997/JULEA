# JULEA

[![CI](https://github.com/parcio/julea/actions/workflows/ci.yml/badge.svg)](https://github.com/parcio/julea/actions/workflows/ci.yml)
[![Dependencies](https://github.com/parcio/julea/actions/workflows/dependencies.yml/badge.svg)](https://github.com/parcio/julea/actions/workflows/dependencies.yml)
[![Containers](https://github.com/parcio/julea/actions/workflows/containers.yml/badge.svg)](https://github.com/parcio/julea/actions/workflows/containers.yml)

## Objective
The objective of this project was to implement a new database solution and assess its performance 
using tail latency as a key metric.

## Implementation
For this task, I have introduced PostgreSQL 10.0 and Redis 5.0 as the new database solutions. 
These choices were made based on their respective strengths and suitability for our project requirements.

fedora-38-dev
```console
FROM fedora:38

# FIXME hostname is required for hostname (julea-config)
# FIXME psmisc is required for killall (setup.sh)
RUN dnf --refresh --assumeyes install gcc libasan libubsan hostname psmisc meson ninja-build pkgconf glib2-devel libbson-devel libfabric-devel lmdb-devel sqlite-devel leveldb-devel mongo-c-driver-devel mariadb-connector-c-devel rocksdb-devel fuse3-devel librados-devel hiredis-devel postgresql-devel

WORKDIR /julea
```

ubuntu-22.04-dev
```console
FROM ubuntu:22.04

# FIXME psmisc is required for killall (setup.sh)
RUN apt update && apt --yes --no-install-recommends install build-essential psmisc meson ninja-build pkgconf libglib2.0-dev libbson-dev libfabric-dev libgdbm-dev liblmdb-dev libsqlite3-dev libleveldb-dev libmongoc-dev libmariadb-dev librocksdb-dev libfuse3-dev libopen-trace-format-dev librados-dev libhiredis-dev libpq-dev

WORKDIR /julea
```

### Performance Evaluation
To evaluate the performance of the newly implemented databases, a comparative analysis was conducted with 
previously utilized solutions, namely SQLite and MySQL. The figures below depict the updated performance comparison:

Throughput Comparison:

```console
throughput1.png
```

Latency Comparison:

```console
latency1.png
```

These visualizations offer insights into the efficiency and effectiveness of each database solution, 
aiding in informed decision-making for our project.

### Redis Performance Analysis
Upon observation, it is notable that Redis exhibits lower throughput compared to other database solutions. 
However, the latency associated with Redis is significantly higher. It's important to note that this 
phenomenon is not indicative of an inherent problem with Redis itself.

Redis operates as an in-memory data store, where all data is stored in RAM. This design facilitates 
remarkably fast read and write operations, as accessing RAM is considerably faster than accessing disk. 
However, this architecture also imposes limitations, particularly concerning available memory. 
Consequently, Redis's throughput can be constrained by the speed at which data can be processed in memory.

The figures presented in the generated PNG outputs clearly illustrate this aspect. The elevated latency 
observed with Redis can be attributed to the potential limitations in my system memory availability. 
Further optimization or allocation of sufficient memory resources could potentially mitigate this latency issue.

## Tail Latency Calculation

One of the challenges encountered during our performance analysis was the difficulty in obtaining individual time elapsed for operations, necessary for calculating tail latency. The storage framework utilized provided only average time measurements, rendering it inadequate for our specific needs.

In an attempt to address this issue, we explored various strategies, including modifying benchmarks to capture individual operation times. However, these attempts proved to be ineffective in providing accurate individual time data.

For further examination and potential solutions, the code pertaining to the attempted benchmark modifications can be found in the benchmark-change folder.

This challenge underscores the importance of selecting appropriate tools and methodologies tailored to the specific requirements of performance analysis, especially when aiming to capture metrics such as tail latency accurately.









