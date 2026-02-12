#!/bin/bash
# Test suite for sql-break
set -e

BINARY="./build/breaksql"
TEST_DIR="$(mktemp -d)"
PASS=0
FAIL=0

cleanup() {
    rm -rf "$TEST_DIR"
}
trap cleanup EXIT

assert_file_exists() {
    if [ ! -f "$1" ]; then
        echo "FAIL: Expected file $1 to exist"
        FAIL=$((FAIL + 1))
        return 1
    fi
    return 0
}

assert_file_not_exists() {
    if [ -f "$1" ]; then
        echo "FAIL: Expected file $1 to NOT exist"
        FAIL=$((FAIL + 1))
        return 1
    fi
    return 0
}

assert_file_contains() {
    if ! grep -qF "$2" "$1" 2>/dev/null; then
        echo "FAIL: Expected file $1 to contain: $2"
        FAIL=$((FAIL + 1))
        return 1
    fi
    return 0
}

assert_file_not_contains() {
    if grep -qF "$2" "$1" 2>/dev/null; then
        echo "FAIL: Expected file $1 to NOT contain: $2"
        FAIL=$((FAIL + 1))
        return 1
    fi
    return 0
}

pass() {
    echo "PASS: $1"
    PASS=$((PASS + 1))
}

# ------------------------------------------------------------------
# Test 1: Basic splitting with two tables
# ------------------------------------------------------------------
echo "--- Test 1: Basic two-table split ---"
INPUT="$TEST_DIR/dump1.sql"
cat > "$INPUT" <<'EOF'
-- Table structure for table `users`

DROP TABLE IF EXISTS `users`;
CREATE TABLE `users` (id INT);
INSERT INTO `users` VALUES (1),(2),(3);

-- Table structure for table `orders`

DROP TABLE IF EXISTS `orders`;
CREATE TABLE `orders` (id INT);
INSERT INTO `orders` VALUES (10),(20);
EOF

$BINARY "$INPUT"

if assert_file_exists "${INPUT}_000.sql" && \
   assert_file_exists "${INPUT}_001.sql" && \
   assert_file_contains "${INPUT}_000.sql" "users" && \
   assert_file_contains "${INPUT}_001.sql" "orders" && \
   assert_file_not_contains "${INPUT}_000.sql" "orders" && \
   assert_file_not_contains "${INPUT}_001.sql" "users"; then
    pass "Basic two-table split"
fi

# ------------------------------------------------------------------
# Test 2: Preamble content is preserved
# ------------------------------------------------------------------
echo "--- Test 2: Preamble preservation ---"
INPUT="$TEST_DIR/dump2.sql"
cat > "$INPUT" <<'EOF'
-- MySQL dump 10.13
-- Server version 8.0.30

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;

-- Table structure for table `items`

CREATE TABLE `items` (id INT);
EOF

$BINARY "$INPUT"

if assert_file_exists "${INPUT}_000.sql" && \
   assert_file_exists "${INPUT}_001.sql" && \
   assert_file_contains "${INPUT}_000.sql" "MySQL dump" && \
   assert_file_contains "${INPUT}_000.sql" "CHARACTER_SET_CLIENT" && \
   assert_file_contains "${INPUT}_001.sql" "items"; then
    pass "Preamble content preserved"
fi

# ------------------------------------------------------------------
# Test 3: Constraint headers and footers present
# ------------------------------------------------------------------
echo "--- Test 3: Constraint headers/footers ---"
INPUT="$TEST_DIR/dump3.sql"
cat > "$INPUT" <<'EOF'
-- Table structure for table `t1`
CREATE TABLE `t1` (id INT);
EOF

$BINARY "$INPUT"

if assert_file_exists "${INPUT}_000.sql" && \
   assert_file_contains "${INPUT}_000.sql" "SET @OLD_UNIQUE_CHECKS" && \
   assert_file_contains "${INPUT}_000.sql" "SET @OLD_FOREIGN_KEY_CHECKS" && \
   assert_file_contains "${INPUT}_000.sql" "SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS" && \
   assert_file_contains "${INPUT}_000.sql" "SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS"; then
    pass "Constraint headers and footers present"
fi

# ------------------------------------------------------------------
# Test 4: No false positive from marker text inside data
# ------------------------------------------------------------------
echo "--- Test 4: No false positive from data containing marker ---"
INPUT="$TEST_DIR/dump4.sql"
cat > "$INPUT" <<'EOF'
-- Table structure for table `logs`

CREATE TABLE `logs` (msg TEXT);
INSERT INTO `logs` VALUES ('This log says -- Table structure for table inside a string');
INSERT INTO `logs` VALUES ('normal data');

-- Table structure for table `events`

CREATE TABLE `events` (id INT);
EOF

$BINARY "$INPUT"

# Should produce exactly 2 files: logs and events
if assert_file_exists "${INPUT}_000.sql" && \
   assert_file_exists "${INPUT}_001.sql" && \
   assert_file_not_exists "${INPUT}_002.sql" && \
   assert_file_contains "${INPUT}_000.sql" "logs" && \
   assert_file_contains "${INPUT}_001.sql" "events"; then
    pass "No false positive from marker text inside data"
fi

# ------------------------------------------------------------------
# Test 5: Single table (no splitting needed)
# ------------------------------------------------------------------
echo "--- Test 5: Single table ---"
INPUT="$TEST_DIR/dump5.sql"
cat > "$INPUT" <<'EOF'
-- Table structure for table `only_table`

CREATE TABLE `only_table` (id INT);
INSERT INTO `only_table` VALUES (1);
EOF

$BINARY "$INPUT"

if assert_file_exists "${INPUT}_000.sql" && \
   assert_file_not_exists "${INPUT}_001.sql" && \
   assert_file_contains "${INPUT}_000.sql" "only_table"; then
    pass "Single table handled correctly"
fi

# ------------------------------------------------------------------
# Test 6: Many tables (tests 3-digit numbering)
# ------------------------------------------------------------------
echo "--- Test 6: Many tables ---"
INPUT="$TEST_DIR/dump6.sql"
> "$INPUT"
for i in $(seq 1 15); do
    printf -- "-- Table structure for table \`t%d\`\nCREATE TABLE \`t%d\` (id INT);\n\n" "$i" "$i" >> "$INPUT"
done

$BINARY "$INPUT"

if assert_file_exists "${INPUT}_000.sql" && \
   assert_file_exists "${INPUT}_014.sql" && \
   assert_file_not_exists "${INPUT}_015.sql" && \
   assert_file_contains "${INPUT}_000.sql" "t1" && \
   assert_file_contains "${INPUT}_014.sql" "t15"; then
    pass "Many tables with 3-digit numbering"
fi

# ------------------------------------------------------------------
# Test 7: Empty input file
# ------------------------------------------------------------------
echo "--- Test 7: Empty input ---"
INPUT="$TEST_DIR/dump7.sql"
> "$INPUT"

$BINARY "$INPUT"

if assert_file_not_exists "${INPUT}_000.sql"; then
    pass "Empty input produces no output"
fi

# ------------------------------------------------------------------
# Test 8: Usage message on no arguments
# ------------------------------------------------------------------
echo "--- Test 8: Usage message ---"
if $BINARY 2>&1 | grep -q "Usage:"; then
    pass "Usage message displayed"
else
    echo "FAIL: No usage message displayed"
    FAIL=$((FAIL + 1))
fi

# ------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------
echo ""
echo "================================"
echo "Results: $PASS passed, $FAIL failed"
echo "================================"

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
