// Copyright (c) 2026 Ritesh Pandit
// Last modified: 2026-09-10
// Modified by: Ritesh Pandit

import type { Icon } from '@solar-icons/react/lib/types'

interface MaterialIconProps {
  icon: Icon
  slot?: 'icon'
}

export function MaterialIcon({ icon: Icon, slot }: MaterialIconProps) {
  return (
    <span className="material-icon" slot={slot}>
      <Icon color="currentColor" size={24} />
    </span>
  )
}
