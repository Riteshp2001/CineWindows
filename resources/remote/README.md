# CineWindows Companion Remote

The companion interface is a Vite-powered React app embedded into the CineWindows Qt resources.

## Development

```powershell
npm install
npm run dev
```

The Vite development server uses mock playback and folder data when it is not opened through CineWindows, so the responsive interface can be reviewed independently.

## Build and verification

```powershell
npm run lint
npm run build
```

`npm run build` writes the production bundle to `dist/`. The stable filenames in that folder are referenced by `resources/cinewindows.qrc`, so rebuild the Qt application after changing the web interface.
