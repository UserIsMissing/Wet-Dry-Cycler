#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# === Check for Node.js and npm ===
echo "Checking for Node.js and npm..."
if ! command -v node &> /dev/null; then
  echo "Node.js is not installed. Installing Node.js..."
  # Download and install Node.js directly
  NODE_VERSION="18.17.1" # Specify the desired Node.js version
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
cd "$SCRIPT_DIR"

# === Start Backend Server ===
echo "Starting backend server on port 5000..."
osascript -e 'tell app "Terminal" to do script "cd \"'"$SCRIPT_DIR"'/server\" && node server.js"'

# === Start Frontend Vite Dev Server ===
echo "Starting frontend (Vite) on default port..."
osascript -e 'tell app "Terminal" to do script "cd \"'"$SCRIPT_DIR"'\" && npm run dev"'

# === Open in Browser ===
sleep 5
# echo "Opening backend WS server in browser..."
# open http://localhost:5175
echo "Opening frontend in browser..."
open http://localhost:5174