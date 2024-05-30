#!/bin/bash

# JULEA - Flexible storage framework
# Copyright (C) 2013-2024 Michael Kuhn
# Copyright (C) 2013 Anna Fuchs
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

#!/bin/bash
#!/bin/bash
set -e

SELF_PATH="$(readlink --canonicalize-existing -- "$0")"
SELF_DIR="${SELF_PATH%/*}"
SELF_BASE="${SELF_PATH##*/}"

# shellcheck source=scripts/common
. "${SELF_DIR}/common"
# shellcheck source=scripts/setup
. "${SELF_DIR}/setup"
# shellcheck source=scripts/spack
. "${SELF_DIR}/spack"

usage ()
{
	echo "Usage: ${SELF_BASE} start|stop|restart [backend]"
	echo "Backends: sqlite, postgres, redis"
	exit 1
}

setup_slurm ()
{
	local mode
	local nodes

	mode="$1"

	test -n "${mode}" || return 1

	if test -n "${SLURM_JOB_NODELIST}"
	then
		nodes="$(scontrol show hostnames)"

		for node in ${nodes}
		do
			# shellcheck disable=SC2029
			ssh "${node}" "${SELF_PATH}" "${mode}-local"
		done

		return 0
	fi

	return 1
}

test -n "$1" || usage

MODE="$1"
BACKEND="$2"

# Set default backend if not provided
if test -z "${BACKEND}"
then
	BACKEND="sqlite"
fi

#export G_MESSAGES_DEBUG=JULEA

set_path
set_library_path
set_backend_path

if test -z "${JULEA_SPACK_DIR}"
then
	JULEA_SPACK_DIR="$(get_directory "${SELF_DIR}/..")/dependencies"
fi

spack_load_dependencies

setup_init

configure_backend() {
	local backend=$1
	case "${backend}" in
		sqlite)
			julea-config --user \
				--object-servers="$(hostname)" --kv-servers="$(hostname)" --db-servers="$(hostname)" \
				--object-backend=posix --object-path="/tmp/julea-$(id -u)/posix" \
				--kv-backend=lmdb --kv-path="/tmp/julea-$(id -u)/lmdb" \
				--db-backend=sqlite --db-path="/tmp/julea-$(id -u)/sqlite"
			;;
		mysql)
			julea-config --user \
				--object-servers="$(hostname)" --kv-servers="$(hostname)" --db-servers="$(hostname)" \
				--object-backend=posix --object-path="/tmp/julea-$(id -u)/posix" \
				--kv-backend=lmdb --kv-path="/tmp/julea-$(id -u)/lmdb" \
				--db-backend=sqlite --db-path="/tmp/julea-$(id -u)/mysql"
			;;
		postgres)
			julea-config --user \
				--object-servers="$(hostname)" --kv-servers="$(hostname)" --db-servers="$(hostname)" \
				--object-backend=posix --object-path="/tmp/julea-$(id -u)/posix" \
				--kv-backend=lmdb --kv-path="/tmp/julea-$(id -u)/lmdb" \
				--db-backend=postgres --db-path="/tmp/julea-$(id -u)/postgres"
			;;
		redis)
			julea-config --user \
				--object-servers="$(hostname)" --kv-servers="$(hostname)" --db-servers="$(hostname)" \
				--object-backend=posix --object-path="/tmp/julea-$(id -u)/posix" \
				--kv-backend=lmdb --kv-path="/tmp/julea-$(id -u)/lmdb" \
				--db-backend=redis --db-path="/tmp/julea-$(id -u)/redis"
			;;
		*)
			echo "Unknown backend: $backend"
			exit 1
			;;
	esac
}

case "${MODE}" in
	start)
		configure_backend "${BACKEND}"
		if ! setup_slurm "${MODE}"
		then
			setup_start
		fi
		;;
	start-local)
		setup_start
		;;
	stop)
		if ! setup_slurm "${MODE}"
		then
			setup_stop
		fi
		;;
	stop-local)
		setup_stop
		;;
	clean)
		if ! setup_slurm "${MODE}"
		then
			setup_clean
		fi
		;;
	clean-local)
		setup_clean
		;;
	restart)
		configure_backend "${BACKEND}"
		if ! setup_slurm stop
		then
			setup_stop
		fi

		sleep 10

		if ! setup_slurm start
		then
			setup_start
		fi
		;;
	*)
		usage
		;;
esac
