<!--
  CineWindows - Video Player
  Copyright (c) 2026 Ritesh Pandit

  CineWindows Community License

  This source code is made available for personal, non-commercial
  use only. Organizations may not use, copy, modify, or distribute
  this code without written permission from Ritesh Pandit.

  See the LICENSE.md file for full license terms.

  Project: CineWindows
  Author:  Ritesh Pandit
-->

# Third-Party Notices

This file records third-party source material retained by CineWindows for reference or adaptation. CineWindows's proprietary license does not replace the licenses listed below for their respective material.

## Material Web

- Project: Material Web (`@material/web`)
- Repository: https://github.com/material-components/material-web
- Version: 2.5.0
- Copyright: Google LLC
- License: Apache License 2.0
- CineWindows status: selected Material 3 web components are bundled into the local companion remote UI.

Material Web is provided under the Apache License 2.0: https://www.apache.org/licenses/LICENSE-2.0

## QR Code Generator Library

- Project: QR Code generator library
- Creator: Project Nayuki
- Repository: https://github.com/nayuki/QR-Code-generator
- Pinned revision: `2c9044de6b049ca25cb3cd1649ed7e27aa055138`
- License: MIT License
- CineWindows status: the C++ implementation generates companion pairing QR codes locally.

## Solar Icons

- Project: Solar icon set
- React package: `@solar-icons/react` 1.1.1 (MIT wrapper; Solar artwork remains CC BY 4.0)
- Creator: 480 Design
- Official source: https://www.figma.com/community/file/1166831539721848736
- Iconify collection: https://icon-sets.iconify.design/solar/
- License: Creative Commons Attribution 4.0 International (CC BY 4.0), https://creativecommons.org/licenses/by/4.0/
- CineWindows status: selected Linear and Bold SVGs are embedded as application action icons; the retained CineWindows audio-off and subtitles-off icons are excluded from this attribution.

Attribution: Solar icons by 480 Design, licensed under CC BY 4.0. The SVGs were renamed and wrapped with 24x24 `currentColor`-compatible markup for CineWindows.

## RinUI

- Project: RinUI, a Fluent Design-like UI library for Qt Quick
- Repository: https://github.com/RinLit-233-shiroko/Rin-UI
- Pinned revision: `1d46b25639b6603a18d163d7fa0474def4aebefc`
- Copyright: Copyright (c) 2025 RinLit
- License: MIT License
- CineWindows status: source-reference submodule only; RinUI is not loaded, imported, linked, or deployed at runtime.

The pinned source is retained in `third_party/RinUI` for design and implementation reference. Future adaptations must copy the necessary code or assets into CineWindows-owned resources, preserve attribution for copied or substantially derived material, remove RinUI runtime/theme/Python/import-path dependencies, and add an immediate production consumer and relevant tests in the same change. The submodule must not become a runtime or packaging dependency.

### MIT License

Copyright (c) 2025 RinLit

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
