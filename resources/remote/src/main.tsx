// Copyright (c) 2026 Ritesh Pandit
// Last modified: 2026-09-10
// Modified by: Ritesh Pandit

import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { SolarProvider } from '@solar-icons/react'
import '@material/web/button/filled-button.js'
import '@material/web/button/filled-tonal-button.js'
import '@material/web/button/outlined-button.js'
import '@material/web/iconbutton/icon-button.js'
import '@material/web/iconbutton/filled-tonal-icon-button.js'
import '@material/web/slider/slider.js'
import '@material/web/tabs/tabs.js'
import '@material/web/tabs/primary-tab.js'
import '@material/web/textfield/outlined-text-field.js'
import App from './App'
import './index.css'

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <SolarProvider color="currentColor" size={24}>
      <App />
    </SolarProvider>
  </StrictMode>,
)
