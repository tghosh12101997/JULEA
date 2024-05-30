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





JULEA is a flexible storage framework that allows offering arbitrary I/O interfaces to applications.
To be able to rapidly prototype new approaches, it offers object, key-value and database backends.
Support for popular storage technologies such as POSIX, LevelDB and MongoDB is already included.

Additionally, JULEA allows dynamically adapting the I/O operations' semantics and can thus be adjusted to different use-cases.
It runs completely in user space, which eases development and debugging.
Its goal is to provide a solid foundation for storage research and teaching.

For more information, please refer to the [documentation](doc/README.md).
There is also a separate [API documentation](https://parcio.github.io/julea/) available.

## Quick Start

To use JULEA, first clone the Git repository and enter the directory.

```console
$ git clone https://github.com/parcio/julea.git
$ cd julea
```

JULEA has three mandatory dependencies (GLib, libbson and libfabric) and several optional ones that enable additional functionality.
The dependencies can either be installed using [your operating system's package manager](doc/dependencies.md#manual-installation) or with JULEA's `install-dependencies` script that installs them into the `dependencies` subdirectory using [Spack](https://spack.io/).

```console
$ ./scripts/install-dependencies.sh
```

To allow the dependencies to be found, the JULEA environment has to be loaded.
This also ensures that JULEA's binaries and libraries are found later.
Make sure to load the script using `. ` instead of executing it with `./` because the environment changes will not persist otherwise.

```console
$ . scripts/environment.sh
```

JULEA now has to be configured using [Meson](https://mesonbuild.com/) and compiled using [Ninja](https://ninja-build.org/);
the different configuration and build options can be shown with `meson setup --help`.

```console
$ meson setup --prefix="${HOME}/julea-install" -Db_sanitize=address,undefined bld
$ ninja -C bld
```

Finally, a JULEA configuration has to be created.

```console
$ julea-config --user \
  --object-servers="$(hostname)" --kv-servers="$(hostname)" --db-servers="$(hostname)" \
  --object-backend=posix --object-path="/tmp/julea-$(id -u)/posix" \
  --kv-backend=lmdb --kv-path="/tmp/julea-$(id -u)/lmdb" \
  --db-backend=sqlite --db-path="/tmp/julea-$(id -u)/sqlite"
```

You can check whether JULEA works by executing the integrated test suite.

```console
$ ./scripts/setup.sh start
$ ./scripts/test.sh
$ ./scripts/setup.sh stop
```

To get an idea about how to use JULEA from your own application, check out the `example` directory.

```console
$ cd example
$ make run
```

The version is JULEA built using this guide is mainly meant for development and debugging.
If you want to deploy a release version of JULEA, please refer to the documentation about [installation and usage](doc/installation-usage.md).

## Citing JULEA

If you want to cite JULEA, please use the following paper:

- [JULEA: A Flexible Storage Framework for HPC](https://doi.org/10.1007/978-3-319-67630-2_51) (Michael Kuhn), In High Performance Computing, Lecture Notes in Computer Science (10524), (Editors: Julian Kunkel, Rio Yokota, Michela Taufer, John Shalf), Springer International Publishing, ISC High Performance 2017, Frankfurt, Germany, ISBN: 978-3-319-67629-6, 2017-11
