
<img style="vertical-align: middle;" src="data\icons\hicolor\scalable\apps\io.github.gyrolet.CineWindows.png" width="300" height="300" align="left">

### Cine

Play your videos on Windows

<br>

<a href="https://github.com/diegopvlk/cine/releases">
  <img width="240" alt="Download for Windows" src="https://img.shields.io/badge/Download-Windows-blue?style=for-the-badge&logo=windows"/>
</a>

### Description

Cine combines a clean interface with a high-performance playback engine to deliver a smooth and modern video-watching experience on Windows.

### Features

- **Clean Windows Experience** — A refined, distraction-free interface designed for everyday desktop use
- **MPV-Based Playback** — Powered by MPV for reliable performance, wide format support, and smooth playback
- **Audio and Subtitles** — Easily manage audio tracks, subtitle tracks, and synchronization
- **Video Controls** — Adjust brightness, contrast, zoom, aspect ratio, playback options, and more
- **Lightweight and Fast** — Built to keep the viewing experience simple, responsive, and focused

### Screenshot

<p align="center">
  <img src="screenshots/video.png" alt="Video Playing"/>
</p>

<div>
  <details>
    <summary>More Screenshots (Expand):</summary><br>
      <p align="center"><img src="screenshots/preferences.png" alt="Preferences"/></p>
      <p align="center"><img src="screenshots/options.png" alt="Video Options"/></p>
      <p align="center"><img src="screenshots/window.png" alt="Main Window"/></p>
  </details>
</div>

### Installation

Download the latest Windows build from the **Releases** page.

1. Go to the latest release
2. Download the Windows installer or portable build
3. Run Cine
4. Open your video and start watching

### Donate

If you want to help with a donation, thank you! You can use:

### ☕ Buy Me a Coffee

<a href="https://www.buymeacoffee.com/riteshp2001">
  <img src="https://img.shields.io/badge/Buy_Me_A_Coffee-FFDD00?style=for-the-badge&logo=buy-me-a-coffee&logoColor=black" alt="Buy Me a Coffee">
</a>

### UPI

`panditritesh2001@okhdfcbank`

<a href="upi://pay?pa=panditritesh2001@okhdfcbank&pn=Ritesh%20Pandit&cu=INR">
  <img src="data\icons\hicolor\scalable\apps\upiqr-code.svg" width="250" height="250" alt="UPI QR Code">
</a>

Scan with any UPI app (Google Pay, PhonePe, Paytm, BHIM)


### Translations

You can help translate Cine using [Weblate](https://hosted.weblate.org/projects/cine/app/)

[![Translation status](https://hosted.weblate.org/widget/cine/app/multi-auto.svg)](https://hosted.weblate.org/engage/cine/)

### Easy Method
1. Run `scripts/setup_windows.ps1` as Administrator. This will install MSYS2, Python, and all dependencies automatically.
2. The script will generate a bundled runtime folder (`runtime`).

### Manual Method
1. Download and install [MSYS2](https://www.msys2.org/).
2. Open "MSYS2 MINGW64" terminal.
3. Install dependencies: `pacman -S mingw-w64-x86_64-python mingw-w64-x86_64-gtk4 mingw-w64-x86_64-libadwaita mingw-w64-x86_64-mpv`
4. Install python packages: `pip install python-mpv yt-dlp`
5. Run `scripts/build_windows.ps1` to compile the resources.

## Running the App
- Double click `run_cine.bat` or run `.\cine.ps1` in PowerShell.

## Building the Installer
- Install NSIS.
- Run `scripts/build_windows.ps1` and it will automatically generate the installer using NSIS.

### Code of Conduct

This project follows a respectful and inclusive code of conduct for all contributors and users.

### Build from source

To build Cine from source on Windows:

1. Clone the repository

```bash
git clone https://github.com/diegopvlk/cine.git
cd cine
````

2. Install the required Windows build dependencies

3. Build and run the project using your preferred development environment or terminal

### License

Please check the repository license before using, modifying, or distributing the project.
