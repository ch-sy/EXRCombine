# Maintainer: Christian Seybert

pkgname=exrcombine
pkgver=1.0.0
pkgrel=1
pkgdesc='Combines EXR Images'
arch=('x86_64' 'i686')
url="https://github.com/ch-sy/EXRCombine"
license=('MIT')
depends=('openexr' 'openmpi')
makedepends=('cmake')
source=("https://github.com/ch-sy/EXRCombine")
md5sums=('cb93195a879de40873568731c0f792f4')

build() {
    cd "$srcdir"
    mkdir -p build && cd build
    cmake ..
    make
}

package() {
    cd "$srcdir"/build
    make DESTDIR="$pkgdir/" install
}