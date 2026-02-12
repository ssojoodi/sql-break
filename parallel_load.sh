#!/usr/bin/env bash
set -euo pipefail

usage() { echo "Usage: $0 <dumpfile> <db> [-j N] [-h host] [-u user] [-p pass]" >&2; exit 1; }

[[ $# -lt 2 ]] && usage

DUMPFILE="$1"; DB="$2"; shift 2
JOBS=4 HOST=localhost USER=root PASS=""

while getopts "j:h:u:p:" opt; do
  case $opt in
    j) JOBS="$OPTARG";;
    h) HOST="$OPTARG";;
    u) USER="$OPTARG";;
    p) PASS="$OPTARG";;
    *) usage;;
  esac
done

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

# Split dump into chunks inside temp dir
cp "$DUMPFILE" "$TMPDIR/dump.sql"
(cd "$TMPDIR" && breaksql dump.sql)
rm "$TMPDIR/dump.sql"

MYSQL_OPTS="-h $HOST -u $USER ${PASS:+-p$PASS} $DB"

# Load preamble/first table synchronously
echo "Loading dump.sql_000.sql (preamble)..."
mysql $MYSQL_OPTS < "$TMPDIR/dump.sql_000.sql"

# Load remaining chunks in parallel
find "$TMPDIR" -name 'dump.sql_*.sql' ! -name 'dump.sql_000.sql' -print0 | sort -z | \
  xargs -0 -P "$JOBS" -I{} sh -c 'echo "Loading $(basename {})..." && mysql '"$MYSQL_OPTS"' < {}'

echo "Done. All chunks loaded."
