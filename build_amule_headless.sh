#!/bin/bash

set -e

echo "📁 Preparando entorno de compilación..."
cd "$(dirname "$0")"

rm -rf build
mkdir build
cd build

echo "⚙️ Ejecutando CMake..."
cmake .. \
  -DBUILD_DAEMON=ON \
  -DBUILD_AMULECMD=ON \
  -DBUILD_WEBSERVER=ON \
  -DBUILD_MONOLITHIC=OFF \
  -DBUILD_REMOTEGUI=OFF \
  -DBUILD_ALC=OFF \
  -DBUILD_ALCC=OFF \
  -DENABLE_NLS=OFF \
  -DENABLE_IO_URING=ON

echo "🛠️ Compilando aMule..."
make -j$(nproc)

echo "✅ Compilación completada. Puedes instalar con:"
echo "   sudo make install"
sudo make install
