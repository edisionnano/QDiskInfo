# QDiskInfo
CrystalDiskInfo alternative for Linux
![mockup](https://github.com/edisionnano/QDiskInfo/assets/26039434/e5488f41-6ea2-4304-9ae8-13d5dac7715b)
The icon is a modified version of the KDE Partition Manager logo which is available under GPL-3.0+

## Compiling and Installing
First you will need to clone the repository and cd inside
```sh
https://github.com/edisionnano/QDiskInfo.git && cd QDiskInfo
```
Then you must create a build directory and cd inside
```sh
mkdir build && cd build
```
After that use CMake to setup the project, Qt version can also be set to 5 (for example for Ubuntu)
```sh
cmake .. -DCMAKE_BUILD_TYPE:STRING=MinSizeRel -DQT_VERSION_MAJOR=6
```
And finally compile the project with all the threads
```sh
make -j$(nproc)
```
This will create a `QDiskInfo` binary on the build directory but you can also install it using
```sh
sudo make install
```
