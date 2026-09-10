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

import { clsx, type ClassValue } from "clsx"
import { twMerge } from "tailwind-merge"

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs))
}
