// CineWindows - Video Player
// Copyright (c) 2026 Ritesh Pandit
//
// CineWindows Community License
//
// This source code is made available for personal, non-commercial
// use only. Organizations may not use, copy, modify, or distribute
// this code without written permission from Ritesh Pandit.
//
// See the LICENSE.md file for full license terms.
//
// Project: CineWindows
// Author:  Ritesh Pandit
// Last modified: 2026-09-10
// Modified by: Ritesh Pandit

import { cva } from "class-variance-authority"

export const badgeVariants = cva(
  "inline-flex min-h-7 items-center gap-2 rounded-full border px-3 py-1 text-xs font-semibold uppercase tracking-[0.12em]",
  {
    variants: {
      variant: {
        default: "border-accent/30 bg-accent/10 text-accent",
        outline: "border-border-strong text-muted",
      },
    },
    defaultVariants: {
      variant: "default",
    },
  },
)
