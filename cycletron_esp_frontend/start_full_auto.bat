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
  if %errorlevel% neq 0 (
    echo ERROR: Frontend npm install failed!
    pause
    exit /b 1
  )
) else (
  echo Installing frontend dependencies for the first time...
  call npm install
  if %errorlevel% neq 0 (
    echo ERROR: Frontend npm install failed!
    pause
    exit /b 1
  )
)

REM Ensure Vite is installed
echo Checking if Vite is installed...
call npm list vite || (
  echo Installing Vite...
  call npm install vite
  if %errorlevel% neq 0 (
    echo ERROR: Vite install failed!
    pause
    exit /b 1
  )
)

REM Ensure Bulma is installed
echo Checking if Bulma is installed...
findstr /C:"\"bulma\"" package.json >nul
if %errorlevel% neq 0 (
  echo Bulma not found in dependencies, installing Bulma...
  call npm install bulma
  if %errorlevel% neq 0 (
    echo ERROR: Bulma install failed!
    pause
    exit /b 1
  )
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
  if %errorlevel% neq 0 (
    echo ERROR: Backend npm install failed!
    pause
    exit /b 1
  )
)

REM Check if server.js exists
if not exist server.js (
  echo ERROR: server.js not found in server directory!
  echo Current directory: %cd%
  dir
  pause
  exit /b 1
)

REM === Determine how to run the server ===
set NODEMON_CMD=

REM Check if nodemon is installed globally
where nodemon >nul 2>nul
if %errorlevel% equ 0 (
  echo nodemon is installed globally.
  set NODEMON_CMD=nodemon
  goto start_server
)

REM Check if nodemon is installed locally
if exist node_modules\.bin\nodemon.cmd (
  echo nodemon is installed locally.
  set NODEMON_CMD=npx nodemon
  goto start_server
)

REM Try to install nodemon globally
echo nodemon is not installed. Trying to install globally...
npm install -g nodemon >nul 2>nul
if %errorlevel% equ 0 (
  echo nodemon installed globally successfully.
  set NODEMON_CMD=nodemon
  goto start_server
)

REM Try to install nodemon locally
echo Global install failed. Trying to install locally...
npm install nodemon --save-dev >nul 2>nul
if %errorlevel% equ 0 (
  echo nodemon installed locally successfully.
  set NODEMON_CMD=npx nodemon
  goto start_server
)

REM Fall back to regular node
echo WARNING: Could not install nodemon. Using regular node (no auto-restart).
set NODEMON_CMD=node

:start_server
cd ..

REM === Start Backend Server ===
echo Starting backend server on port 5175...
echo Using command: %NODEMON_CMD%
if "%NODEMON_CMD%"=="node" (
  start "Backend" cmd /k "echo Starting backend with node... && cd /d %cd% && node server/server.js"
) else (
  start "Backend" cmd /k "echo Starting backend with %NODEMON_CMD%... && cd /d %cd% && %NODEMON_CMD% server/server.js --ignore Frontend_Recovery.json"
)

REM Wait a bit for backend to start
echo Waiting for backend to start...
timeout /t 8 >nul

REM === Start Frontend Vite Dev Server ===
echo Starting frontend (Vite) on default port...
start "Frontend" cmd /k "echo Starting frontend... && cd /d %cd% && npm run dev"

REM === Wait before opening browser ===
echo Waiting for services to fully start...
timeout /t 10 >nul

REM Check if ports are actually listening before opening browser
echo Checking if services are running...
netstat -an | findstr :5174 >nul
if %errorlevel% equ 0 (
  echo Frontend is running on port 5174
) else (
  echo WARNING: Frontend may not be running on port 5174
)

netstat -an | findstr :5175 >nul
if %errorlevel% equ 0 (
  echo Backend is running on port 5175
) else (
  echo WARNING: Backend may not be running on port 5175
)

start http://localhost:5174
start http://localhost:5175

echo All services should be started. Check the Backend and Frontend windows for any error messages.
pause
endlocal