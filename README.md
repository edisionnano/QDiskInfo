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
