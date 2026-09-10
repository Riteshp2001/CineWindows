// Copyright (c) 2026 Ritesh Pandit
// Last modified: 2026-09-10
// Modified by: Ritesh Pandit

import type { BrowserState, PlayerState } from './types'

const token = new URLSearchParams(location.search).get('token') ?? ''

function endpoint(path: string, params: Record<string, string> = {}) {
  return `${path}?${new URLSearchParams({ ...params, token })}`
}

async function request<T>(url: string, options?: RequestInit): Promise<T> {
  const response = await fetch(url, options)
  let data: Partial<T> & { error?: string } = {}
  try {
    data = await response.json()
  } catch {
    // The error below provides a useful fallback for non-JSON server responses.
  }
  if (!response.ok) throw new Error(data.error ?? 'CineWindows did not accept that request.')
  return data as T
}

export const hasPairingToken = token.length > 0

export function getPlayerState() {
  return request<PlayerState>(endpoint('/api/state'))
}

export function browseFolder(path = '') {
  return request<BrowserState>(endpoint('/api/browse', path ? { path } : {}))
}

export function sendCommand(action: string, value?: string | number) {
  return request<{ ok: boolean }>(endpoint('/api/command'), {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ action, value }),
  })
}
