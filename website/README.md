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

# CineWindows website

Fast marketing, privacy, and support site built with Vite, React, TypeScript, Tailwind CSS v4, Radix primitives, and local shadcn/ui-style components.

## Run locally

```powershell
cd website
npm install
npm run dev
```

## Production configuration

Create `website/.env.production` (it is intentionally ignored by Git) with:

```dotenv
VITE_SITE_URL=https://your-real-domain.example
```

`VITE_SITE_URL` is used for canonical and social metadata.

## Build

```powershell
cd website
npm run lint
npm run build
npm run preview
```

The output is written to `website/dist`.

## Deploy to Vercel

Use `website` as the project root, `npm run build` as the build command, and `dist` as the output directory. Add both production environment variables in the Vercel project settings. The included `vercel.json` preserves the `/privacy` and `/support` routes.

After deployment, verify:

- `/`, `/privacy`, and `/support` load directly and after refresh.
- The privacy URL is the same one entered wherever applicable.
- Canonical and Open Graph URLs use the production domain.
- The support email is correct.
