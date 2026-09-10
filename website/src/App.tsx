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

import {
  useEffect,
  useState,
  type CSSProperties,
  type ReactNode,
} from "react"
import {
  ArrowRight,
  Captions,
  Check,
  Code2,
  Download,
  FileVideo2,
  Globe2,
  Keyboard,
  Languages,
  ListVideo,
  Menu,
  MonitorPlay,
  MousePointer2,
  PanelsTopLeft,
  Radio,
  ShieldCheck,
  Subtitles,
  WandSparkles,
  Zap,
} from "lucide-react"

import { BrandMark } from "@/components/brand-mark"
import { SiteLink } from "@/components/site-link"
import { useReveals } from "@/hooks/use-reveal"
import {
  Accordion,
  AccordionContent,
  AccordionItem,
  AccordionTrigger,
} from "@/components/ui/accordion"
import { Badge } from "@/components/ui/badge"
import { Button } from "@/components/ui/button"
import { Separator } from "@/components/ui/separator"
import {
  Sheet,
  SheetClose,
  SheetContent,
  SheetDescription,
  SheetTitle,
  SheetTrigger,
} from "@/components/ui/sheet"

const configuredSiteUrl = (import.meta.env.VITE_SITE_URL || "").replace(
  /\/$/,
  "",
)

const navigation: { href: string; label: string; external?: boolean }[] = [
  { href: "/#features", label: "Features" },
  { href: "/download", label: "Download" },
  { href: "/changelog", label: "Changelog" },
  { href: "/docs/", label: "Docs" },
  { href: "/#faq", label: "FAQ" },
]

const metadata: Record<
  string,
  { title: string; description: string }
> = {
  "/": {
    title: "CineWindows — Cinema, without the clutter",
    description:
      "A fast, native Windows media player for local video, streaming, subtitles, playlists, and precise playback control.",
  },
  "/docs": {
    title: "Docs — CineWindows",
    description:
      "Documentation for CineWindows: installation, playback, streaming, shortcuts, configuration, and more.",
  },
  "/docs/": {
    title: "Docs — CineWindows",
    description:
      "Documentation for CineWindows: installation, playback, streaming, shortcuts, configuration, and more.",
  },
  "/download": {
    title: "Download — CineWindows",
    description: "Download the latest CineWindows installer or portable build for Windows.",
  },
  "/changelog": {
    title: "Changelog — CineWindows",
    description: "Release notes and downloads for every CineWindows version.",
  },
  "/privacy": {
    title: "Privacy Policy — CineWindows",
    description:
      "Learn how CineWindows handles local settings, update checks, streaming URLs, and support requests.",
  },
  "/terms": {
    title: "Terms of Service — CineWindows",
    description:
      "The terms governing your use of the CineWindows application, website, and related services.",
  },
  "/support": {
    title: "Support — CineWindows",
    description:
      "Get help with CineWindows installation, playback, streaming, subtitles, and settings.",
  },
}

function Header() {
  return (
    <header className="site-header">
      <div className="site-container flex h-14 items-center justify-between">
        <SiteLink
          href="/"
          className="rounded-lg outline-none focus-visible:ring-2 focus-visible:ring-accent"
          aria-label="CineWindows home"
        >
          <BrandMark />
        </SiteLink>

        <nav className="hidden items-center gap-6 lg:flex" aria-label="Primary">
          {navigation.map((item) => (
            <a
              className="nav-link"
              href={item.href}
              key={item.href}
              target={item.external ? "_blank" : undefined}
              rel={item.external ? "noreferrer" : undefined}
            >
              {item.label}
            </a>
          ))}
        </nav>

        <div className="hidden items-center gap-2 sm:flex">
          <Button variant="ghost" size="sm" asChild>
            <a
              href="https://github.com/Riteshp2001/CineWindows"
              target="_blank"
              rel="noreferrer"
            >
              <Code2 className="size-4" />
              GitHub
            </a>
          </Button>
          <Button size="sm" asChild>
            <a href="https://github.com/Riteshp2001/CineWindows/releases" target="_blank" rel="noreferrer">
              Get CineWindows
              <ArrowRight className="size-4" />
            </a>
          </Button>
        </div>

        <Sheet>
          <SheetTrigger asChild>
            <Button
              variant="ghost"
              size="icon"
              className="sm:hidden"
              aria-label="Open navigation"
            >
              <Menu className="size-5" />
            </Button>
          </SheetTrigger>
          <SheetContent>
            <SheetTitle>
              <BrandMark />
            </SheetTitle>
            <SheetDescription className="mt-3">
              Cinema, without the clutter.
            </SheetDescription>
            <nav
              className="mt-10 flex flex-col border-t border-border"
              aria-label="Mobile"
            >
              {navigation.map((item) => (
                <SheetClose asChild key={item.href}>
                  <a
                    className="flex min-h-14 items-center justify-between border-b border-border text-lg font-semibold"
                    href={item.href}
                    target={item.external ? "_blank" : undefined}
                    rel={item.external ? "noreferrer" : undefined}
                  >
                    {item.label}
                    <ArrowRight className="size-4 text-muted" />
                  </a>
                </SheetClose>
              ))}
            </nav>
            <Button className="mt-auto w-full" size="lg" asChild>
              <a href="https://github.com/Riteshp2001/CineWindows/releases" target="_blank" rel="noreferrer">
                <Download className="size-4" />
                Get CineWindows
              </a>
            </Button>
          </SheetContent>
        </Sheet>
      </div>
    </header>
  )
}

function Footer() {
  return (
    <footer className="border-t border-border bg-surface">
      <div className="site-container py-12 sm:py-16">
        <div className="flex flex-col gap-8 lg:flex-row lg:items-end lg:justify-between">
          <div>
            <BrandMark />
            <p className="mt-3 max-w-sm text-sm leading-6 text-muted">
              A focused, native media player for Windows. Built to disappear
              when the movie starts.
            </p>
          </div>
          <nav
            className="flex flex-wrap gap-x-8 gap-y-3 text-sm"
            aria-label="Footer"
          >
            <SiteLink className="footer-link" href="/docs/">
              Docs
            </SiteLink>
            <SiteLink className="footer-link" href="/download">
              Download
            </SiteLink>
            <SiteLink className="footer-link" href="/changelog">
              Changelog
            </SiteLink>
            <SiteLink className="footer-link" href="/privacy">
              Privacy
            </SiteLink>
            <SiteLink className="footer-link" href="/terms">
              Terms
            </SiteLink>
            <SiteLink className="footer-link" href="/support">
              Support
            </SiteLink>
            <a
              className="footer-link"
              href="https://github.com/Riteshp2001/CineWindows"
              target="_blank"
              rel="noreferrer"
            >
              GitHub
            </a>
            <a
              className="footer-link"
              href="mailto:panditritesh2001@gmail.com"
            >
              Contact
            </a>
          </nav>
        </div>
        <Separator className="my-8" />
        <div className="flex flex-col gap-1 text-xs text-muted sm:flex-row sm:items-center sm:justify-between">
          <p>© 2026 Ritesh Pandit. All rights reserved.</p>
          <p>Made for Windows 10 and 11</p>
        </div>
      </div>
    </footer>
  )
}

function Hero() {
  return (
    <section className="hero-section">
      <div className="site-container py-16 sm:py-20">
        <div className="grid items-center gap-12 lg:grid-cols-[1fr_1.2fr] lg:gap-16">

          <div className="flex flex-col items-start text-left">
            <h1 className="hero-title hero-enter">
              Cinema,
              <span
                className="block"
                style={{
                  background: "linear-gradient(135deg, oklch(80% 0.16 195) 0%, oklch(70% 0.15 195) 100%)",
                  WebkitBackgroundClip: "text",
                  WebkitTextFillColor: "transparent",
                  backgroundClip: "text",
                }}
              >
                without the clutter.
              </span>
            </h1>

            <p className="mt-5 max-w-lg text-base leading-relaxed text-muted sm:text-lg sm:leading-8 hero-enter" style={{ "--hero-delay": "70ms" } as CSSProperties}>
              A fast, native Windows media player for the movies you own and the streams you follow.
              {" "}<span className="text-foreground/80 font-medium">Zero accounts, zero analytics, zero noise.</span>
            </p>

            <div className="mt-7 flex flex-col gap-3 w-full sm:w-auto sm:flex-row sm:items-center hero-enter" style={{ "--hero-delay": "140ms" } as CSSProperties}>
              <Button
                size="lg"
                className="w-full sm:w-auto px-6 h-12 text-sm font-semibold rounded-xl"
                asChild
              >
                <a
                  href="https://github.com/Riteshp2001/CineWindows/releases"
                  target="_blank"
                  rel="noreferrer"
                >
                  <Download className="size-4" />
                  Get from GitHub Releases
                </a>
              </Button>
              <Button
                variant="outline"
                size="lg"
                className="w-full sm:w-auto px-6 h-12 text-sm font-semibold rounded-xl"
                asChild
              >
                <a href="#features">
                  See features
                  <ArrowRight className="size-4" />
                </a>
              </Button>
            </div>

            <div className="mt-6 flex flex-wrap gap-x-5 gap-y-2 text-[11px] font-medium text-muted hero-enter" style={{ "--hero-delay": "210ms" } as CSSProperties}>
              {[
                "Windows 10 & 11 (64-bit)",
                "No Account Required",
              ].map(label => (
                <span key={label} className="inline-flex items-center gap-1.5">
                  <Check className="size-3.5 text-accent" />
                  {label}
                </span>
              ))}
            </div>
          </div>

          <div className="hero-shot overflow-hidden rounded-xl border border-border hero-enter" style={{ "--hero-delay": "260ms" } as CSSProperties}>
            <img
              src="/player-controls.png"
              alt="CineWindows screenshot"
              className="w-full"
            />
          </div>
        </div>
      </div>
    </section>
  )
}

const features = [
  {
    icon: FileVideo2,
    title: "Wide format support",
    copy: "Play the formats FFmpeg understands, from everyday MP4 files to high-bitrate MKV collections.",
  },
  {
    icon: Zap,
    title: "Hardware acceleration",
    copy: "Use modern Windows decoding paths when available, with control when compatibility matters more.",
  },
  {
    icon: Subtitles,
    title: "Subtitles that behave",
    copy: "Auto-load common formats, choose tracks, set language priority, and tune style, timing, and position.",
  },
  {
    icon: ListVideo,
    title: "Serious playlists",
    copy: "Reorder, shuffle, repeat, save, and restore playlists without turning playback into library management.",
  },
  {
    icon: Keyboard,
    title: "Remappable shortcuts",
    copy: "Change every key binding or bring your existing mpv input configuration with you.",
  },
  {
    icon: Code2,
    title: "Local JSON-RPC",
    copy: "Control playback and observe properties from local tools through an mpv-compatible TCP interface.",
  },
  {
    icon: Languages,
    title: "Global by design",
    copy: "A localization-ready interface with more than 50 translations already included.",
  },
  {
    icon: ShieldCheck,
    title: "Private by default",
    copy: "No account, advertising profile, or analytics pipeline. Your local media stays on your device.",
  },
]

function FeaturesSection() {
  return (
    <section id="features" className="section-space bg-background">
      <div className="site-container">
        <div className="max-w-xl reveal">
          <p className="eyebrow text-muted">Built with intention</p>
          <h2 className="section-title mt-4">
            Small footprint.
            <span className="text-muted"> Deep control.</span>
          </h2>
        </div>
        <div className="mt-12 grid gap-4 sm:grid-cols-2 lg:grid-cols-4" data-stagger>
          {features.map((feature) => {
            const Icon = feature.icon
            return (
              <article className="feature-card reveal" key={feature.title}>
                <div className="inline-flex size-9 items-center justify-center rounded-lg bg-accent/10 text-accent">
                  <Icon className="size-4" aria-hidden="true" />
                </div>
                <h3 className="mt-4 text-base font-semibold">
                  {feature.title}
                </h3>
                <p className="mt-2 text-sm leading-6 text-muted">
                  {feature.copy}
                </p>
              </article>
            )
          })}
        </div>
      </div>
    </section>
  )
}

const highlights = [
  {
    eyebrow: "Playback",
    title: "Controls when you need them.",
    copy: "A frameless player keeps the picture dominant. Hover for seeking, subtitles, chapters, speed, audio tracks, and more.",
    icon: MonitorPlay,
  },
  {
    eyebrow: "Streaming",
    title: "Paste a URL. Keep watching.",
    copy: "Open supported online media with built-in yt-dlp integration — then control it with the same native interface.",
    icon: Radio,
  },
  {
    eyebrow: "Control",
    title: "Your player, your way.",
    copy: "Remap keys, tune video and subtitle presentation, manage playlists, loop sections, and expose playback through local JSON-RPC.",
    icon: MonitorPlay,
  },
]

const specs = [
  ["Platform", "Windows 10 or 11, 64-bit"],
  ["Playback engine", "libmpv + FFmpeg"],
  ["Media", "Local files, folders, playlists, URLs"],
  ["Subtitles", "SRT, ASS, SSA, VTT, and more"],
  ["Acceleration", "D3D11VA, DXVA2, configurable"],
  ["Input", "Mouse, keyboard, drag and drop"],
]

function SpecsSection() {
  return (
    <section className="section-space border-t border-border bg-surface">
      <div className="site-container">
        <div className="grid gap-12 lg:grid-cols-[0.6fr_1.4fr] lg:gap-20">
          <div className="reveal">
            <p className="eyebrow text-muted">Technical details</p>
            <h2 className="mt-4 text-3xl font-semibold tracking-[-0.045em] sm:text-4xl">
              Native where it counts.
            </h2>
            <p className="mt-4 max-w-md text-sm leading-6 text-muted">
              Built in C++ and Qt 6 for the desktop, with a proven playback
              engine underneath and no browser runtime between you and the
              frame.
            </p>
          </div>
          <div className="grid gap-10 lg:grid-cols-2 lg:gap-6" data-stagger>
            <div className="reveal">
              {specs.slice(0, 3).map(([label, value]) => (
                <div className="border-t border-border py-3 text-sm" key={label}>
                  <dt className="text-muted">{label}</dt>
                  <dd className="mt-1 font-medium text-foreground">{value}</dd>
                </div>
              ))}
            </div>
            <div>
              {specs.slice(3).map(([label, value]) => (
                <div className="border-t border-border py-3 text-sm" key={label}>
                  <dt className="text-muted">{label}</dt>
                  <dd className="mt-1 font-medium text-foreground">{value}</dd>
                </div>
              ))}
            </div>
          </div>
        </div>

        <div className="mt-16 grid gap-10 lg:grid-cols-3 lg:gap-8" data-stagger>
          {highlights.map((item) => {
            const Icon = item.icon
            return (
              <article className="reveal" key={item.title}>
                <div className="flex items-center gap-3">
                  <div className="inline-flex size-8 items-center justify-center rounded-lg bg-surface text-muted">
                    <Icon className="size-4" aria-hidden="true" />
                  </div>
                  <p className="eyebrow text-muted">{item.eyebrow}</p>
                </div>
                <h3 className="mt-4 text-xl font-semibold tracking-[-0.03em]">
                  {item.title}
                </h3>
                <p className="mt-3 text-sm leading-6 text-muted">
                  {item.copy}
                </p>
              </article>
            )
          })}
        </div>
      </div>
    </section>
  )
}

const faqs = [
  {
    question: "What can CineWindows play?",
    answer:
      "CineWindows uses libmpv and FFmpeg, so it supports a broad range of video, audio, subtitle, and playlist formats. That includes common MP4, MKV, AVI, MOV, MP3, FLAC, SRT, ASS, SSA, VTT, M3U, and M3U8 files.",
  },
  {
    question: "Does it require an account or internet connection?",
    answer:
      "No account is required. Local playback works offline. An internet connection is only needed for online streams, opening external links, or checking GitHub for app updates.",
  },
  {
    question: "Can I stream from a URL?",
    answer:
      "Yes. CineWindows includes yt-dlp integration for supported media URLs. Availability depends on the provider, your network, and the provider's own terms.",
  },
  {
    question: "Can I customize keyboard shortcuts?",
    answer:
      "Yes. Every key binding can be changed in Preferences, and advanced users can work directly with an mpv-style input configuration.",
  },
  {
    question: "Does CineWindows collect analytics?",
    answer:
      "No. CineWindows does not include analytics, advertising trackers, or an account system. Optional update checks contact GitHub, and online playback connects to the media provider you choose.",
  },
]

function FaqSection() {
  return (
    <section id="faq" className="section-space bg-background">
      <div className="site-container">
        <div className="grid gap-10 lg:grid-cols-[0.5fr_1.5fr] lg:gap-20">
          <div className="reveal">
            <p className="eyebrow text-muted">FAQ</p>
            <h2 className="mt-4 text-3xl font-semibold tracking-[-0.045em] sm:text-4xl">
              Before you press play.
            </h2>
          </div>
          <Accordion type="single" collapsible className="border-t border-border reveal" style={{ "--reveal-delay": "120ms" } as CSSProperties}>
            {faqs.map((faq, index) => (
              <AccordionItem value={`item-${index}`} key={faq.question}>
                <AccordionTrigger>{faq.question}</AccordionTrigger>
                <AccordionContent>{faq.answer}</AccordionContent>
              </AccordionItem>
            ))}
          </Accordion>
        </div>
      </div>
    </section>
  )
}

function DownloadSection() {
  return (
    <section className="section-space border-t border-border bg-surface">
      <div className="site-container">
        <div className="max-w-2xl reveal">
          <Badge variant="outline" className="border-accent/30 text-accent bg-accent/5">Made for the desktop</Badge>
          <h2 className="mt-6 text-4xl font-semibold leading-[0.97] tracking-[-0.055em] sm:text-5xl lg:text-6xl">
            Your next movie deserves a quieter player.
          </h2>
          <div className="mt-8 flex flex-col gap-4 sm:flex-row sm:items-center">
            <Button size="lg" className="h-12 px-6 rounded-xl text-sm font-semibold" asChild>
              <a href="https://github.com/Riteshp2001/CineWindows/releases" target="_blank" rel="noreferrer">
                <PanelsTopLeft className="size-4" />
                Get CineWindows
              </a>
            </Button>
            <p className="text-sm leading-6 text-muted">
              Version 1.0.0 &mdash; Windows 10 and 11 (64-bit)
            </p>
          </div>
        </div>
      </div>
    </section>
  )
}

const capabilityWords = [
  "VIDEO",
  "AUDIO",
  "STREAMS",
  "SUBTITLES",
  "PLAYLISTS",
  "CHAPTERS",
]

function CapabilityMarquee() {
  return (
    <div
      className="marquee-pause overflow-hidden border-y border-border bg-accent py-4 text-accent-foreground select-none"
      aria-hidden="true"
    >
      <div className="marquee-track">
        <div className="flex shrink-0 items-center gap-7 pr-7">
          {capabilityWords.map((word) => (
            <span
              className="inline-flex items-center gap-7 text-sm font-bold tracking-wider whitespace-nowrap"
              key={word}
            >
              {word}
              <span className="text-xs opacity-50 font-normal">✦</span>
            </span>
          ))}
        </div>
        <div className="flex shrink-0 items-center gap-7 pr-7">
          {capabilityWords.map((word) => (
            <span
              className="inline-flex items-center gap-7 text-sm font-bold tracking-wider whitespace-nowrap"
              key={`${word}-dup`}
            >
              {word}
              <span className="text-xs opacity-50 font-normal">✦</span>
            </span>
          ))}
        </div>
      </div>
    </div>
  )
}

function HomePage() {
  return (
    <>
      <Header />
      <main id="main-content">
        <Hero />
        <CapabilityMarquee />
        <FeaturesSection />
        <SpecsSection />
        <FaqSection />
        <DownloadSection />
      </main>
      <Footer />
    </>
  )
}

function LegalShell({
  eyebrow,
  title,
  intro,
  children,
}: {
  eyebrow: string
  title: string
  intro: string
  children: ReactNode
}) {
  return (
    <>
      <Header />
      <main id="main-content">
        <div className="site-container py-16 sm:py-24">
          <div className="grid gap-10 lg:grid-cols-[0.55fr_1.45fr] lg:gap-16">
            <header className="lg:sticky lg:top-20 lg:self-start reveal">
              <p className="eyebrow text-muted">{eyebrow}</p>
              <h1 className="mt-4 text-3xl font-semibold tracking-[-0.045em] sm:text-4xl">
                {title}
              </h1>
              <p className="mt-4 max-w-sm text-sm leading-6 text-muted">
                {intro}
              </p>
              <p className="mt-6 text-xs font-semibold uppercase tracking-[0.1em] text-muted">
                Last updated July 15, 2026
              </p>
            </header>
            <article className="legal-copy reveal" style={{ "--reveal-delay": "120ms" } as CSSProperties}>{children}</article>
          </div>
        </div>
      </main>
      <Footer />
    </>
  )
}

function PrivacyPage() {
  return (
    <LegalShell
      eyebrow="Legal"
      title="Privacy Policy"
      intro="CineWindows is designed to play your media, not profile you. This policy explains the limited data involved when you use the app."
    >
      <section>
        <h2>1. Who this covers</h2>
        <p>
          This Privacy Policy applies to the CineWindows desktop application,
          the CineWindows website, and support communications operated by
          Ritesh Pandit.
        </p>
      </section>

      <section>
        <h2>2. Information we handle</h2>
        <h3>Local media and settings</h3>
        <p>
          CineWindows processes media files, folders, playlists, subtitle
          tracks, playback positions, window preferences, language settings,
          keyboard mappings, and related configuration on your device. This
          information stays local. We do not receive your local media or settings.
        </p>
        <h3>Local history and library</h3>
        <p>
          The optional media library, favorites, resume positions, thumbnails,
          playback history, and watch statistics are stored in a local SQLite
          database and cache. They are not analytics and are never transmitted
          to us. History can be cleared from the application.
        </p>
        <h3>Update checks</h3>
        <p>
          When automatic update checks are enabled, CineWindows requests the
          latest release information from GitHub. GitHub receives standard
          network data such as your IP address and request time. You can disable
          automatic update checks in Preferences.
        </p>
        <h3>Optional integrations</h3>
        <p>
          YouTube search runs yt-dlp and contacts YouTube. Subtitle search sends
          the current media title and selected languages to OpenSubtitles when
          you provide an API key. Casting discovers devices on your local
          network. Companion Remote listens on your local network only after you
          enable it and uses a rotating pairing code.
        </p>
        <h3>Online media</h3>
        <p>
          When you open an online media URL, CineWindows connects to the
          provider you selected. That provider receives the URL, IP address, and
          other standard request data under its own privacy policy. We do not
          control third-party practices.
        </p>
        <h3>Support communications</h3>
        <p>
          If you email us or submit a support request, we receive the contact
          information and technical details you provide. Please avoid sending
          passwords, payment data, or private media.
        </p>
      </section>

      <section>
        <h2>3. What we do not collect</h2>
        <p>
          CineWindows has no advertising SDK, analytics tracker, account system,
          or behavioral profile. We do not sell or rent personal information.
        </p>
      </section>

      <section>
        <h2>4. Why information is used</h2>
        <p>
          The limited information described above is used to provide playback,
          remember preferences on your device, check for updates, deliver
          support, and comply with legal obligations.
        </p>
      </section>

      <section>
        <h2>5. Sharing and third parties</h2>
        <p>
          We do not share personal information for advertising. Information may
          be processed by services you intentionally use: GitHub for
          releases, updates, and issues, and your chosen media
          provider for streaming. We may disclose information when required by law.
        </p>
      </section>

      <section>
        <h2>6. Storage and security</h2>
        <p>
          Local settings remain on your device until you clear them or
          uninstall the app. Support records are retained as long as reasonably
          needed. We use reasonable safeguards, but no system is completely secure.
        </p>
      </section>

      <section>
        <h2>7. Your choices</h2>
        <p>
          You can disable update checks, avoid opening online URLs, clear local
          settings, or uninstall CineWindows. Depending on your location, you
          may have rights to access, correct, or delete your personal
          information. Contact us to make a request.
        </p>
      </section>

      <section>
        <h2>8. Children's privacy</h2>
        <p>
          CineWindows is a general-audience media player and is not directed to
          children under 13. We do not knowingly collect information from children.
        </p>
      </section>

      <section>
        <h2>9. International processing</h2>
        <p>
          Third-party services such as GitHub may process
          information in countries other than your own. Their privacy notices
          explain the safeguards they use.
        </p>
      </section>

      <section>
        <h2>10. Changes to this policy</h2>
        <p>
          We may update this policy when CineWindows or requirements change. The
          "Last updated" date identifies the current version. Material changes
          will be posted on the website.
        </p>
      </section>

      <section>
        <h2>11. Contact</h2>
        <p>
          Privacy questions or requests:{" "}
          <a href="mailto:panditritesh2001@gmail.com">panditritesh2001@gmail.com</a>.
          You can also visit the <SiteLink href="/support">support page</SiteLink>.
        </p>
      </section>
    </LegalShell>
  )
}

function TermsPage() {
  return (
    <LegalShell
      eyebrow="Legal"
      title="Terms of Service"
      intro="By using CineWindows, you agree to these terms. They are straightforward: the software is provided as-is, and you are responsible for how you use it."
    >
      <section>
        <h2>1. Acceptance</h2>
        <p>
          By downloading, installing, or using CineWindows ("the Software"), you
          agree to these Terms of Service. If you do not agree, do not use the
          Software.
        </p>
      </section>

      <section>
        <h2>2. License</h2>
        <p>
          CineWindows is proprietary software provided under the license terms
          included with the application and repository. No right to copy,
          modify, redistribute, sublicense, or reverse engineer the Software is
          granted except where applicable law expressly requires otherwise.
        </p>
      </section>

      <section>
        <h2>3. Use of the Software</h2>
        <p>
          You agree to use the Software in compliance with all applicable laws.
          You may not use the Software to infringe on the rights of others,
          distribute malware, or engage in any unlawful activity.
        </p>
      </section>

      <section>
        <h2>4. Intellectual property</h2>
        <p>
          The CineWindows name, logo, and visual identity are trademarks of
          Ritesh Pandit. These terms do not grant you any rights to use them.
          Third-party components used by the Software retain their own licenses.
        </p>
      </section>

      <section>
        <h2>5. Disclaimer of warranties</h2>
        <p>
          The Software is provided "as is," without warranty of any kind,
          express or implied. The entire risk as to the quality and performance
          of the Software is with you.
        </p>
      </section>

      <section>
        <h2>6. Limitation of liability</h2>
        <p>
          In no event shall Ritesh Pandit be liable for any damages arising out
          of the use or inability to use the Software, even if advised of the
          possibility of such damages.
        </p>
      </section>

      <section>
        <h2>7. Third-party services</h2>
        <p>
          The Software may connect to third-party services (GitHub, media providers).
          These services have their own terms and privacy policies. We are not
          responsible for their operation.
        </p>
      </section>

      <section>
        <h2>8. Termination</h2>
        <p>
          Your rights under these terms terminate automatically if you fail to
          comply with any provision. You may stop using the Software at any time.
        </p>
      </section>

      <section>
        <h2>9. Changes</h2>
        <p>
          These terms may be updated when the Software or legal requirements
          change. Continued use after changes constitutes acceptance of the
          new terms.
        </p>
      </section>

      <section>
        <h2>10. Contact</h2>
        <p>
          Questions about these terms:{" "}
          <a href="mailto:panditritesh2001@gmail.com">panditritesh2001@gmail.com</a>.
        </p>
      </section>
    </LegalShell>
  )
}

type ReleaseAsset = {
  id: number
  name: string
  browser_download_url: string
  size: number
  digest?: string
}

type GitHubRelease = {
  id: number
  tag_name: string
  name: string | null
  body: string | null
  html_url: string
  published_at: string
  prerelease: boolean
  assets: ReleaseAsset[]
}

function useReleaseFeed(all = false) {
  const [releases, setReleases] = useState<GitHubRelease[]>([])
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState("")

  useEffect(() => {
    const controller = new AbortController()
    const endpoint = all
      ? "https://api.github.com/repos/Riteshp2001/CineWindows/releases?per_page=20"
      : "https://api.github.com/repos/Riteshp2001/CineWindows/releases/latest"
    fetch(endpoint, {
      signal: controller.signal,
      headers: { Accept: "application/vnd.github+json" },
    })
      .then(async (response) => {
        if (response.status === 404) return []
        if (!response.ok) throw new Error(`GitHub returned ${response.status}`)
        const data = await response.json() as GitHubRelease | GitHubRelease[]
        return Array.isArray(data) ? data : [data]
      })
      .then(setReleases)
      .catch((reason: unknown) => {
        if (reason instanceof DOMException && reason.name === "AbortError") return
        setError(reason instanceof Error ? reason.message : "Could not load releases")
      })
      .finally(() => setLoading(false))
    return () => controller.abort()
  }, [all])

  return { releases, loading, error }
}

function formatBytes(bytes: number) {
  if (!bytes) return "Unknown size"
  return `${(bytes / 1024 / 1024).toFixed(1)} MB`
}

function ReleaseDownloads({ release }: { release: GitHubRelease }) {
  const installer = release.assets.find((asset) => asset.name.endsWith("-win64-setup.exe"))
  const portable = release.assets.find((asset) => asset.name.endsWith("-win64-portable.zip"))
  return (
    <div className="mt-8 grid gap-4 sm:grid-cols-2" data-stagger>
      {[installer, portable].filter(Boolean).map((asset) => asset && (
        <a
          className="feature-card group block outline-none focus-visible:ring-2 focus-visible:ring-accent reveal"
          href={asset.browser_download_url}
          key={asset.id}
        >
          <div className="flex items-start justify-between gap-4">
            <div>
              <p className="font-semibold">{asset.name.includes("setup") ? "Windows installer" : "Portable ZIP"}</p>
              <p className="mt-1 text-sm text-muted">{formatBytes(asset.size)}</p>
            </div>
            <Download className="size-5 text-accent transition-transform group-hover:translate-y-0.5" />
          </div>
          {asset.digest && <code className="mt-5 block break-all text-[11px] text-muted">{asset.digest}</code>}
        </a>
      ))}
    </div>
  )
}

function DownloadPage() {
  const { releases, loading, error } = useReleaseFeed()
  const release = releases[0]
  return (
    <>
      <Header />
      <main id="main-content" className="site-container section-space min-h-[70svh]">
        <p className="eyebrow text-accent reveal">Windows 10 and 11</p>
        <h1 className="mt-4 max-w-3xl text-5xl font-semibold tracking-[-0.055em] sm:text-7xl reveal" style={{ "--reveal-delay": "60ms" } as CSSProperties}>Download CineWindows</h1>
        <p className="mt-6 max-w-2xl text-lg leading-8 text-muted reveal" style={{ "--reveal-delay": "120ms" } as CSSProperties}>Choose the signed installer for automatic setup or the portable archive for a self-contained copy.</p>
        {loading && <p className="mt-12 text-muted">Loading the latest release…</p>}
        {error && <p className="mt-12 text-red-400">{error}</p>}
        {!loading && !error && !release && (
          <div className="feature-card mt-12 max-w-2xl reveal">
            <h2 className="text-xl font-semibold">The first public CineWindows release is being prepared.</h2>
            <p className="mt-3 text-sm leading-6 text-muted">Release downloads will appear here automatically. Do not install assets from the legacy CineWindows repository.</p>
          </div>
        )}
        {release && (
          <section className="mt-12 max-w-4xl reveal" style={{ "--reveal-delay": "180ms" } as CSSProperties}>
            <div className="flex flex-wrap items-baseline gap-x-4 gap-y-2">
              <h2 className="text-2xl font-semibold">{release.name || release.tag_name}</h2>
              <time className="text-sm text-muted">{new Date(release.published_at).toLocaleDateString()}</time>
            </div>
            <ReleaseDownloads release={release} />
            <div className="mt-8 flex flex-wrap gap-3">
              <Button variant="outline" asChild><a href={release.html_url}>Release notes</a></Button>
              <Button variant="ghost" asChild><SiteLink href="/changelog">Full changelog</SiteLink></Button>
            </div>
          </section>
        )}
      </main>
      <Footer />
    </>
  )
}

function ChangelogPage() {
  const { releases, loading, error } = useReleaseFeed(true)
  return (
    <>
      <Header />
      <main id="main-content" className="site-container section-space min-h-[70svh]">
        <p className="eyebrow text-accent reveal">Release history</p>
        <h1 className="mt-4 text-5xl font-semibold tracking-[-0.055em] sm:text-7xl reveal" style={{ "--reveal-delay": "60ms" } as CSSProperties}>Changelog</h1>
        <p className="mt-6 max-w-2xl text-lg leading-8 text-muted reveal" style={{ "--reveal-delay": "120ms" } as CSSProperties}>Every shipped CineWindows build, with release notes and verified artifacts from the canonical CineWindows repository.</p>
        {loading && <p className="mt-12 text-muted">Loading releases…</p>}
        {error && <p className="mt-12 text-red-400">{error}</p>}
        {!loading && !error && releases.length === 0 && <p className="feature-card mt-12 max-w-2xl text-muted reveal">No public CineWindows releases have been published yet.</p>}
        <div className="mt-14 max-w-4xl" data-stagger>
          {releases.map((release) => (
            <article className="border-t border-border py-10 reveal" key={release.id}>
              <div className="flex flex-wrap items-center gap-3">
                <h2 className="text-2xl font-semibold">{release.name || release.tag_name}</h2>
                {release.prerelease && <Badge variant="outline">Pre-release</Badge>}
                <time className="text-sm text-muted">{new Date(release.published_at).toLocaleDateString()}</time>
              </div>
              <p className="mt-5 whitespace-pre-wrap text-sm leading-7 text-muted">{release.body || "No release notes were provided."}</p>
              <ReleaseDownloads release={release} />
            </article>
          ))}
        </div>
      </main>
      <Footer />
    </>
  )
}

const supportTopics = [
  {
    icon: Download,
    title: "Install or update",
    copy: "Help with GitHub Releases installation, portable builds, or update checks.",
  },
  {
    icon: Captions,
    title: "Playback or subtitles",
    copy: "Include the file format, subtitle format, and the exact behavior you see.",
  },
  {
    icon: Globe2,
    title: "Online streams",
    copy: "Share the provider name and a non-private example URL when possible.",
  },
  {
    icon: MousePointer2,
    title: "Controls and settings",
    copy: "Tell us which shortcut, preference, or interaction you expected to work.",
  },
]

function SupportPage() {
  return (
    <LegalShell
      eyebrow="Help"
      title="Support"
      intro="The fastest support request includes your Windows version, CineWindows version, what you tried, and what happened."
    >
      <section className="reveal">
        <h2>Contact support</h2>
        <p>
          Email{" "}
          <a href="mailto:panditritesh2001@gmail.com">panditritesh2001@gmail.com</a>.
          Never include passwords, private media, or payment information.
        </p>
        <div className="mt-6">
          <Button asChild>
            <a href="mailto:panditritesh2001@gmail.com" className="flex items-center gap-2">
              Email support
              <ArrowRight className="size-4" />
            </a>
          </Button>
        </div>
      </section>

      <section className="reveal" style={{ "--reveal-delay": "60ms" } as CSSProperties}>
        <h2>What to include</h2>
        <div className="not-prose mt-6 grid gap-px overflow-hidden rounded-xl border border-border bg-border sm:grid-cols-2">
          {supportTopics.map((topic) => {
            const Icon = topic.icon
            return (
              <div className="bg-background p-5" key={topic.title}>
                <Icon className="size-4 text-muted" />
                <h3 className="mt-3 text-base font-semibold">{topic.title}</h3>
                <p className="mt-1 text-sm leading-6 text-muted">
                  {topic.copy}
                </p>
              </div>
            )
          })}
        </div>
      </section>

      <section className="reveal" style={{ "--reveal-delay": "120ms" } as CSSProperties}>
        <h2>Diagnostic details</h2>
        <ul>
          <li>Windows version and whether it is 64-bit</li>
          <li>CineWindows version from the About dialog</li>
          <li>Media container and codecs, if known</li>
          <li>Whether the problem happens with other files</li>
          <li>Exact steps to reproduce</li>
          <li>A screenshot with private information removed</li>
        </ul>
      </section>

      <section className="reveal" style={{ "--reveal-delay": "180ms" } as CSSProperties}>
        <h2>Response expectations</h2>
        <p>
          Support is provided by the developer on a reasonable-effort basis.
          Complex issues may require additional diagnostic details.
        </p>
      </section>
    </LegalShell>
  )
}

function SidebarLink({ href, label }: { href: string; label: string }) {
  return (
    <a
      href={href}
      className="relative pl-3 text-sm text-muted hover:text-foreground transition-colors before:absolute before:left-0 before:top-1/2 before:-translate-y-1/2 before:h-1 before:w-1 before:rounded-full before:bg-border hover:before:bg-accent"
    >
      {label}
    </a>
  )
}

function DocSection({
  id,
  title,
  children,
}: {
  id: string
  title: string
  children: ReactNode
}) {
  return (
    <section>
      <h2 id={id}>{title}</h2>
      {children}
    </section>
  )
}

function DocsPage() {
  return (
    <>
      <Header />
      <main id="main-content">
        <div className="site-container py-16 sm:py-24">
          <div className="grid gap-10 lg:grid-cols-[0.55fr_1.45fr] lg:gap-16">
            <header className="lg:sticky lg:top-20 lg:self-start reveal">
              <p className="eyebrow text-muted">Documentation</p>
              <h1 className="mt-4 text-3xl font-semibold tracking-[-0.045em] sm:text-4xl">
                How CineWindows works.
              </h1>
              <p className="mt-4 max-w-sm text-sm leading-6 text-muted">
                Everything you need to get playing, from installation to advanced
                configuration and remote control.
              </p>
              <nav className="mt-8" aria-label="Docs sections">
                <div className="flex flex-col gap-1">
                  <span className="text-[10px] font-semibold uppercase tracking-[0.12em] text-muted/60 mb-1">Getting started</span>
                  <SidebarLink href="#installation" label="Installation" />
                  <SidebarLink href="#opening-media" label="Opening media" />
                  <SidebarLink href="#playback-controls" label="Playback controls" />
                  <SidebarLink href="#streaming-urls" label="Streaming URLs" />
                </div>
                <div className="mt-6 flex flex-col gap-1">
                  <span className="text-[10px] font-semibold uppercase tracking-[0.12em] text-muted/60 mb-1">Tracks &amp; playback</span>
                  <SidebarLink href="#subtitles" label="Subtitles" />
                  <SidebarLink href="#audio-tracks" label="Audio tracks" />
                  <SidebarLink href="#video-tracks" label="Video tracks" />
                  <SidebarLink href="#playlists" label="Playlists" />
                </div>
                <div className="mt-6 flex flex-col gap-1">
                  <span className="text-[10px] font-semibold uppercase tracking-[0.12em] text-muted/60 mb-1">Advanced</span>
                  <SidebarLink href="#keyboard-shortcuts" label="Keyboard shortcuts" />
                  <SidebarLink href="#preferences" label="Preferences" />
                  <SidebarLink href="#json-rpc" label="JSON-RPC" />
                  <SidebarLink href="#screenshots" label="Screenshots" />
                  <SidebarLink href="#update-checks" label="Update checks" />
                </div>
              </nav>
            </header>
            <article className="legal-copy reveal" style={{ "--reveal-delay": "120ms" } as CSSProperties}>
              <DocSection id="installation" title="Installation">
                <p>
                  Download CineWindows from{" "}
                  <a href="https://github.com/Riteshp2001/CineWindows/releases" target="_blank" rel="noreferrer">GitHub Releases</a>{" "}
                  for Windows 10 or 11 (64-bit). After installation, launch from
                  the Start menu. The player opens to a blank window ready to
                  accept files.
                </p>
                <p>
                  <strong>Requirements:</strong> Windows 10 22H2 or later,
                  Windows 11. A GPU that supports D3D11VA or DXVA2 is
              recommended for hardware-accelerated playback.
                </p>
              </DocSection>

              <DocSection id="opening-media" title="Opening media">
                <p>
                  CineWindows supports multiple ways to open media:
                </p>
                <ul>
                  <li><strong>Drag and drop</strong> — Drop files, folders, or URLs onto the player window.</li>
                  <li><strong>File dialog</strong> — <code>Ctrl+O</code> or File &gt; Open File to browse.</li>
                  <li><strong>Open directory</strong> — <code>Ctrl+D</code> or File &gt; Open Directory loads every playable file as a playlist.</li>
                  <li><strong>Open URL</strong> — <code>Ctrl+U</code> or File &gt; Open URL for streaming links.</li>
                </ul>
                <p>
                  <strong>Supported formats:</strong> MP4, MKV, AVI, MOV, WMV, FLV, WebM, MP3, FLAC, WAV, M4A, Ogg, M3U, M3U8, and everything FFmpeg understands.
                </p>
                <p>
                  <strong>Supported codecs:</strong> H.264 / AVC, H.265 / HEVC, VP9, AV1, VC-1, MPEG-2, MPEG-4 Part 2, Theora, WMV3 / WMV9, Apple ProRes, DV, HuffYUV, FFV1, MJPEG. Audio codecs: AAC, MP3, Opus, Vorbis, FLAC, PCM, DTS, AC3 / E-AC3, TrueHD.
                </p>
              </DocSection>

              <DocSection id="playback-controls" title="Playback controls">
                <p>
                  The control bar appears at the bottom of the player. It auto-hides after a few seconds during playback and reappears on mouse movement.
                </p>
                <h3>Control bar layout</h3>
                <p>
                  <strong>Left group:</strong> Previous track, Play/Pause, Next track, Volume slider, Mute toggle.
                </p>
                <p>
                  <strong>Center group:</strong> Progress bar (click to seek, hover for timestamp preview), buffered indicator.
                </p>
                <p>
                  <strong>Right group:</strong> Shuffle, Chapters, Options menu, Fullscreen toggle, current time / duration.
                </p>
                <h3>Common actions</h3>
                <ul>
                  <li><strong>Play / Pause</strong> — Space or click the video area.</li>
                  <li><strong>Seek</strong> — Click on the progress bar. Left/Right arrows seek by 5 seconds.</li>
                  <li><strong>Volume</strong> — Scroll wheel or Up/Down arrows. Mute with <code>M</code>.</li>
                  <li><strong>Fullscreen</strong> — Double-click video, press <code>F</code>, or click the fullscreen button. <code>Esc</code> to exit.</li>
                  <li><strong>Speed</strong> — <code>[</code> slows down, <code>]</code> speeds up. <code>\</code> resets to 1x.</li>
                  <li><strong>Frame step</strong> — <code>.</code> advances one frame while paused. <code>,</code> steps back.</li>
                </ul>
                <h3>Track selection</h3>
                <p>
                  The secondary control row (between the progress bar and main controls) shows buttons for Subtitles, Audio, Video, and Playlist. Click to cycle tracks or open the track selection menu.
                </p>
              </DocSection>

              <DocSection id="streaming-urls" title="Streaming URLs">
                <p>
                  CineWindows can stream media from URLs using its bundled yt-dlp integration.
                </p>
                <h3>How to open a stream</h3>
                <ul>
                  <li>Press <code>Ctrl+U</code> or go to File &gt; Open URL.</li>
                  <li>Paste the URL and press Enter. CineWindows resolves the stream and starts playback.</li>
                </ul>
                <h3>Supported providers</h3>
                <p>
                  yt-dlp supports hundreds of sites including YouTube, Vimeo, Dailymotion, Twitch, Bilibili, Nico Nico, and many more. Availability depends on the provider, your network, and regional restrictions.
                </p>
                <h3>Limitations</h3>
                <ul>
                  <li>DRM-protected content (e.g., Netflix, Hulu, Disney+) cannot be played.</li>
                  <li>Provider rate limits or geo-blocking may prevent access.</li>
                  <li>Livestreams are supported but may buffer depending on the provider.</li>
                </ul>
                <h3>Example URLs</h3>
                <p>YouTube video:</p>
                <pre className="not-prose rounded-lg bg-surface border border-border p-3 text-sm text-muted overflow-x-auto">https://www.youtube.com/watch?v=dQw4w9WgXcQ</pre>
                <p>Direct stream (HLS):</p>
                <pre className="not-prose rounded-lg bg-surface border border-border p-3 text-sm text-muted overflow-x-auto">https://example.com/stream.m3u8</pre>
                <p>Twitch channel:</p>
                <pre className="not-prose rounded-lg bg-surface border border-border p-3 text-sm text-muted overflow-x-auto">https://www.twitch.tv/example</pre>
              </DocSection>

              <DocSection id="subtitles" title="Subtitles">
                <p>
                  CineWindows has comprehensive subtitle support. Files with matching filenames are auto-loaded when you open a video. For example, <code>movie.mkv</code> paired with <code>movie.srt</code> in the same folder.
                </p>
                <h3>Loading subtitles</h3>
                <ul>
                  <li><strong>Auto-load</strong> — matching subtitle files are picked up automatically, including external files and embedded tracks.</li>
                  <li><strong>Manual load</strong> — Use Subtitle &gt; Load Subtitle File (<code>Ctrl+L</code>) to browse for a file.</li>
                  <li><strong>Search</strong> — CineWindows can detect subtitles in subfolders near the media file.</li>
                </ul>
                <h3>Supported formats</h3>
                <p>External: SRT, ASS, SSA, VTT, SUB, IDX, PGS (MKS), SMI, RT, LRC, PJS, JACOSub, MicroDVD, MPL2, TMP, SubViewer, SubRip, SubStation Alpha, TTML, TX3G, WebVTT.</p>
                <p>Embedded: PGS, VOBSUB, HDMV PGS, DVB Sub, Teletext, Closed Captions (EIA-608 / EIA-708).</p>
                <h3>Track cycling</h3>
                <p>
                  Click the subtitles button in the control bar to cycle through available tracks. The Subtitle menu shows all loaded tracks with the active one marked. You can also cycle backwards with <code>Shift+J</code> (forward: <code>J</code>).
                </p>
                <h3>Subtitle preferences</h3>
                <ul>
                  <li><strong>Language priority</strong> — Set preferred subtitle languages. Higher priority tracks are selected automatically.</li>
                  <li><strong>Font &amp; size</strong> — Choose the render font, scale, and line height. ASS subtitles use their own styling by default but can be overridden.</li>
                  <li><strong>Position</strong> — Vertical offset from the bottom of the screen.</li>
                  <li><strong>Timing</strong> — Apply a delay in seconds if subtitles are out of sync (<code>Z</code> / <code>X</code> to adjust in 100ms increments).</li>
                  <li><strong>Style override</strong> — Force plain rendering (ignore ASS styling), toggle bold/italic, and set border/background color.</li>
                </ul>
              </DocSection>

              <DocSection id="audio-tracks" title="Audio tracks">
                <p>
                  CineWindows can play files with multiple audio tracks and switch between them during playback. The default track is selected based on your language preferences.
                </p>
                <h3>Track switching</h3>
                <ul>
                  <li>Click the audio button in the control bar to cycle through available tracks.</li>
                  <li>The Audio menu lists all tracks with codec, channel count, and language information.</li>
                  <li>Cycle backwards with <code>Shift+K</code>.</li>
                </ul>
                <h3>Audio device selection</h3>
                <p>
                  In Preferences &gt; Audio, you can select the output device. The default is the system default device. WASAPI is used for low-latency audio output.
                </p>
                <h3>Volume normalization</h3>
                <p>
                  Enable audio normalization (EBU R128 / ReplayGain) in Preferences to level out volume differences between tracks and files. This is especially useful for playlists with mixed sources.
                </p>
              </DocSection>

              <DocSection id="video-tracks" title="Video tracks">
                <p>
                  Files with multiple video tracks (e.g., multi-angle discs or different encodes) let you switch between them during playback.
                </p>
                <h3>Track switching</h3>
                <ul>
                  <li>Click the video button in the control bar to cycle through available video tracks.</li>
                  <li>Some formats may contain additional streams like thumbnails or cover art. These are not shown as playback tracks.</li>
                </ul>
                <h3>Video adjustments</h3>
                <ul>
                  <li><strong>Brightness, contrast, saturation, gamma, hue</strong> — Adjust in real-time from the Video menu or via keyboard shortcuts (default: <code>1</code> through <code>5</code> to cycle through parameters, <code>7</code>/<code>8</code> to adjust).</li>
                  <li><strong>Deinterlacing</strong> — Enable in Preferences &gt; Video when playing interlaced content. Supported methods: D3D11VA deinterlace, yadif, bwdif.</li>
                  <li><strong>Video rotation</strong> — Rotate 90, 180, or 270 degrees. Useful for phone-recorded videos. Shortcuts: <code>Shift+R</code> cycles through rotations.</li>
                  <li><strong>Aspect ratio</strong> — Override the display aspect ratio. Options: Default, 4:3, 16:9, 16:10, 21:9, Square, and Custom.</li>
                  <li><strong>Crop</strong> — Automatically detect and remove black bars. <code>Shift+C</code> toggles auto-crop.</li>
                </ul>
              </DocSection>

              <DocSection id="playlists" title="Playlists">
                <p>
                  The playlist panel shows the current playback queue. Open it with Playlist &gt; Show Playlist (<code>P</code> key) or click the playlist button in the control bar.
                </p>
                <h3>Managing playlists</h3>
                <ul>
                  <li><strong>Reorder</strong> — Drag items in the playlist panel to rearrange them.</li>
                  <li><strong>Remove</strong> — Right-click and select Remove, or select and press <code>Delete</code>.</li>
                  <li><strong>Add files</strong> — Drag new files onto the playlist panel or use File &gt; Append File.</li>
                  <li><strong>Save</strong> — Playlist &gt; Save saves the queue as an M3U file for later use.</li>
                  <li><strong>Load</strong> — Open an M3U/M3U8 file via File &gt; Open File to restore a saved playlist.</li>
                </ul>
                <h3>Playback modes</h3>
                <ul>
                  <li><strong>Shuffle</strong> — Randomizes playback order. Click the shuffle button or use Playlist &gt; Shuffle. Toggle off to return to sequential order.</li>
                  <li><strong>Repeat</strong> — Repeats the current playlist. Available modes: Repeat All, Repeat One (loop current file).</li>
                  <li><strong>Loop section</strong> — Set loop points with <code>[</code> (start) and <code>]</code> (end). Disable with <code>Alt+[</code>.</li>
                </ul>
                <h3>Example: Save and restore</h3>
                <pre className="not-prose rounded-lg bg-surface border border-border p-3 text-sm text-muted overflow-x-auto">1. Open several files via File &gt; Open Directory
2. Reorder tracks by dragging in the playlist panel
3. Playlist &gt; Save — name it "my_playlist.m3u"
4. Next session: File &gt; Open File — select "my_playlist.m3u"</pre>
              </DocSection>

              <DocSection id="keyboard-shortcuts" title="Keyboard shortcuts">
                <p>
                  All key bindings are customizable in Preferences &gt; Shortcuts. You can change individual keys or import a complete mpv <code>input.conf</code> file. Below are the default bindings.
                </p>
                <h3>Playback</h3>
                <table className="not-prose w-full text-sm border-collapse">
                  <thead>
                    <tr className="border-b border-border">
                      <th className="py-2 pr-4 text-left text-muted font-medium">Key</th>
                      <th className="py-2 text-left text-muted font-medium">Action</th>
                    </tr>
                  </thead>
                  <tbody>
                    {[
                      ["Space", "Play / Pause"],
                      ["Left / Right", "Seek backward / forward 5s"],
                      ["Shift+Left / Right", "Seek backward / forward 30s"],
                      ["Ctrl+Left / Right", "Seek to previous / next chapter"],
                      ["Up / Down", "Volume + / -"],
                      ["M", "Mute toggle"],
                      ["[ / ]", "Speed - / + 0.1x"],
                      ["\\", "Reset speed to 1x"],
                      [".", "Frame step forward (paused)"],
                      [",", "Frame step backward (paused)"],
                    ].map(([key, action]) => (
                      <tr key={key} className="border-b border-border">
                        <td className="py-1.5 pr-4 font-mono text-xs">{key}</td>
                        <td className="py-1.5 text-muted">{action}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
                <h3>Navigation</h3>
                <table className="not-prose w-full text-sm border-collapse">
                  <thead>
                    <tr className="border-b border-border">
                      <th className="py-2 pr-4 text-left text-muted font-medium">Key</th>
                      <th className="py-2 text-left text-muted font-medium">Action</th>
                    </tr>
                  </thead>
                  <tbody>
                    {[
                      ["F", "Toggle fullscreen"],
                      ["P", "Toggle playlist panel"],
                      ["T", "Toggle title bar / cinema mode"],
                      ["Esc", "Exit fullscreen / close dialogs"],
                      ["Ctrl+O", "Open file dialog"],
                      ["Ctrl+D", "Open directory"],
                      ["Ctrl+U", "Open URL"],
                    ].map(([key, action]) => (
                      <tr key={key} className="border-b border-border">
                        <td className="py-1.5 pr-4 font-mono text-xs">{key}</td>
                        <td className="py-1.5 text-muted">{action}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
                <h3>Subtitles &amp; audio</h3>
                <table className="not-prose w-full text-sm border-collapse">
                  <thead>
                    <tr className="border-b border-border">
                      <th className="py-2 pr-4 text-left text-muted font-medium">Key</th>
                      <th className="py-2 text-left text-muted font-medium">Action</th>
                    </tr>
                  </thead>
                  <tbody>
                    {[
                      ["J", "Next subtitle track"],
                      ["Shift+J", "Previous subtitle track"],
                      ["K", "Next audio track"],
                      ["Shift+K", "Previous audio track"],
                      ["Z / X", "Subtitle delay - / + 100ms"],
                      ["R", "Cycle subtitle rotation"],
                      ["V", "Toggle video track"],
                    ].map(([key, action]) => (
                      <tr key={key} className="border-b border-border">
                        <td className="py-1.5 pr-4 font-mono text-xs">{key}</td>
                        <td className="py-1.5 text-muted">{action}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
                <h3>Video</h3>
                <table className="not-prose w-full text-sm border-collapse">
                  <thead>
                    <tr className="border-b border-border">
                      <th className="py-2 pr-4 text-left text-muted font-medium">Key</th>
                      <th className="py-2 text-left text-muted font-medium">Action</th>
                    </tr>
                  </thead>
                  <tbody>
                    {[
                      ["1 - 5", "Cycle through video equalizer parameters"],
                      ["7 / 8", "Decrease / increase current parameter"],
                      ["Shift+R", "Cycle video rotation"],
                      ["Shift+C", "Toggle auto-crop"],
                      ["S", "Take screenshot"],
                      ["Ctrl+S", "Take screenshot (without subtitles)"],
                    ].map(([key, action]) => (
                      <tr key={key} className="border-b border-border">
                        <td className="py-1.5 pr-4 font-mono text-xs">{key}</td>
                        <td className="py-1.5 text-muted">{action}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
                <h3>Importing mpv configuration</h3>
                <p>
                  If you already use mpv, you can import your <code>input.conf</code> in Preferences &gt; Shortcuts &gt; Import. CineWindows parses the file and applies your bindings. Invalid or unsupported commands are reported so you can review them. This makes migration from mpv nearly instant.
                </p>
              </DocSection>

              <DocSection id="preferences" title="Preferences">
                <p>
                  Open Preferences from the main menu or with <code>Ctrl+,</code>. The current release keeps common settings in one focused scrolling window.
                </p>
                <h3>Playback</h3>
                <ul>
                  <li><strong>Session restore</strong> — Restore the saved queue, selected item, and playback position from SQLite.</li>
                  <li><strong>Remember position</strong> — Keep per-file resume progress and local History.</li>
                  <li><strong>Hardware decoding</strong> — Enable the compatible mpv hardware-decoding mode.</li>
                  <li><strong>Normalization</strong> — Apply loudness normalization.</li>
                </ul>
                <h3>Subtitles</h3>
                <ul>
                  <li><strong>Language priority</strong> — Preferred subtitle languages.</li>
                  <li><strong>Typography</strong> — Font, scale, foreground color, and optional background.</li>
                  <li><strong>OpenSubtitles</strong> — Enter an API key only when using the optional subtitle-search dialog.</li>
                </ul>
                <h3>Interface</h3>
                <ul>
                  <li><strong>Theme</strong> — Switch between the shared dark and light palettes and choose an accent color.</li>
                  <li><strong>Reduced motion</strong> — Disable non-essential panel and page transitions.</li>
                  <li><strong>Language</strong> — Select the translated UI locale.</li>
                  <li><strong>Input</strong> — Configure left/right click behavior and remappable shortcuts.</li>
                  <li><strong>Updates</strong> — Enable startup checks, download with progress, and install after verification.</li>
                </ul>
              </DocSection>

              <DocSection id="json-rpc" title="JSON-RPC">
                <p>
                  CineWindows exposes an mpv-compatible JSON-RPC interface over TCP, allowing external applications to control playback, query properties, and observe changes in real-time.
                </p>
                <h3>Setup</h3>
                <ol>
                  <li>Launch CineWindows with <code>--ipc-server=&lt;port&gt;</code>, for example <code>--ipc-server=32321</code>.</li>
                  <li>The raw mpv-compatible server binds to <code>127.0.0.1</code> only and is disabled unless explicitly requested.</li>
                  <li>For phone control, use Companion Remote in the app menu. It is opt-in and protected by a rotating pairing code.</li>
                </ol>
                <h3>Example: Get file path</h3>
                <p>Commands are newline-delimited JSON over raw TCP, not HTTP:</p>
                <pre className="not-prose rounded-lg bg-surface border border-border p-3 text-sm text-muted overflow-x-auto">{`echo '{"command":["get_property","path"],"request_id":1}' | nc -q0 127.0.0.1 32321`}</pre>
                <h3>Example: Toggle play</h3>
                <pre className="not-prose rounded-lg bg-surface border border-border p-3 text-sm text-muted overflow-x-auto">{`{"command":["cycle","pause"],"request_id":2}`}</pre>
                <h3>Example: Seek to position</h3>
                <pre className="not-prose rounded-lg bg-surface border border-border p-3 text-sm text-muted overflow-x-auto">{`{"command":["set_property","time-pos",120.5],"request_id":3}`}</pre>
                <h3>Example: Observe property changes</h3>
                <p>
                  Use <code>observe_property</code> to receive updates when a property changes. The server pushes notifications as JSON lines:
                </p>
                <pre className="not-prose rounded-lg bg-surface border border-border p-3 text-sm text-muted overflow-x-auto">{`{"jsonrpc":"2.0","method":"observe_property","params":[1,"time-pos"]}
{"jsonrpc":"2.0","method":"observe_property","params":[2,"pause"]}`}</pre>
                <h3>Supported methods</h3>
                <p>
                  The JSON-RPC implementation mirrors mpv's specification. Supported methods include:
                </p>
                <ul>
                  <li><code>get_property</code>, <code>set_property</code>, <code>observe_property</code>, <code>unobserve_property</code></li>
                  <li><code>get_property_string</code>, <code>set_property_string</code></li>
                  <li><code>cycle</code>, <code>cycle_values</code></li>
                  <li><code>command</code>, <code>commandv</code>, <code>command_native</code></li>
                  <li><code>keypress</code>, <code>keydown</code>, <code>keyup</code></li>
                  <li><code>multiply</code>, <code>add</code></li>
                  <li><code>screenshot</code>, <code>screenshot_to_file</code></li>
                  <li><code>loadfile</code>, <code>loadlist</code>, <code>playlist_next</code>, <code>playlist_prev</code>, <code>playlist_clear</code></li>
                  <li><code>seek</code>, <code>revert_seek</code></li>
                  <li><code>set</code>, <code>change_list</code></li>
                  <li><code>get_version</code>, <code>get_properties</code></li>
                </ul>
              </DocSection>

              <DocSection id="screenshots" title="Screenshots">
                <p>Capture the current video frame with <code>S</code>. Use <code>Shift+S</code> for video without subtitles and <code>Ctrl+S</code> for the rendered window.</p>
                <ul>
                  <li>Screenshots are written to <strong>Pictures/CineWindows</strong>.</li>
                  <li>The default filename template is <code>cine_%n</code>.</li>
                  <li>Advanced users can customize mpv screenshot options in their mpv configuration.</li>
                </ul>
              </DocSection>

              <DocSection id="update-checks" title="Update checks">
                <p>
                  CineWindows checks the canonical CineWindows GitHub releases feed on startup by default. Automatic checks can be disabled in Preferences.
                </p>
                <h3>How it works</h3>
                <ul>
                  <li>The app selects only the matching CineWindows 64-bit installer asset.</li>
                  <li>Downloads show progress and are verified against GitHub's SHA-256 digest when available.</li>
                  <li>Installation starts only after you choose Install and Restart.</li>
                  <li>No account or personal data is sent. GitHub receives a standard HTTP request (IP, timestamp, user-agent).</li>
                </ul>
                <h3>Updating</h3>
                <ul>
                  <li>Standalone builds can download and launch the signed installer in-app.</li>
                  <li>Portable users can get the latest ZIP from the <SiteLink href="/download">Download page</SiteLink>.</li>
                </ul>
              </DocSection>
            </article>
          </div>
        </div>
      </main>
      <Footer />
    </>
  )
}

function NotFoundPage() {
  return (
    <>
      <Header />
      <main
        id="main-content"
        className="grid min-h-[70svh] place-items-center px-6 text-center"
      >
        <div className="hero-enter">
          <WandSparkles className="mx-auto size-6 text-muted" />
          <p className="eyebrow mt-5 text-muted">404 — Page Not Found</p>
          <h1 className="mt-3 text-4xl font-semibold tracking-[-0.045em]">
            This frame isn't in the cut.
          </h1>
          <Button className="mt-6" asChild>
            <SiteLink href="/">
              Back to CineWindows
              <ArrowRight className="size-4" />
            </SiteLink>
          </Button>
        </div>
      </main>
      <Footer />
    </>
  )
}

export default function App() {
  const [path, setPath] = useState(window.location.pathname)

  useEffect(() => {
    const updatePath = () => setPath(window.location.pathname)
    window.addEventListener("popstate", updatePath)
    return () => window.removeEventListener("popstate", updatePath)
  }, [])

  useEffect(() => {
    const page = metadata[path] ?? {
      title: "Page not found — CineWindows",
      description: "The requested CineWindows page could not be found.",
    }
    document.title = page.title
    document
      .querySelector('meta[name="description"]')
      ?.setAttribute("content", page.description)

    const siteUrl = configuredSiteUrl || window.location.origin
    const canonicalUrl = `${siteUrl}${path === "/" ? "/" : path}`
    document
      .querySelector('link[rel="canonical"]')
      ?.setAttribute("href", canonicalUrl)
    document
      .querySelector('meta[property="og:url"]')
      ?.setAttribute("content", canonicalUrl)
    document
      .querySelector('meta[property="og:title"]')
      ?.setAttribute("content", page.title)
    document
      .querySelector('meta[property="og:description"]')
      ?.setAttribute("content", page.description)
    document
      .querySelector('meta[name="twitter:title"]')
      ?.setAttribute("content", page.title)
    document
      .querySelector('meta[name="twitter:description"]')
      ?.setAttribute("content", page.description)
  }, [path])

  useReveals(path)

  if (path === "/docs" || path === "/docs/") return <DocsPage />
  if (path.startsWith("/docs")) {
    window.location.assign(path)
    return null
  }
  if (path === "/download") return <DownloadPage />
  if (path === "/changelog") return <ChangelogPage />
  if (path === "/privacy") return <PrivacyPage />
  if (path === "/terms") return <TermsPage />
  if (path === "/support") return <SupportPage />
  if (path === "/") return <HomePage />
  return <NotFoundPage />
}
