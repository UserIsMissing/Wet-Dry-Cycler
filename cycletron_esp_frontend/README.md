## Cole Schreiner
# 5/15/2025

Adding several files to deal with file storage, crash recovery, page reloads, power loss, etc.

Added the following files to base of CYCLETRON_ESP_FRONTEND
- gpio_state.json
- recovery_state.json
- esp_data.db (Although that was here before, I was told its for recovery possibly relating to temperature data?)
- src/hooks/useWebSocket.js (This is a dedicated file or hook for managing the WebSocket connection and handling messages)


# React + Vite

This template provides a minimal setup to get React working in Vite with HMR and some ESLint rules.

Currently, two official plugins are available:

- [@vitejs/plugin-react](https://github.com/vitejs/vite-plugin-react/blob/main/packages/plugin-react) uses [Babel](https://babeljs.io/) for Fast Refresh
- [@vitejs/plugin-react-swc](https://github.com/vitejs/vite-plugin-react/blob/main/packages/plugin-react-swc) uses [SWC](https://swc.rs/) for Fast Refresh

## Expanding the ESLint configuration

If you are developing a production application, we recommend using TypeScript with type-aware lint rules enabled. Check out the [TS template](https://github.com/vitejs/vite/tree/main/packages/create-vite/template-react-ts) for information on how to integrate TypeScript and [`typescript-eslint`](https://typescript-eslint.io) in your project.
