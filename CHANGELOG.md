# Changelog

All notable changes to CineWindows are documented here. Downloadable packages
and their checksums are available from
[GitHub Releases](https://github.com/Riteshp2001/CineWindows/releases).

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

