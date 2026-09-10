# Packaging Scripts

Release staging, verification, and platform package generation.

| Script | Purpose |
| --- | --- |
| `deploy_windows.ps1` | Stage Windows runtime files and create portable and NSIS packages. |
| `verify_windows_deployment.ps1` | Independently verify staged PE dependencies and startup behavior. |
| `deploy_linux.sh` | Build the Linux AppImage. |
| `deploy_flatpak.sh` | Build and verify the Linux Flatpak bundle. |
| `deploy_macos.sh` | Bundle the macOS application and create a DMG. |

The Windows verifier remains separate because it is a reusable validation boundary that accepts any
staging directory, even though the deployment script also invokes it automatically.

## Related

- [Scripts index](../README.md)
- [Packaging overview](../../packaging/README.md)
- [License](../../LICENSE.md)