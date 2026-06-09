# Maintainer: gyrolet
# MSYS2 PKGBUILD for Cine (mingw-w64)
# Build with: makepkg-mingw -f

MINGW_PACKAGE_NAME="mingw-w64-${MINGW_CHOST}-cinewindows"
pkgname="${MINGW_PACKAGE_NAME}"
pkgver=1.5.2
pkgrel=1
pkgdesc="A GTK4 video player (Windows port)"
arch=("any")
mingw_arch=("mingw64")
url="https://github.com/Riteshp2001/CineWindows"
license=("GPL3")
depends=(
    "${MINGW_PACKAGE_PREFIX}-gtk4"
    "${MINGW_PACKAGE_PREFIX}-libadwaita"
    "${MINGW_PACKAGE_PREFIX}-python"
    "${MINGW_PACKAGE_PREFIX}-python-gobject"
    "${MINGW_PACKAGE_PREFIX}-python-mpv"
    "${MINGW_PACKAGE_PREFIX}-mpv"
    "${MINGW_PACKAGE_PREFIX}-adwaita-icon-theme"
    "${MINGW_PACKAGE_PREFIX}-yt-dlp"
)
makedepends=(
    "${MINGW_PACKAGE_PREFIX}-glade"
)
options=("!strip" "!staticlibs")
source=("${url}/archive/v${pkgver}.tar.gz")
sha256sums=("SKIP")

build() {
cd "${srcdir}/CineWindows-${pkgver}"

    # Compile Blueprint UI files
    for f in src/*.blp; do
        blueprint-compiler compile "$f" --output "src/$(basename "$f" .blp).ui"
    done

    # Compile GResource
    glib-compile-resources src/cine.gresource.xml \
        --target=src/cine.gresource --sourcedir=src

    # Compile GSettings schema
    glib-compile-schemas data/

    # Build launcher
gcc -O2 -mwindows src/launcher.c -o CineWindows.exe
}

package() {
cd "${srcdir}/CineWindows-${pkgver}"

install -d "${pkgdir}${MINGW_PREFIX}/share/CineWindows"
cp -r src "${pkgdir}${MINGW_PREFIX}/share/CineWindows/"
cp -r data "${pkgdir}${MINGW_PREFIX}/share/CineWindows/"
cp CineWindows.exe "${pkgdir}${MINGW_PREFIX}/share/CineWindows/"
cp start_cine.py "${pkgdir}${MINGW_PREFIX}/share/CineWindows/"
cp requirements.txt "${pkgdir}${MINGW_PREFIX}/share/CineWindows/"

    # Desktop entry
    install -d "${pkgdir}${MINGW_PREFIX}/share/applications"
sed "s|@bindir@|${MINGW_PREFIX}/share/CineWindows|" \
    data/io.github.gyrolet.CineWindows.desktop.in \
    > "${pkgdir}${MINGW_PREFIX}/share/applications/io.github.gyrolet.CineWindows.desktop"

    # Icons
    install -d "${pkgdir}${MINGW_PREFIX}/share/icons"
    cp -r data/icons/* "${pkgdir}${MINGW_PREFIX}/share/icons/"

    # GSettings schema
    install -d "${pkgdir}${MINGW_PREFIX}/share/glib-2.0/schemas"
    cp data/gschemas.compiled "${pkgdir}${MINGW_PREFIX}/share/glib-2.0/schemas/"
}
