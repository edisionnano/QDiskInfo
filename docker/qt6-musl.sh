mkdir -p build/qt6/musl
cd build/qt6/musl
cmake ../../..
make -j$(nproc)
