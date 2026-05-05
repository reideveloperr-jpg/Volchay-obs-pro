# Lumen Stream

> Современный десктоп-клиент стриминга на Twitch на C++/Qt6.
> Дизайн в духе Luna, без копирования — с янтарно-оранжевым акцентом и в стиле Acri.

![themes preview](docs/themes.png)

## Что внутри

- **Qt6 / C++17** интерфейс с тремя темами: **Светлая**, **Blackout** (true-black для OLED), **RGB** (анимация акцента по кругу HSV).
- **Янтарно-оранжевый** акцент по умолчанию (`#FF9900`), цвет можно поменять прямо в настройках.
- Стриминг через локальный **`ffmpeg`** (subprocess) — RTMP H.264 + AAC прямо в Twitch.
- 4 встроенных пресета по [официальной документации Twitch](https://help.twitch.tv/s/article/broadcasting-guidelines):

  | Пресет | Резолюция | FPS | Видео битрейт | Аудио |
  |---|---|---|---|---|
  | Твич 720p30 | 1280×720 | 30 | 3000 kbps | 160 kbps |
  | Твич 720p60 | 1280×720 | 60 | 4500 kbps | 160 kbps |
  | Твич 1080p30 | 1920×1080 | 30 | 4500 kbps | 160 kbps |
  | **Твич 1080p60** *(по умолчанию)* | 1920×1080 | 60 | **6000 kbps** | 160 kbps |

  Все — **CBR**, **keyframe interval 2 сек**, **profile high**, AAC 48 kHz stereo, x264 `veryfast`.

- Ручные настройки кодировщика: можно переключать на NVENC / QuickSync / AMF, менять rate control, x264-пресет, keyframe interval.
- Захват **рабочего стола** (gdigrab/x11grab/avfoundation) или **тестовой картинки** (lavfi `testsrc2`) для проверки RTMP без живого источника.
- Stream key хранится в `QSettings` (если включён чекбокс «запомнить ключ»).

## Зависимости

| | Linux | Windows | macOS |
|---|---|---|---|
| Qt | `qt6-base-dev qt6-tools-dev` | Qt 6.5+ через [aqtinstall](https://github.com/jurplel/install-qt-action) или официальный установщик | `brew install qt@6` |
| CMake | `cmake` (≥ 3.16) | CMake 3.16+ | `brew install cmake` |
| Компилятор | GCC 11+ / Clang 13+ | MSVC 2022 / MinGW-w64 | Apple Clang |
| **ffmpeg** *(runtime)* | `apt install ffmpeg` | [gyan.dev/ffmpeg](https://www.gyan.dev/ffmpeg/builds/), добавь в PATH | `brew install ffmpeg` |

> ffmpeg не линкуется в бинарник — он вызывается как subprocess. Без `ffmpeg` в PATH стрим не запустится.

## Сборка из исходников

### Linux

```bash
sudo apt install qt6-base-dev qt6-tools-dev cmake g++ ffmpeg
git clone https://github.com/reideveloperr-jpg/lumen-stream.git
cd lumen-stream
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/lumen-stream
```

### Windows (MSVC + Qt 6.5)

```powershell
# Установка Qt: https://www.qt.io/download-open-source
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_PREFIX_PATH="C:/Qt/6.5.3/msvc2019_64"
cmake --build build --config Release --parallel

# Упаковка зависимостей рядом с .exe
& "C:/Qt/6.5.3/msvc2019_64/bin/windeployqt.exe" `
      build/Release/LumenStream.exe
```

Готовый `.exe` появится в `build/Release/`.

### Готовый Windows .exe

Каждый push в основную ветку запускает [GitHub Actions workflow](.github/workflows/build.yml), который собирает релизный `.exe`, упаковывает Qt-DLL через `windeployqt` и публикует архив как **artifact** в раздел **Actions** → последний run → **Artifacts** → `LumenStream-windows-x64`.

## Как пользоваться

1. Получи **Stream Key** на [dashboard.twitch.tv → Settings → Stream](https://dashboard.twitch.tv/settings/stream).
2. Запусти `lumen-stream` (или `LumenStream.exe`).
3. Кнопка **Настройки** → вкладка **Стрим** → выбери пресет (по умолчанию Твич 1080p60), вставь Stream Key, нажми **OK**.
4. На вкладке **Стрим** нажми **Начать стрим** — ffmpeg уйдёт в RTMP, статус-пилл сменится на красный **LIVE**.
5. **Остановить** — отправит `q\n` в stdin ffmpeg для корректного завершения FLV.

### Приватность

- Stream key хранится в `QSettings` (`~/.config/LumenStream/Lumen Stream.conf` на Linux, реестр HKCU на Windows).
- Чекбокс «Запомнить ключ» можно снять — тогда ключ удаляется из конфига при сохранении.
- Stream key **никогда не пишется в лог** (в логе видна только команда `ffmpeg ...`, а ключ подставляется только в финальный RTMP URL — он будет в логе, так что не делай скриншот лога публично).

## Архитектура

```
src/
├── main.cpp              — точка входа, QApplication
├── MainWindow.{h,cpp}    — главное окно: сайдбар, превью-карточка, лог
├── SettingsDialog.{h,cpp}— модальные настройки (Стрим/Кодировщик/Внешний вид)
├── PresetManager.{h,cpp} — 4 встроенных Twitch-пресета
├── StreamEngine.{h,cpp}  — обёртка над QProcess для ffmpeg
├── ThemeManager.{h,cpp}  — подмена placeholder'ов в QSS, RGB-таймер
├── PreviewWidget.{h,cpp} — кастомная отрисовка плейсхолдера превью
└── AccentBadge.{h,cpp}   — кружок-индикатор акцента в сайдбаре

resources/
├── themes/
│   ├── light.qss         — Светлая
│   ├── blackout.qss      — Blackout (OLED)
│   └── rgb.qss           — RGB (с анимированным акцентом)
└── icons/                — SVG иконки
```

## Лицензия

MIT — см. [LICENSE](LICENSE).

## Twitch — пропустит ли поток?

Да. Twitch принимает любой валидный RTMP H.264 + AAC поток на `rtmp://live.twitch.tv/app/<key>`. Никакого whitelist-а программ нет — наш ffmpeg-поток выглядит идентично OBS, Streamlabs или Twitch Studio. Что Twitch проверяет: валидный stream key, битрейт ≤ 8500 kbps, keyframe ≤ 6 сек, кодеки H.264/AAC — все эти условия пресеты соблюдают.
