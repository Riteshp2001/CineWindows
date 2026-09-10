// Copyright (c) 2026 Ritesh Pandit
// Last modified: 2026-09-10
// Modified by: Ritesh Pandit

/// <reference types="vite/client" />

import type { HTMLAttributes } from 'react'

type MaterialElementProps = HTMLAttributes<HTMLElement> & {
  active?: boolean
  disabled?: boolean
  error?: boolean
  errorText?: string
  label?: string
  max?: number
  min?: number
  placeholder?: string
  required?: boolean
  step?: number
  type?: string
  value?: number | string
}

declare module 'react' {
  namespace JSX {
    interface IntrinsicElements {
      'md-filled-button': MaterialElementProps
      'md-filled-tonal-button': MaterialElementProps
      'md-outlined-button': MaterialElementProps
      'md-icon-button': MaterialElementProps
      'md-filled-tonal-icon-button': MaterialElementProps
      'md-slider': MaterialElementProps
      'md-tabs': MaterialElementProps
      'md-primary-tab': MaterialElementProps
      'md-outlined-text-field': MaterialElementProps
    }
  }
}
