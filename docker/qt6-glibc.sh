mkdir -p build/qt6/glibc
cd build/qt6/glibc
cmake ../../..
make -j$(nproc)
