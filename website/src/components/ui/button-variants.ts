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

export const buttonVariants = cva(
  "inline-flex min-h-11 shrink-0 items-center justify-center gap-2 whitespace-nowrap rounded-full text-sm font-semibold tracking-[-0.01em] transition-[color,background-color,border-color,transform] duration-150 ease-[cubic-bezier(0.25,1,0.5,1)] outline-none focus-visible:ring-2 focus-visible:ring-accent focus-visible:ring-offset-2 focus-visible:ring-offset-background disabled:pointer-events-none disabled:opacity-50 active:scale-[0.98] [&_svg]:pointer-events-none [&_svg]:size-4 [&_svg]:shrink-0",
  {
    variants: {
      variant: {
        default:
          "bg-accent text-accent-foreground hover:bg-accent-strong",
        outline:
          "border border-border-strong bg-transparent text-foreground hover:bg-surface-2",
        ghost: "text-foreground hover:bg-surface-2",
        inverse: "bg-foreground text-background hover:bg-foreground-muted",
      },
      size: {
        default: "px-5 py-2.5",
        sm: "min-h-10 px-4",
        lg: "min-h-13 px-6 text-base",
        icon: "size-11 p-0",
      },
    },
    defaultVariants: {
      variant: "default",
      size: "default",
    },
  },
)
