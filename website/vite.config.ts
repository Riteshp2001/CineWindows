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

import path from "node:path"
import tailwindcss from "@tailwindcss/vite"
import react from "@vitejs/plugin-react"
import { defineConfig, loadEnv, type Plugin } from "vite"

function metadataPlugin(siteUrl: string): Plugin {
  return {
    name: "cinewindows-metadata",
    transformIndexHtml(html) {
      return html.replaceAll("__SITE_URL__", siteUrl)
    },
  }
}

function seoPlugin(siteUrl: string): Plugin {
  const lastmod = "2026-08-01"
  const pages = [
    { path: "/", priority: "0.8" },
    { path: "/download", priority: "0.7" },
    { path: "/changelog", priority: "0.6" },
    { path: "/privacy", priority: "0.5" },
    { path: "/terms", priority: "0.5" },
    { path: "/support", priority: "0.6" },
    { path: "/docs/", priority: "0.6" },
  ]
  const xmlEscape = (value: string) =>
    value.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
  const sitemap = `<?xml version="1.0" encoding="UTF-8"?>
<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
${pages
  .map(
    (page) => `  <url>
    <loc>${xmlEscape(siteUrl)}${page.path}</loc>
    <lastmod>${lastmod}</lastmod>
    <changefreq>monthly</changefreq>
    <priority>${page.priority}</priority>
  </url>`
  )
  .join("\n")}
</urlset>
`
  const robots = `User-agent: *
Allow: /

Sitemap: ${xmlEscape(siteUrl)}/sitemap.xml
`
  return {
    name: "cinewindows-seo-files",
    generateBundle() {
      this.emitFile({ type: "asset", fileName: "robots.txt", source: robots })
      this.emitFile({ type: "asset", fileName: "sitemap.xml", source: sitemap })
    },
  }
}

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), "")
  const siteUrl = (
    env.VITE_SITE_URL || "http://localhost:5173"
  ).replace(/\/$/, "")

  return {
    plugins: [
      react(),
      tailwindcss(),
      metadataPlugin(siteUrl),
      seoPlugin(siteUrl),
    ],
    resolve: {
      alias: {
        "@": path.resolve(import.meta.dirname, "./src"),
      },
    },
    build: {
      target: "es2022",
      cssMinify: "lightningcss",
    },
  }
})
