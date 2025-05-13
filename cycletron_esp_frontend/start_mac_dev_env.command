#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# === Check for Node.js and npm ===
echo "Checking for Node.js and npm..."
if ! command -v node &> /dev/null; then
  echo "Node.js is not installed. Installing Node.js..."
  # Install Node.js using Homebrew
  if command -v brew &> /dev/null; then
    brew install node
  else
    echo "Homebrew is not installed. Please install Homebrew first: https://brew.sh/"
    exit 1
  fi
else
  echo "Node.js is already installed. Version: $(node -v)"
fi

if ! command -v npm &> /dev/null; then
  echo "npm is not installed. Installing npm..."
  # npm comes with Node.js, so this should rarely be needed
  echo "Please check your Node.js installation."
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
open http://localhost:5000
open http://localhost:5174