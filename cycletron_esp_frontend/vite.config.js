import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    host: true,       // exposes the server to your local network
    port: 5174,       // optional, makes sure it's always the same port
  }
});