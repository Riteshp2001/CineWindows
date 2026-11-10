# QML Media Library

Media Hub views for browsing, searching, history, favorites, and idle-state discovery.

## Scope

Database and filesystem behavior belongs in C++ services; this directory owns the corresponding views and interaction flow.

History has Recent Files and All Visits views under one sidebar entry.
Manage Folders controls included library roots, excluded subfolders, optional
library-only browsing, and individually visible Places. Exclusions take
precedence over inclusions and apply to both indexing and filesystem browsing.

## Related

- [Parent directory](../README.md)
- [Project overview](../../README.md)
- [License](../../LICENSE.md)
