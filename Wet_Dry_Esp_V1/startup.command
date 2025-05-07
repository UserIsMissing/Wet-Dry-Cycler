#!/bin/bash
# .command script for Wet Dry Cycling project (relative paths)

cd "$(dirname "$0")"

echo "Starting backend server..."
cd Node_React_Server/server
node server.js &

echo "Starting frontend client..."
cd ../client
npm install
npm start
