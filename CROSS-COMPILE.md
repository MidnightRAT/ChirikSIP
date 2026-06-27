# ChirikSIP Windows Cross-Compilation

## Передумови

Для збірки під Windows через MinGW потрібні:
- MinGW32/64 cross-compiler (вже встановлено)
- PortAudio (зібрано в `~/cross-libs/mingw32` та `~/cross-libs/mingw64`)
- PJSIP (зібрано в `~/cross-libs/mingw32` та `~/cross-libs/mingw64`)
- Qt6 для MinGW (потрібно зібрати або завантажити)

## Проблема з Qt6

Збірка Qt6 з source для MinGW крос-компіляції складна через:
- Конфлікти headers між хостовою та цільовою системами
- Складну систему збірки Qt6
- Залежності від Python, Perl, Ruby для конфігурації

## Рішення: Docker

Використовуємо Docker контейнер для izoljacії середовища збірки:

```bash
# Зібрати Docker образ (містить всі залежності)
./docker-build.sh

# Зібрати exe файли
./build-windows.sh
```

## Альтернатива: ручна збірка Qt6

Якщо потрібна локальна збірка:

1. Завантажте Qt6 Source
2. Встановіть залежності: `python3, perl, ruby, flex, bison`
3. Знайдіть правильні CMake flags для MinGW
4. Збірка займає 2-4 години

## Структура проєкту

```
ChirikSIP/
├── CMakeLists.txt          # Platform-specific build
├── Dockerfile.mingw        # Docker контейнер для збірки
├── build-windows.sh        # Скрипт збірки
├── docker-build.sh         # Збірка Docker образу
├── toolchains/
│   ├── mingw32.cmake       # CMake toolchain 32-bit
│   └── mingw64.cmake       # CMake toolchain 64-bit
├── windows/
│   ├── chiriksip.rc        # Windows resource файл
│   └── chiriksip.ico       # Windows іконка
└── src/                    # Вихідний код
```

## Залежності (зібрані локально)

```
~/cross-libs/mingw32/
├── include/pjsua.h
├── include/portaudio.h
├── lib/libpj*.a
├── lib/libportaudio.a
└── lib/pkgconfig/libpjproject.pc

~/cross-libs/mingw64/
├── include/pjsua.h
├── include/portaudio.h
├── lib/libpj*.a
├── lib/libportaudio.a
└── lib/pkgconfig/libpjproject.pc
```
