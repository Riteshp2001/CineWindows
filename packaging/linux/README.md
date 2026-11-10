# Linux Packaging

Desktop integration metadata and the output location for Linux packages.

## Scope

Use `scripts/package/deploy_linux.sh` to produce the AppImage. The separate
Flatpak manifest and bundle instructions live under `packaging/flatpak/`. Do
not commit generated artifacts.

AppImage releases use Ubuntu 22.04 (glibc 2.35) and GCC 12. Packaging rejects
ELF dependencies requiring a newer glibc. Build and package on that baseline;
copying libraries from a newer distribution does not lower their requirements.

PipeWire is deployed explicitly, including its client configuration, client
modules, SPA support/audio-conversion plugins, and their shared dependencies.
`pipewire-hook.sh` locates these inside the AppImage at startup. The C++ runtime
is bundled; glibc and the host graphics driver remain system dependencies.

## Related

- [Parent directory](../README.md)
- [Project overview](../../README.md)
- [License](../../LICENSE.md)
