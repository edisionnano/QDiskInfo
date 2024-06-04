# KDiskInfo
CrystalDiskInfo alternative for Linux

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
