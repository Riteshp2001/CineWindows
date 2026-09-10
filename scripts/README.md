# Project Scripts

Repeatable setup, build-support, packaging, and deployment tools.

## Layout

| Directory | Purpose |
| --- | --- |
| [`setup/`](setup/README.md) | Install or prepare platform dependencies. |
| [`generate/`](generate/README.md) | Generate build inputs such as translations and macOS icons. |
| [`package/`](package/README.md) | Stage, verify, and package release artifacts. |

Run commands from the repository root unless a script documents otherwise. Scripts resolve project
paths from their own location and write only to documented dependency, build, or packaging folders.

The temporary repository-metadata generator was removed after its one-time migration completed.
Generated community files and directory indexes are now maintained directly.

## Related

- [Project overview](../README.md)
- [License](../LICENSE.md)
