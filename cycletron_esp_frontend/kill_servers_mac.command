#!/bin/bash

echo "Checking for processes using port 5175..."
PID=$(lsof -ti:5175)

if [ ! -z "$PID" ]; then
  echo "Found process $PID using port 5175. Killing it..."
  kill -9 $PID
  echo "Process killed."
else
  echo "No process found using port 5175."
fi

echo "Checking for processes using port 5174..."
PID=$(lsof -ti:5174)

if [ ! -z "$PID" ]; then
  echo "Found process $PID using port 5174. Killing it..."
  kill -9 $PID
  echo "Process killed."
else
  echo "No process found using port 5174."
fi

echo "All ports cleared."
