# Contributing to CineWindows

Thank you for improving CineWindows. Contributions should be focused, reviewable, and consistent
with the existing C++20, Qt 6, QML, CMake, and packaging conventions.

## Before you start

1. Read the [CineWindows Community License](LICENSE.md). It permits personal, non-commercial use
   and governs submitted contributions.
2. Search [existing issues](https://github.com/Riteshp2001/CineWindows/issues) and discussions before opening duplicates.
3. Open an issue for behavior changes, architecture changes, or work with a broad user impact.
4. Never include private media, credentials, API keys, proprietary samples, or unlicensed assets.

## Development workflow

1. Fork the repository and create a focused branch from `main`.
2. Initialize required submodules with `git submodule update --init --recursive`.
3. Follow the setup and platform commands in the [project README](README.md).
4. Keep behavior in its owning layer: C++ for application/domain logic and QML for presentation.
5. Add or update documentation, translations, attribution, and packaging metadata when applicable.
6. Submit a pull request using the repository template.

## Authorship and modification history

First-party source headers record the latest repository-wide metadata update and modifier.
Exact function-level history remains authoritative in Git; use `git blame` or `git log -L`
when you need to identify who changed a function and when. The default code owner is
[Ritesh Pandit](https://github.com/Riteshp2001).

## Validation

Run the checks relevant to the changed area:

```text
cmake --preset release
cmake --build --preset release
cmake --build --preset release --target CineWindows_qmllint
cmake --build --preset release --target release_translations
npm ci --prefix website
npm run --prefix website lint
npm run --prefix website build
npm ci --prefix resources/remote
npm run --prefix resources/remote lint
npm run --prefix resources/remote build
```

Explain any check you could not run. Keep generated files and dependency lockfiles synchronized
with their source inputs.

## Pull request expectations

- Describe the user-visible behavior and the root cause or design rationale.
- Keep unrelated cleanup out of the pull request.
- Include screenshots for visible UI changes and reproduction steps for bug fixes.
- Preserve the CineWindows license header and all applicable third-party notices.
- Do not add a co-author, contributor, or organization without that party's consent.

## Contribution license

By submitting a contribution, you agree to the grant in section 5 of the
[CineWindows Community License](LICENSE.md). You retain any rights you already hold in your work.
