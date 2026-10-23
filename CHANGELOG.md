# Changelog

All notable changes to CineWindows are documented here. Downloadable packages
and their checksums are available from
[GitHub Releases](https://github.com/Riteshp2001/CineWindows/releases).

## [0.0.4] - 2026-10-23

CineWindows 0.0.4 improves timeline preview responsiveness and web-stream handling.

## Improvements

- Thumbnail previews use mpv event notifications instead of frequent polling and keep completed frames cached.
- Network previews inherit the player's cookies, request headers, proxy, and TLS settings.
- QML metadata now includes MpvQt's anonymous base type on supported older ECM builds.

## Verification

- Covers local previews, HTTP byte ranges, cookies, request headers, redirects, and cached-frame delivery.
- Compares first-frame latency against upstream thumbfast using the same 1080p H.264 fixture.
- Runs playback and configuration regression tests with the bundled mpv library.

Previews require seekable media. Live streams without timeline seeking and protected media that disallow seeking may not provide hover thumbnails.

[Compare changes since v0.0.3](https://github.com/Riteshp2001/CineWindows/compare/v0.0.3...v0.0.4)

## [0.0.3] - 2026-10-23

CineWindows 0.0.3 fixes subtitle selection and makes playback-position saving
finish before the player closes.

## Fixes

- Selecting **None** in the subtitle menu now calls the embedded player's
  supported track-disable API. The same correction applies to audio and video
  track disabling.
- When **Save Video Position on Close** is enabled, closing the window waits
  for mpv's watch-later state to be written before tearing down the player.
- The embedded player uses an explicit CineWindows watch-later directory and
  applies the resume preference when opening media.

## Interface

- Consistent divider lines and spacing in grouped Preferences, Shortcuts,
  Equalizer, and video-options sections.
- Shared scrollbar gutters keep scrollbars clear of controls and text.
- Swatch-first color selection with a custom editor and transparency support
  for subtitle backgrounds.
- Streamlined URL and subtitle-search dialogs and searchable keyboard shortcuts.

## Verification

- Adds real-libmpv regression coverage for the production subtitle-selection
  action and writing/restoring saved playback positions.
- Runs playback and configuration regression tests on Linux before publishing
  AppImage release artifacts.

Position saving remains optional: enable **Save Video Position on Close** in
Preferences to resume files opened again later. Credentialed integrations and
casting to external hardware are not covered by these automated tests.

[Compare changes since v0.0.2](https://github.com/Riteshp2001/CineWindows/compare/v0.0.2...v0.0.3)

## [0.0.2] - 2026-09-15

CineWindows 0.0.2 adds saved streaming-quality preferences, improves everyday
controls, and fixes Windows portable packaging.

### Added

- **YouTube Quality:** save Automatic or a resolution limit from 144p through
  2160p (4K) in Preferences or the YouTube Search menu. Both selectors use the
  same setting and Solar 4K icons. Lower available resolutions and combined
  video/audio streams are supported as fallbacks. References #9.
- **Open Media:** browse files or paste local paths and URLs, including multiple
  entries and quoted paths, through the player picker.
- **Optional TMDB metadata:** enrich the local library with movie and TV matches,
  artwork, search, and categories using a user-provided API credential. References #3.
- **Local diagnostics:** route Qt, QML, mpv, and service messages through QtLogger;
  inspect system, graphics, and display information; open the log folder or export
  a diagnostic report from Preferences. Each running instance has its own log slot.

### Improved

- Slim, themed volume, equalizer, and HDR sliders with round thumbs, keyboard
  focus feedback, reference markers, and correct vertical and mirrored positions.
- Evenly spaced equalizer bands with zero-centered gain fills, a visible volume
  percentage, and a readable companion-remote address field in both themes.
- Solar Subtitles Linear/Bold icons consistently reflect inactive, active, and
  hovered states in the main player and picture-in-picture views.
- Shared theme colors for the media picker, buttons, and menus.
- Human-readable subtitle language names with ISO 639 codes. References #6.

### Fixed

- Main-window startup no longer depends on an invisible video's first rendered
  frame. Startup placement uses one monitor's work area rather than the entire
  virtual desktop, with later monitor-change bounds checks retained.
- Concurrent instances use separate temporary mpv configuration files and log
  files; library initialization avoids unnecessary schema work and migration races.
- Native timeline previews no longer free mpv-owned screenshot data. Captures
  wait for frame readiness, coalesce hover requests, retain a bounded cache, and
  reuse a quiet worker before releasing it after inactivity.
- Linux desktop identity and matching raster/scalable icons are installed together.
- Website download and changelog cards reveal correctly when release data arrives
  asynchronously; FAQ navigation and wide-screen marquee coverage are corrected.
- QtLogger's upstream headers are treated as system headers while application
  conversion and shadow warnings remain enabled.
- Windows `.zip` downloads now contain actual ZIP data, rather than mislabeled
  7-Zip data. The portable package and installer include matching Qt runtime
  libraries, plugins, and QML modules. Packaging checks reject missing runtime
  dependencies and unreadable archives.

### Downloads And Notes

- Windows x86_64: portable ZIP and NSIS installer. Both include the core playback
  runtime. The separate media-tools ZIP contains yt-dlp, FFmpeg, and FFprobe for
  online extraction and related integrations; place those executables beside
  `CineWindows.exe`. No Qt installation or DLL-download website is needed.
- Linux x86_64: AppImage and Flatpak. macOS: separate arm64 and x86_64 DMGs.
- SHA-256 manifests accompany the packages. Windows executables are unsigned.
- Quality preferences apply to the next URL opened, not streams already playing.
  Availability still depends on the source.
- Diagnostic exports may contain local media paths; review reports before sharing.

[Compare changes since v0.0.1](https://github.com/Riteshp2001/CineWindows/compare/v0.0.1...v0.0.2)

## [0.0.1] - 2026-09-10

The first public CineWindows release delivers a native desktop media player
built with Qt 6, QML, C++20, libmpv, and FFmpeg. It is designed for local media,
streaming, detailed playback control, and a focused desktop viewing experience.

### Playback

- Play video and audio formats supported by FFmpeg through the libmpv engine.
- Open individual files, folders, playlists, subtitles, external audio tracks,
	and supported network URLs.
- Configure hardware decoding, playback speed from 0.25x to 4.0x, precise seek
	intervals, frame stepping, chapter navigation, and track selection.
- Use file and playlist looping, shuffle, A-B repeat, resume positions, and
	automatic session restoration.
- Drag media, folders, and subtitle files directly into the player.
- Capture screenshots with or without subtitles, the application window, or
	every rendered frame.

### Subtitles, Audio, and Video

- Load embedded or external SRT, ASS, SSA, VTT, and other subtitle formats,
	with automatic discovery and manual track selection.
- Customize subtitle fonts, size, colors, borders, shadows, position, timing,
	and secondary subtitle tracks.
- Search for subtitles through the optional OpenSubtitles integration.
- Select audio tracks, adjust audio delay, normalize loudness, and configure a
	10-band equalizer with presets.
- Display waveform and spectrum audio visualizations.
- Adjust brightness, contrast, gamma, saturation, hue, aspect ratio, crop,
	zoom, pan, rotation, and horizontal or vertical flipping.
- Configure deinterlacing, debanding, stereo 3D conversion, and HDR tone
	mapping with target peak controls.

### Media Hub and Interface

- Browse the filesystem and index selected folders in the Media Hub.
- Search the local library, mark favorites, revisit recent items, and review
	playback history with resume progress and local watch statistics.
- Generate and cache media thumbnails for library items and seek previews.
- Manage playlists in a side drawer with reordering, removal, shuffle,
	repeat, and M3U/M3U8 import and export.
- Use a frameless desktop window with light and dark palettes, custom accent
	colors, on-screen playback feedback, reduced-motion support, and responsive
	controls.
- Switch to picture-in-picture or mini-player layouts with always-on-top and
	compact playback controls.
- Configure mouse behavior and remap keyboard shortcuts from Preferences.
- Use the translated interface across more than 50 bundled locales.

### Streaming and Integrations

- Open supported streaming sites and direct media URLs through yt-dlp.
- Search YouTube from the application and play results with the normal player
	controls.
- Discover compatible DLNA renderers on the local network and cast direct
	media URLs.
- Pair a browser on the local network through a QR code and token-protected
	companion remote for playback, volume, seeking, and folder browsing.
- Enable the optional localhost JSON-RPC server for mpv-compatible automation.
- Run the interactive CLI mode for newline-delimited JSON-RPC or raw mpv
	commands over standard input and output.

### Reliability, Privacy, and Updates

- Keep settings, library data, history, favorites, thumbnails, and playback
	state on the local device.
- Operate local playback without an account, advertising, or analytics.
- Keep IPC disabled unless explicitly requested and bind it to localhost.
- Protect the companion remote with a short-lived pairing token and bounded
	request handling.
- Check GitHub Releases for updates and verify supported installer downloads
	against their published SHA-256 digest when available.
- Include the CineWindows Community License and third-party attribution in
	packaged distributions.

### Packages and Documentation

- Windows x86_64: NSIS installer and portable ZIP.
- Linux x86_64: AppImage and Flatpak bundle using application ID
	`com.gyrolet.CineWindows`.
- macOS arm64 and x86_64: architecture-specific DMG packages.
- Publish SHA-256 checksum manifests with release artifacts.
- Provide a project website, installation and feature guides, configuration
	reference, keyboard shortcut reference, and IPC protocol documentation.

### License

CineWindows is source-available under the CineWindows Community License for
personal, non-commercial use. See [LICENSE.md](LICENSE.md) for the complete
terms and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for dependency and
asset attribution.

[0.0.1]: https://github.com/Riteshp2001/CineWindows/releases/tag/v0.0.1
[0.0.2]: https://github.com/Riteshp2001/CineWindows/releases/tag/v0.0.2
[0.0.3]: https://github.com/Riteshp2001/CineWindows/releases/tag/v0.0.3
[0.0.4]: https://github.com/Riteshp2001/CineWindows/releases/tag/v0.0.4
