#!/bin/bash
set -e

BACKENDS=("mysql" "sqlite" "postgres" "redis")

configure_backend() {
	local backend=$1
	case "${backend}" in
		sqlite)
			julea-config --user --db-backend=sqlite --db-path="/tmp/julea-$(id -u)/sqlite"
			;;
		mysql)
			julea-config --user --db-backend=mysql --db-path="/tmp/julea-$(id -u)/mysql"
			;;
		postgres)
			julea-config --user --db-backend=postgres --db-path="/tmp/julea-$(id -u)/postgres"
			;;
		redis)
			julea-config --user --db-backend=redis --db-path="/tmp/julea-$(id -u)/redis"
			;;
		*)
			echo "Unknown backend: $backend"
			exit 1
			;;
	esac
}

for BACKEND in "${BACKENDS[@]}"; do
  echo "Benchmarking $BACKEND backend"
  ./scripts/setup.sh start $BACKEND
  ./scripts/benchmark.sh --machine-readable --machine-separator=, > "result_${BACKEND}.csv"
  ./scripts/setup.sh stop $BACKEND
  echo "Completed benchmarking for $BACKEND backend"
  echo "------------------------------------------"
done
