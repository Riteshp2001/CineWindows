# Flatpak Packaging

The Flatpak package uses the application ID `com.gyrolet.CineWindows` and the
KDE runtime. It builds pinned libmpv, MpvQt, and yt-dlp dependencies inside the
sandbox rather than linking to libraries from the build host.

## Build

Install Flatpak and flatpak-builder, add the Flathub remote, and install the
runtime and SDK declared by the manifest. From the repository root, run:

```bash
bash scripts/package/deploy_flatpak.sh
```

The resulting `CineWindows-x86_64.flatpak` bundle is written to
`packaging/flatpak/artifacts/`. Generated build directories and bundles are not
committed.

## Permissions

The sandbox permits audio, GPU acceleration, Wayland/X11 fallback, network
access for streaming and the companion remote, read-only host media access, and
write access to the user's home directory for playlists and screenshots.

The Flatpak bundle remains governed by the
[CineWindows Community License](../../LICENSE.md).