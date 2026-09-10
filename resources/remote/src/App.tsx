// Copyright (c) 2026 Ritesh Pandit
// Last modified: 2026-09-10
// Modified by: Ritesh Pandit

import { FormEvent, useCallback, useEffect, useRef, useState } from 'react'
import {
  ClapperboardPlayBoldIcon,
  ClapperboardPlayLinearIcon,
  GlobalLinearIcon,
} from '@solar-icons/react'
import { browseFolder, getPlayerState, hasPairingToken, sendCommand } from './api'
import type { BrowserState, PlayerState } from './types'
import { FolderBrowser } from './components/FolderBrowser'
import { PlayerCard } from './components/PlayerCard'
import { MaterialIcon } from './components/MaterialIcon'

const emptyPlayer: PlayerState = {
  title: '', path: '', position: 0, duration: 0, paused: true, idle: true,
  muted: false, volume: 100, playlistPosition: -1, playlistCount: 0,
}

const emptyBrowser: BrowserState = { ok: true, path: '', parent: '', items: [] }

export default function App() {
  const [player, setPlayer] = useState(emptyPlayer)
  const [connected, setConnected] = useState(false)
  const [browser, setBrowser] = useState(emptyBrowser)
  const [browserLoading, setBrowserLoading] = useState(true)
  const [activeTab, setActiveTab] = useState(0)
  const [mediaUrl, setMediaUrl] = useState('')
  const [urlError, setUrlError] = useState('')
  const [toast, setToast] = useState({
    message: hasPairingToken ? '' : 'Open the full paired address from CineWindows.',
    error: !hasPairingToken,
    visible: !hasPairingToken,
  })
  const toastTimer = useRef<number | undefined>(undefined)

  const showToast = useCallback((message: string, error = false) => {
    window.clearTimeout(toastTimer.current)
    setToast({ message, error, visible: true })
    toastTimer.current = window.setTimeout(
      () => setToast((current) => ({ ...current, visible: false })),
      2600,
    )
  }, [])

  const refreshPlayer = useCallback(async () => {
    if (document.hidden) return
    try {
      setPlayer(await getPlayerState())
      setConnected(true)
    } catch {
      setConnected(false)
    }
  }, [])

  const runCommand = useCallback(async (action: string, value?: string | number) => {
    try {
      await sendCommand(action, value)
      if (action === 'open') showToast('Playing on CineWindows')
      if (action === 'openFolder') showToast('Folder added to playback')
      await refreshPlayer()
    } catch (error) {
      showToast(error instanceof Error ? error.message : 'Command failed.', true)
    }
  }, [refreshPlayer, showToast])

  const browse = useCallback(async (path = '') => {
    setBrowserLoading(true)
    try {
      const result = await browseFolder(path)
      setBrowser({ ...result, items: result.items ?? [] })
    } catch (error) {
      showToast(error instanceof Error ? error.message : 'Folder unavailable.', true)
    } finally {
      setBrowserLoading(false)
    }
  }, [showToast])

  useEffect(() => {
    const initialRefresh = window.setTimeout(refreshPlayer, 0)
    const initialBrowse = window.setTimeout(() => { void browse() }, 0)
    const timer = window.setInterval(refreshPlayer, 1000)
    return () => {
      window.clearTimeout(initialRefresh)
      window.clearTimeout(initialBrowse)
      window.clearInterval(timer)
      window.clearTimeout(toastTimer.current)
    }
  }, [browse, refreshPlayer, showToast])

  function submitLink(event: FormEvent) {
    event.preventDefault()
    try {
      const url = new URL(mediaUrl.trim())
      if (!['http:', 'https:'].includes(url.protocol)) throw new Error()
      setUrlError('')
      void runCommand('open', mediaUrl.trim())
    } catch {
      setUrlError('Enter a complete http:// or https:// link')
    }
  }

  return (
    <main className="shell">
      <header>
        <div className="brand">
          <span className="brand-mark"><ClapperboardPlayBoldIcon /></span>
          <span className="brand-name">CineWindows</span>
        </div>
        <div className="connection">
          <span className="connection-dot" data-connected={connected} />
          <span>{connected ? 'Connected' : 'Reconnecting'}</span>
        </div>
      </header>

      <div className="workspace">
        <PlayerCard player={player} onCommand={runCommand} />

        <div className="remote-panel">
          <md-tabs
            aria-label="Remote sections"
            onChange={(event) => setActiveTab(Number((event.currentTarget as HTMLElement & { activeTabIndex: number }).activeTabIndex))}
          >
            <md-primary-tab active={activeTab === 0} onClick={() => setActiveTab(0)}>Open link</md-primary-tab>
            <md-primary-tab active={activeTab === 1} onClick={() => setActiveTab(1)}>Browse folders</md-primary-tab>
          </md-tabs>

          {activeTab === 0 ? (
            <section className="view">
              <h2 className="section-heading">Play from a link</h2>
              <p className="section-copy">Paste a direct media or streaming link. Playback starts on your computer.</p>
              <form id="linkForm" onSubmit={submitLink}>
                <md-outlined-text-field
                  error={Boolean(urlError)}
                  errorText={urlError}
                  label="Media link"
                  onInput={(event) => setMediaUrl(String((event.currentTarget as HTMLElement & { value: string }).value))}
                  placeholder="https://…"
                  required
                  type="url"
                  value={mediaUrl}
                />
                <md-filled-button type="submit">
                  <MaterialIcon icon={GlobalLinearIcon} slot="icon" />
                  Play link
                </md-filled-button>
              </form>
              <div className="link-hint">
                <ClapperboardPlayLinearIcon />
                <span>Works with direct video, audio, and stream URLs supported by CineWindows.</span>
              </div>
            </section>
          ) : (
            <FolderBrowser browser={browser} loading={browserLoading} onBrowse={browse} onCommand={runCommand} />
          )}
        </div>
      </div>

      <div
        id="toast"
        aria-live="polite"
        data-error={toast.error}
        data-visible={toast.visible}
        role="status"
      >
        {toast.message}
      </div>
    </main>
  )
}
