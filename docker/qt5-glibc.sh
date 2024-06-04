mkdir -p build/qt5/glibc
cd build/qt5/glibc
cmake ../../..
make -j$(nproc)
