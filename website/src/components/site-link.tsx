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

import type { AnchorHTMLAttributes, MouseEvent } from "react"

type SiteLinkProps = AnchorHTMLAttributes<HTMLAnchorElement> & {
  href: string
}

export function SiteLink({ href, onClick, ...props }: SiteLinkProps) {
  function handleClick(event: MouseEvent<HTMLAnchorElement>) {
    onClick?.(event)

    if (
      event.defaultPrevented ||
      event.button !== 0 ||
      event.metaKey ||
      event.ctrlKey ||
      event.shiftKey ||
      event.altKey ||
      !href.startsWith("/")
    ) {
      return
    }

    event.preventDefault()
    window.history.pushState({}, "", href)
    window.dispatchEvent(new PopStateEvent("popstate"))
    const hashIndex = href.indexOf("#")
    const hash = hashIndex >= 0 ? href.slice(hashIndex + 1) : ""
    if (hash) {
      window.requestAnimationFrame(() => {
        document.getElementById(hash)?.scrollIntoView({ behavior: "smooth" })
      })
    } else {
      window.scrollTo({ top: 0, behavior: "auto" })
    }
  }

  return <a href={href} onClick={handleClick} {...props} />
}
