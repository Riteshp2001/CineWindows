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

import { useEffect } from "react"

let revealObserver: IntersectionObserver | null = null

function getRevealObserver() {
  if (revealObserver) return revealObserver
  revealObserver = new IntersectionObserver(
    (entries) => {
      for (const entry of entries) {
        if (!entry.isIntersecting) continue
        const element = entry.target as HTMLElement
        element.classList.add("is-visible")
        revealObserver?.unobserve(element)
      }
    },
    { threshold: 0.12, rootMargin: "0px 0px -48px 0px" },
  )
  return revealObserver
}

function applyStagger(element: HTMLElement) {
  const group = element.closest("[data-stagger]")
  if (!group) return
  const siblings = Array.from(group.querySelectorAll<HTMLElement>(".reveal"))
  const index = siblings.indexOf(element)
  if (index > 0) {
    element.style.setProperty("--reveal-delay", `${Math.min(index, 8) * 55}ms`)
  }
}

export function useReveals(dep: unknown) {
  useEffect(() => {
    if (typeof window === "undefined") return
    const reducedMotion = window.matchMedia(
      "(prefers-reduced-motion: reduce)",
    ).matches
    const observedElements = new Set<HTMLElement>()
    const observer = reducedMotion ? null : getRevealObserver()

    function observeElement(element: HTMLElement) {
      if (observedElements.has(element)) return
      observedElements.add(element)
      if (reducedMotion) {
        element.classList.add("is-visible")
        element.style.removeProperty("--reveal-delay")
        return
      }
      applyStagger(element)
      observer?.observe(element)
    }

    function observeReveals(root: ParentNode) {
      if (root instanceof HTMLElement && root.matches(".reveal")) {
        observeElement(root)
      }
      root.querySelectorAll<HTMLElement>(".reveal").forEach(observeElement)
    }

    observeReveals(document)
    const mutationObserver = new MutationObserver((mutations) => {
      mutations.forEach((mutation) => {
        mutation.addedNodes.forEach((node) => {
          if (node instanceof HTMLElement) observeReveals(node)
        })
      })
    })
    mutationObserver.observe(document.body, { childList: true, subtree: true })

    return () => {
      mutationObserver.disconnect()
      observedElements.forEach((element) => {
        observer?.unobserve(element)
        element.classList.remove("is-visible")
      })
    }
  }, [dep])
}
