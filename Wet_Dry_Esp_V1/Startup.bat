@echo off
REM Setup script for Wet Dry Cycling project

echo Starting backend server...
start cmd /k "cd Node_React_Server\server && node server.js"

echo Starting frontend client...
start cmd /k "cd Node_React_Server\client && npm start"