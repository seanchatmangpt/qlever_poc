#!/bin/bash

echo "=== QLever JavaScript Integration Test Suite ==="
echo ""

# Check if server is running
echo "Checking if QLever server is running on http://localhost:3000..."
if ! curl -s http://localhost:3000/health > /dev/null 2>&1; then
  echo ""
  echo "✗ Server is not running. Starting server..."
  echo ""
  echo "To start the server, run in another terminal:"
  echo "  cd /home/user/qlever/js"
  echo "  node server.js"
  echo ""
  exit 1
fi

echo "✓ Server is running"
echo ""

# Run the integration tests
echo "Running integration tests..."
echo ""

node integration-test.js
TEST_RESULT=$?

exit $TEST_RESULT
