// Copyright (c) 2026 Ritesh Pandit
// Last modified: 2026-09-10
// Modified by: Ritesh Pandit

import {
  AltArrowLeftLinearIcon,
  AltArrowRightLinearIcon,
  ClapperboardLinearIcon,
  FolderLinearIcon,
} from '@solar-icons/react'
import type { BrowserState } from '../types'
import { MaterialIcon } from './MaterialIcon'

interface FolderBrowserProps {
  browser: BrowserState
  loading: boolean
  onBrowse: (path?: string) => void
  onCommand: (action: string, value?: string | number) => void
}

export function FolderBrowser({ browser, loading, onBrowse, onCommand }: FolderBrowserProps) {
  return (
    <section className="view">
      <div className="folder-toolbar">
        <md-icon-button aria-label="Go up" disabled={!browser.path} onClick={() => onBrowse(browser.parent)}>
          <MaterialIcon icon={AltArrowLeftLinearIcon} />
        </md-icon-button>
        <span className="folder-path">{browser.path || 'This computer'}</span>
        {browser.path && (
          <md-filled-tonal-button onClick={() => onCommand('openFolder', browser.path)}>
            Play folder
          </md-filled-tonal-button>
        )}
      </div>

      <div id="browserList" aria-busy={loading} aria-live="polite" data-loading={loading}>
        {!loading && browser.truncated && (
          <p className="browser-notice" role="status">
            Showing the first 1,000 playable items. Open a subfolder to narrow the list.
          </p>
        )}
        {!loading && browser.items.length === 0 && (
          <div className="empty-state">
            <FolderLinearIcon className="empty-icon" size={28} />
            <strong>This folder is quiet</strong>
            <span>No playable media or subfolders were found.</span>
          </div>
        )}
        {browser.items.map((item, index) => (
          <button
            className="media-row"
            key={item.path}
            onClick={() => item.directory ? onBrowse(item.path) : onCommand('open', item.path)}
            style={{ '--row-index': Math.min(index, 8) } as React.CSSProperties}
            type="button"
          >
            <span className="row-icon">
              {item.directory ? <FolderLinearIcon /> : <ClapperboardLinearIcon />}
            </span>
            <span className="row-copy">
              <strong>{item.name}</strong>
              <small>{item.directory ? 'Folder' : 'Play now'}</small>
            </span>
            <AltArrowRightLinearIcon className="chevron" />
          </button>
        ))}
      </div>
    </section>
  )
}
