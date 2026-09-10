import { defineConfig } from "blume";

export default defineConfig({
  title: "CineWindows",
  description: "Modern, high-performance media player for Windows. Built with Qt 6, QML, C++20, and libmpv.",
  basePath: "/docs",
  deployment: {
    site: "https://cinewindows.vercel.app",
  },
  logo: {
    image: {
      dark: "/logo-dark.svg",
      light: "/logo-light.svg",
      alt: "CineWindows logo",
    },
  },
  content: {
    root: "content",
  },
  navigation: {
    sidebar: [
      {
        label: "Getting Started",
        items: [
          { label: "Overview", href: "/guides/overview" },
          { label: "Installation", href: "/guides/installation" },
          { label: "Quick Start", href: "/guides/quick-start" },
        ],
      },
      {
        label: "Playback & UI",
        items: [
          { label: "User Interface", href: "/guides/user-interface" },
          { label: "Playback Controls", href: "/guides/playback" },
          { label: "Playlists", href: "/guides/playlists" },
          { label: "Subtitles", href: "/guides/subtitles" },
          { label: "Audio & Filters", href: "/guides/audio-filters" },
          { label: "Keyboard Shortcuts", href: "/guides/keyboard-shortcuts" },
        ],
      },
      {
        label: "Media & Features",
        items: [
          { label: "Media Hub", href: "/guides/media-hub" },
          { label: "Streaming & URLs", href: "/guides/streaming" },
          { label: "Picture-in-Picture", href: "/guides/picture-in-picture" },
        ],
      },
      {
        label: "Integrations & Remote",
        items: [
          { label: "Remote Control", href: "/guides/remote-control" },
          { label: "Casting (DLNA)", href: "/guides/casting" },
          { label: "IPC Protocol", href: "/guides/ipc-protocol" },
        ],
      },
      {
        label: "Settings & Support",
        items: [
          { label: "Preferences", href: "/guides/preferences" },
          { label: "Localization", href: "/guides/localization" },
          { label: "Troubleshooting", href: "/guides/troubleshooting" },
        ],
      },
      {
        label: "Technical Reference",
        items: [
          { label: "CLI & Command Line", href: "/reference/cli" },
          { label: "Key Bindings", href: "/reference/key-bindings" },
          { label: "Configuration", href: "/reference/configuration" },
          { label: "IPC Protocol Reference", href: "/reference/ipc-protocol" },
        ],
      },
    ],
  },
});
