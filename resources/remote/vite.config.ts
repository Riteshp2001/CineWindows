// Copyright (c) 2026 Ritesh Pandit
// Last modified: 2026-09-10
// Modified by: Ritesh Pandit

import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  base: './',
  plugins: [react()],
  build: {
    outDir: 'dist',
    emptyOutDir: true,
    target: 'es2020',
    cssCodeSplit: false,
    rollupOptions: {
      output: {
        entryFileNames: 'companion.js',
        assetFileNames: 'companion.[ext]',
      },
    },
  },
})
