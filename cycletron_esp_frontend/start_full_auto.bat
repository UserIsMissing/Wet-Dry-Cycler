@echo off
setlocal

REM === Kill existing processes on ports ===
echo Checking for processes using ports 5174 and 5175...
powershell -Command "Get-NetTCPConnection -LocalPort 5174,5175 -ErrorAction SilentlyContinue | ForEach-Object { Stop-Process -Id $_.OwningProcess -Force -ErrorAction SilentlyContinue }"
echo Cleaned up any existing processes on ports 5174 and 5175
timeout /t 2 >nul

REM === Frontend Dependencies ===
echo Checking frontend dependencies...

REM Check if package-lock.json exists (indicates npm was run before)
if exist package-lock.json (
  echo Updating frontend dependencies...
  call npm install
) else (
  echo Installing frontend dependencies for the first time...
  call npm install
)

REM Ensure Vite is installed
echo Checking if Vite is installed...
call npm list vite || (
  echo Installing Vite...
  call npm install vite
)

REM Ensure Bulma is installed
echo Checking if Bulma is installed...
findstr /C:"\"bulma\"" package.json >nul
if %errorlevel% neq 0 (
  echo Bulma not found in dependencies, installing Bulma...
  call npm install bulma
) else (
  echo Bulma is already installed.
)

REM === Backend Dependencies ===
echo Checking backend dependencies...
cd server
if exist node_modules (
  echo Backend node_modules found, skipping install
) else (
  echo Backend node_modules not found, running npm install...
  npm install
)

REM Ensure nodemon is installed globally
where nodemon >nul 2>nul
if %errorlevel% neq 0 (
  echo nodemon is not installed globally. Installing it...
  npm install -g nodemon
) else (
  echo nodemon is already installed.
)
cd ..

REM === Start Backend Server with nodemon (ignoring Frontend_Recovery.json) ===
echo Starting backend server with nodemon on port 5175...
start "Backend" cmd /k nodemon server/server.js --ignore Frontend_Recovery.json

REM === Start Frontend Vite Dev Server ===
echo Starting frontend (Vite) on default port...
start "Frontend" cmd /k npm run dev

REM === Open in Browser ===
timeout /t 5 >nul
start http://localhost:5175
start http://localhost:5174

endlocal