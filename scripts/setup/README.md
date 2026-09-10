# Setup Scripts

Repeatable platform dependency setup for local development and CI.

| Script | Purpose |
| --- | --- |
| `bootstrap_libmpv_windows.ps1` | Download and prepare the standalone Windows libmpv runtime. |
| `bootstrap_media_tools_windows.ps1` | Download FFmpeg, ffprobe, and yt-dlp for Windows packages. |
| `bootstrap_mpvqt_windows.ps1` | Build and install MpvQt against the prepared Windows dependencies. |
| `bootstrap_linux.sh` | Install supported Debian/Ubuntu build dependencies. |
| `bootstrap_macos.sh` | Install supported macOS build and packaging dependencies. |

These scripts are operational setup tools, not generated artifacts. Keep download verification,
version pins, and CI invocations synchronized when dependencies change.

## Related

- [Scripts index](../README.md)
- [Build instructions](../../README.md#build)
- [License](../../LICENSE.md)