// Copyright (c) 2026 Ritesh Pandit
// Last modified: 2026-09-10
// Modified by: Ritesh Pandit

export interface PlayerState {
  title: string
  path: string
  position: number
  duration: number
  paused: boolean
  idle: boolean
  muted: boolean
  volume: number
  playlistPosition: number
  playlistCount: number
}

export interface BrowserItem {
  name: string
  path: string
  directory: boolean
}

export interface BrowserState {
  ok: boolean
  path: string
  parent: string
  items: BrowserItem[]
  truncated?: boolean
  error?: string
}
