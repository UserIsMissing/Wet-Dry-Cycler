@echo off
setlocal

REM === Frontend Dependencies ===
echo Checking frontend dependencies...
if exist node_modules (
  echo Frontend node_modules found, skipping install
) else (
  echo Frontend node_modules not found, running npm install...
  npm install
)

REM Ensure Bulma is installed
echo Checking if Bulma is installed...
findstr /C:"\"bulma\"" package.json >nul
if %errorlevel% neq 0 (
  echo Bulma not found in dependencies, installing Bulma...
  npm install bulma
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

REM === Start Backend Server with nodemon (using nodemon.json config) ===
echo Starting backend server with nodemon on port 5175...
start "Backend" cmd /k "cd server && nodemon"

REM === Start Frontend Vite Dev Server ===
echo Starting frontend (Vite) on default port...
start "Frontend" cmd /k npm run dev

REM === Open in Browser ===
timeout /t 5 >nul
start http://localhost:5175
start http://localhost:5174

endlocal
