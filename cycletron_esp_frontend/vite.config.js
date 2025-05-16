import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    host: true,       // expose to local network
    port: 5174,       // fixed port for frontend dev server
    proxy: {
      '/api': 'http://localhost:5175',      // Proxy REST API calls
      '/ws': {
        target: 'ws://localhost:5175',      // Proxy websocket path
        ws: true,
      },
      '/socket': {
        target: 'ws://localhost:5175',
        ws: true,
      }
    }
  }
});
