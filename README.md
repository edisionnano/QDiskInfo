# QDiskInfo
QDiskInfo is a frontend for smartctl (part of the smartmontools package). It provides a user experience similar to CrystalDiskInfo. It shows the SMART (Self-Monitoring, Analysis, and Reporting Technology) data of modern hard disk drives.
![mockup](https://github.com/edisionnano/QDiskInfo/assets/26039434/e5488f41-6ea2-4304-9ae8-13d5dac7715b)
The icon is a modified version of the KDE Partition Manager logo which is available under GPL-3.0+.

## Packages
[![Packaging status](https://repology.org/badge/vertical-allrepos/qdiskinfo.svg)](https://repology.org/project/qdiskinfo/versions)

## Compiling and Installing
### Compilation and Runtime Dependencies
Ubuntu and based distros (Mint, Pop!\_OS, etc.)
```sh
sudo apt install build-essential cmake git libgl1-mesa-dev libxkbcommon-dev qt6-base-dev qt6-tools-dev qt6-wayland smartmontools
```
Fedora and derivatives (Nobara Project, etc.)
```
sudo dnf install cmake git mesa-libGL-devel libxkbcommon-devel qt6-qtbase-devel qt6-qttools-devel qt6-qtwayland-devel smartmontools
```
### Compilation Steps
First you will need to clone the repository and cd inside
```sh
git clone https://github.com/edisionnano/QDiskInfo.git && cd QDiskInfo
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

## Localisation
Currently, QDiskInfo has support for the following languages:
- 🌐 English (C)
- 🇬🇷 Greek (`el_GR`)
- 🇪🇸 Spanish (`es_ES`)
- 🇧🇷 Brazilian Portuguese (`pt_BR`)
- 🇨🇳 Simplified Chinese (`zh_CN`)

You can translate QDiskInfo to your language very easily using [Crowdin](https://crowdin.com/project/qdiskinfo). If your language isn't available on Crowdin, feel free to create a new issue.
<br><br>Alternatively you can copy the `qdiskinfo_en_US.ts`, which can be found inside the translations folder, to the locale you want. For example to `qdiskinfo_de_DE.ts` for German. Then you can use the QT Linguist application to translate the strings, marking every finished one with a tick. Once you are finished you can compile the .ts file to a .qm file by running this command from the root of the project:
```sh
lrelease translations/qdiskinfo_de_DE.ts -qm translations/qdiskinfo_de_DE.qm
```
Once you do that, add the .qm file on `src/resources.qrc` and compile the project.
<br>If your system language differs from the one you are translating to, you can use
```sh
LC_ALL=de_DE.UTF-8 ./QDiskInfo
```
to force the app to use the language of your choice.

## Using CrystalDiskInfo Anime Themes
The process is similar to the one above with a few changes:<br>
First you must download the edition of CrystalDiskInfo you want (Aoi for example) in ZIP format from [here](https://crystalmark.info/en/download/), from this archive you shall copy the light and dark backgrounds as well as the good, caution, bad, unknown icons to dist/theme with the same name as the templates there.<br>
Once you do that compile like above but when running CMake do this instead:
```sh
cmake .. -DCMAKE_BUILD_TYPE:STRING=MinSizeRel -DQT_VERSION_MAJOR=6 -DINCLUDE_OPTIONAL_RESOURCES=ON -DCHARACTER_IS_RIGHT=ON
```
Regarding the `-DCHARACTER_IS_RIGHT` set it to ON for themes where the character is right (like Aoi) or OFF for most other themes.
