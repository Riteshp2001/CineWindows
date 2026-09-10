/*
 * CineWindows - Video Player
 * Copyright (c) 2026 Ritesh Pandit
 *
 * CineWindows Community License
 *
 * This source code is made available for personal, non-commercial
 * use only. Organizations may not use, copy, modify, or distribute
 * this code without written permission from Ritesh Pandit.
 *
 * See the LICENSE.md file for full license terms.
 *
 * Project: CineWindows
 * Author:  Ritesh Pandit
 * Last modified: 2026-09-10
 * Modified by: Ritesh Pandit
 */

import { cn } from "@/lib/utils"

export function BrandMark({
  compact = false,
  className,
}: {
  compact?: boolean
  className?: string
}) {
  return (
    <span className={cn("inline-flex items-center gap-2.5", className)}>
      <span className="grid size-7 place-items-center rounded-lg border border-border bg-accent/10 text-accent">
        <svg viewBox="0 0 64 64" className="size-4" aria-hidden="true">
          <path d="M22.5 17.27a2.5 2.5 0 0 0-2.5 2.5v24.46a2.5 2.5 0 0 0 3.78 2.12l19.62-12.23a2.5 2.5 0 0 0 0-4.24L23.78 17.65a2.5 2.5 0 0 0-1.28-.38Z" fill="currentColor"/>
        </svg>
      </span>
      {!compact && (
        <span className="text-sm font-bold tracking-[-0.03em]">
          CineWindows
        </span>
      )}
    </span>
  )
}
