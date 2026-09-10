// Copyright (c) 2026 Ritesh Pandit
// Last modified: 2026-09-10
// Modified by: Ritesh Pandit

import { useState } from 'react'
import {
  PauseBoldIcon,
  PlayBoldIcon,
  Rewind10SecondsBackLinearIcon,
  Rewind10SecondsForwardLinearIcon,
  SkipNextLinearIcon,
  SkipPreviousLinearIcon,
  StopLinearIcon,
  VolumeCrossLinearIcon,
  VolumeLoudLinearIcon,
} from '@solar-icons/react'
import type { PlayerState } from '../types'
import { MaterialIcon } from './MaterialIcon'

type SliderElement = HTMLElement & { value: number; max: number }

interface PlayerCardProps {
  player: PlayerState
  onCommand: (action: string, value?: string | number) => void
}

function formatTime(value: number) {
  const seconds = Math.max(0, Number(value) || 0)
  const hours = Math.floor(seconds / 3600)
  const minutes = Math.floor((seconds % 3600) / 60)
  const remainder = Math.floor(seconds % 60)
  return hours
    ? `${hours}:${String(minutes).padStart(2, '0')}:${String(remainder).padStart(2, '0')}`
    : `${minutes}:${String(remainder).padStart(2, '0')}`
}

export function PlayerCard({ player, onCommand }: PlayerCardProps) {
  const [seekPreview, setSeekPreview] = useState<number | null>(null)
  const [volumePreview, setVolumePreview] = useState<number | null>(null)
  const queue = player.playlistCount > 0
    ? `${Math.max(1, player.playlistPosition + 1)} of ${player.playlistCount}`
    : 'Queue empty'

  return (
    <section className="player" aria-labelledby="mediaTitle">
      <div className="eyebrow">{player.idle ? 'Ready to play' : player.paused ? 'Paused' : 'Playing'}</div>
      <h1 id="mediaTitle">{player.title || 'Nothing playing'}</h1>
      <div className="queue">{queue}</div>

      <div className="timeline">
        <md-slider
          aria-label="Playback position"
          max={Math.max(1, player.duration)}
          min={0}
          onChange={(event) => {
            const value = Number((event.currentTarget as SliderElement).value)
            setSeekPreview(null)
            onCommand('seek', value)
          }}
          onInput={(event) => setSeekPreview(Number((event.currentTarget as SliderElement).value))}
          step={1}
          value={seekPreview ?? player.position}
        />
        <div className="timeline-meta">
          <span>{formatTime(seekPreview ?? player.position)}</span>
          <span>{formatTime(player.duration)}</span>
        </div>
      </div>

      <div className="primary-controls">
        <md-icon-button aria-label="Previous" onClick={() => onCommand('previous')}>
          <MaterialIcon icon={SkipPreviousLinearIcon} />
        </md-icon-button>
        <md-icon-button aria-label="Back 10 seconds" onClick={() => onCommand('seekRelative', -10)}>
          <MaterialIcon icon={Rewind10SecondsBackLinearIcon} />
        </md-icon-button>
        <md-filled-tonal-icon-button
          id="playPause"
          aria-label={player.paused ? 'Play' : 'Pause'}
          onClick={() => onCommand('toggle')}
        >
          <MaterialIcon icon={player.paused ? PlayBoldIcon : PauseBoldIcon} />
        </md-filled-tonal-icon-button>
        <md-icon-button aria-label="Forward 10 seconds" onClick={() => onCommand('seekRelative', 10)}>
          <MaterialIcon icon={Rewind10SecondsForwardLinearIcon} />
        </md-icon-button>
        <md-icon-button aria-label="Next" onClick={() => onCommand('next')}>
          <MaterialIcon icon={SkipNextLinearIcon} />
        </md-icon-button>
      </div>

      <div className="utility-rail">
        <md-icon-button aria-label={player.muted ? 'Unmute' : 'Mute'} onClick={() => onCommand('mute')}>
          <MaterialIcon icon={player.muted ? VolumeCrossLinearIcon : VolumeLoudLinearIcon} />
        </md-icon-button>
        <div className="volume-control">
          <div className="volume-label">
            <span>Volume</span>
            <strong>{Math.round(volumePreview ?? player.volume)}%</strong>
          </div>
          <md-slider
            aria-label="Volume"
            max={200}
            min={0}
            onChange={(event) => {
              const value = Number((event.currentTarget as SliderElement).value)
              setVolumePreview(null)
              onCommand('volume', value)
            }}
            onInput={(event) => setVolumePreview(Number((event.currentTarget as SliderElement).value))}
            step={1}
            value={volumePreview ?? player.volume}
          />
        </div>
        <md-outlined-button onClick={() => onCommand('stop')}>
          <MaterialIcon icon={StopLinearIcon} slot="icon" />
          Stop
        </md-outlined-button>
      </div>
    </section>
  )
}
