#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# === Check for Node.js and npm ===
echo "Checking for Node.js and npm..."
if ! command -v node &> /dev/null; then
  echo "Node.js is not installed. Installing Node.js..."
  NODE_VERSION="18.17.1"
  NODE_DISTRO="node-v$NODE_VERSION-darwin-x64"
  curl -O "https://nodejs.org/dist/v$NODE_VERSION/$NODE_DISTRO.tar.gz"
  tar -xzf "$NODE_DISTRO.tar.gz"
  mv "$NODE_DISTRO" "$SCRIPT_DIR/node"
  export PATH="$SCRIPT_DIR/node/bin:$PATH"
  echo "Node.js installed. Version: $(node -v)"
else
  echo "Node.js is already installed. Version: $(node -v)"
fi

if ! command -v npm &> /dev/null; then
  echo "npm is not installed. Please check your Node.js installation."
  exit 1
else
  echo "npm is already installed. Version: $(npm -v)"
fi

# === Frontend Dependencies ===
echo "Checking frontend dependencies..."
if [ -d "node_modules" ]; then
  echo "Frontend node_modules found, skipping install"
else
  echo "Frontend node_modules not found, running npm install..."
  npm install
fi

# Ensure Bulma is installed
if ! grep -q '"bulma"' package.json; then
  echo "Bulma not found in dependencies, installing Bulma..."
  npm install bulma
else
  echo "Bulma is already installed."
fi

# === Backend Dependencies ===
echo "Checking backend dependencies..."
cd "$SCRIPT_DIR/server"
if [ -d "node_modules" ]; then
  echo "Backend node_modules found, skipping install"
else
  echo "Backend node_modules not found, running npm install..."
  npm install
fi

# Ensure nodemon is installed globally
if ! command -v nodemon &> /dev/null; then
  echo "nodemon is not installed. Installing globally..."
  npm install -g nodemon
else
  echo "nodemon is already installed. Version: $(nodemon -v)"
fi
cd "$SCRIPT_DIR"

# === Kill any existing servers ===
echo "Checking for existing server processes..."
PID_5175=$(lsof -ti:5175 2>/dev/null)
PID_5174=$(lsof -ti:5174 2>/dev/null)

if [ ! -z "$PID_5175" ]; then
  echo "Killing existing process on port 5175 (PID: $PID_5175)..."
  kill -9 $PID_5175
fi

if [ ! -z "$PID_5174" ]; then
  echo "Killing existing process on port 5174 (PID: $PID_5174)..."
  kill -9 $PID_5174
fi

# === Start Backend Server with nodemon (ignoring recovery files) ===
echo "Starting backend server with nodemon on port 5175..."
osascript -e 'tell app "Terminal" to do script "cd \"'"$SCRIPT_DIR"'/server\" && nodemon server.js --ignore Frontend_Recovery.json --ignore ESP_Recovery.json"'

# === Start Frontend Vite Dev Server ===
echo "Starting frontend (Vite) on default port..."
osascript -e 'tell app "Terminal" to do script "cd \"'"$SCRIPT_DIR"'\" && npm run dev"'

# === Open in Browser ===
sleep 5
echo "Opening frontend and backend in browser..."
open http://localhost:5175
open http://localhost:5174
