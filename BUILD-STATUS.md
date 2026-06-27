# ChirikSIP Windows Cross-Compilation - Стан

## Що зроблено

### Зібрано локально:
- **MinGW32/64** — встановлено (`mingw32-gcc-c++`, `mingw64-gcc-c++`)
- **PortAudio** — зібрано для mingw32 та mingw64 в `~/cross-libs/`
- **PJSIP 2.13.1** — зібрано для mingw32 та mingw64 в `~/cross-libs/`

### Що створено в гілці mingw:
- `CMakeLists.txt` — platform-specific збірка (if(WIN32))
- `toolchains/mingw32.cmake`, `mingw64.cmake`
- `windows/chiriksip.rc`, `chiriksip.ico`
- `Dockerfile.mingw` — контейнер для збірки
- `build-windows.sh`, `docker-build.sh`
- `CROSS-COMPILE.md` — документація

## Проблема: Qt6

Збірка Qt6 з source для MinGW крос-компіляції неможлива через:
1. **Несумісність версій** — хостовий Qt6 (6.11) несумісний з source Qt6 (6.7)
2. **Відсутність бінарників** — Fedora не має mingw Qt6 пакетів
3. **Складна система збірки** — Qt6 потребує двох етапів (host + target)

## Рішення

### Варіант 1: Docker (найпростіший)
```bash
./docker-build.sh     # зібрати контейнер з Qt6
./build-windows.sh    # зібрати exe
```

### Варіант 2: Використати MSYS2
1. Встановити MSYS2 на Windows
2. Встановити `mingw-w64-x86_64-qt6-base`
3. Зібрати ChirikSIP на Windows напряму

### Варіант 3: Онлайн-збірка
Використати GitHub Actions з MSYS2 середовищем для автоматичної збірки.

## Наступні кроки

1. **Docker**: Запустити `./docker-build.sh` для збірки контейнера
2. **Або MSYS2**: Встановити MSYS2 на Windows і зібрати там
3. **Або GitHub Actions**: Створити workflow для автоматичної збірки
