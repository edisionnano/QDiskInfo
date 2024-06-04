mkdir -p build/qt5/musl
cd build/qt5/musl
cmake ../../..
make -j$(nproc)
