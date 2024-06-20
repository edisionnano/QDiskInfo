# KDiskInfo
CrystalDiskInfo alternative for Linux

![image](https://github.com/edisionnano/KDiskInfo/assets/26039434/cdfe181d-f99e-4534-9ec4-25efd8a4391b)
The icon is a modified version of the KDE Partition Manager logo which is available under GPL-3.0+

## Docker Builds

You will need docker and docker compose installed.

### Building for musl/glibc/qt5/qt6

```sh
docker compose up
``` 

### Docker Specific Builds

#### Building for Qt5/musl

```sh
docker compose up kdiskinfo-qt5-musl
```

#### Building for Qt6/musl

```sh
docker compose up kdiskinfo-qt6-musl
```

#### Building for Qt5/glibc

```sh
docker compose up kdiskinfo-qt5-glibc
```

#### Building for Qt6/glibc

```sh
docker compose up kdiskinfo-qt6-glibc
```

### Using Shell inside Docker Builds

You can replace kdiskinfo-qt6-musl for the build options above:

```sh
docker compose run kdiskinfo-qt6-musl sh
```

### Generating Translate Files with Docker

The commands below should be executed inside the container shell.
On the examples the locale used is pt_BR, you can change for your desired locale.

#### Qt6/musl

Generate the translations file:

```sh
/usr/lib/qt6/bin/lupdate src/ -ts translations/kdiskinfo_pt_BR.ts -noobsolete
```

After finishing the translations run:

```sh
/usr/lib/qt6/bin/lrelease translations/kdiskinfo_pt_BR.ts -qm translations/kdiskinfo_pt_BR.qm
```

After finishing make sure to also edit ``src/resources.qrc`` after generating the qm to include it.
