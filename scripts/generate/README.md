# Generation Scripts

Tools that generate inputs consumed by normal builds and releases.

| Script | Purpose |
| --- | --- |
| `generate_icns.sh` | Generate the macOS application icon bundle from the canonical SVG. |
| `update_translations.ps1` | Refresh and release Qt translation catalogs. |

These tools are retained because they are repeatable whenever icons or translatable strings change.
Run them from the repository root using the commands documented in the project and localization
READMEs.

## Related

- [Scripts index](../README.md)
- [Localization guide](../../i18n/README.md)
- [License](../../LICENSE.md)