// Copyright (c) 2026 Ritesh Pandit
// Last modified: 2026-09-10
// Modified by: Ritesh Pandit

const fs = require('fs');
const path = require('path');

const src = fs.existsSync('docs/dist') 
  ? 'docs/dist' 
  : fs.existsSync('../docs/dist') 
    ? '../docs/dist' 
    : null;

if (!src) {
  console.warn('docs/dist not found – skipping docs integration (docs not built)');
  process.exit(0);
}

// 1. Merge the basePath-prefixed documentation pages into the site output.
const docsSrc = path.join(src, 'docs');
if (!fs.existsSync(docsSrc)) {
  console.error('docs/dist/docs not found - documentation build is incomplete');
  process.exit(1);
}
fs.cpSync(docsSrc, 'dist/docs', { recursive: true });

// 2. Copy _astro assets to root dist/_astro so assets load relative to root
if (fs.existsSync(path.join(src, '_astro'))) {
  fs.cpSync(path.join(src, '_astro'), 'dist/_astro', { recursive: true });
}

// 3. Publish root assets and agent-facing files at the URLs Blume advertises.
for (const f of [
  'agent-readability.json',
  'blume-search.json',
  'docs.md',
  'docs.mdx',
  'index.md',
  'index.mdx',
  'llms-full.txt',
  'llms.txt',
  'logo-light.svg',
  'logo-dark.svg',
  'openapi.json',
]) {
  const fileSrc = path.join(src, f);
  if (fs.existsSync(fileSrc)) {
    fs.copyFileSync(fileSrc, path.join('dist', f));
  }
}

for (const directory of ['.well-known', 'api']) {
  const directorySrc = path.join(src, directory);
  if (fs.existsSync(directorySrc)) {
    fs.cpSync(directorySrc, path.join('dist', directory), { recursive: true });
  }
}

// 4. Ensure favicon.svg is copied to dist/ and dist/docs/
const publicFavicon = fs.existsSync('public/favicon.svg') ? 'public/favicon.svg' : '../website/public/favicon.svg';
if (fs.existsSync(publicFavicon)) {
  fs.copyFileSync(publicFavicon, 'dist/favicon.svg');
  fs.copyFileSync(publicFavicon, 'dist/docs/favicon.svg');
}

// 5. Replace default Blume base64 icon with CineWindows favicon.svg in all HTML files under dist/docs
function fixHtmlFiles(dir) {
  const entries = fs.readdirSync(dir, { withFileTypes: true });
  for (const entry of entries) {
    const fullPath = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      fixHtmlFiles(fullPath);
    } else if (entry.isFile() && entry.name.endsWith('.html')) {
      let content = fs.readFileSync(fullPath, 'utf8');
      // Replace base64 icon data URI with /favicon.svg
      content = content.replace(
        /<link\s+href="data:image\/png;base64,[^"]*"\s+rel="icon"\s+type="image\/png">/g,
        '<link rel="icon" type="image/svg+xml" href="/favicon.svg">'
      );
      content = content.replace(
        /<link\s+rel="icon"\s+type="image\/png"\s+href="data:image\/png;base64,[^"]*">/g,
        '<link rel="icon" type="image/svg+xml" href="/favicon.svg">'
      );
      // Remove duplicate H1 heading from main article body
      content = content.replace(/<h1 id="cinewindows-documentation">CineWindows Documentation<\/h1>/g, '');
      // Rewrite internal docs links so they stay inside /docs subpath
      content = content.replace(/href="\/guides\//g, 'href="/docs/guides/');
      content = content.replace(/href="\/reference\//g, 'href="/docs/reference/');
      // Clean <title> tags to eliminate non-ASCII dash encoding glitches (??? in browser tabs)
      content = content.replace(/<title>(.*?)<\/title>/gi, (m, t) => {
        const clean = t.replace(/[—–]/g, '-').replace(/\s+-\s+/g, ' - ');
        return `<title>${clean}</title>`;
      });
      fs.writeFileSync(fullPath, content, 'utf8');
    }
  }
}

fixHtmlFiles('dist/docs');
console.log('Docs, _astro assets & CineWindows favicon integrated cleanly into dist/');
