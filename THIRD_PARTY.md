# Что использовалось

`bridge.mjs`, `firmware/AgentMonitor/` (кроме сторонних глифов), тесты,
утилиты и документация — код этой сборки, MIT.

## Шрифт

`firmware/AgentMonitor/font_ru.h` сгенерирован из DejaVu Sans Mono:
Bitstream Vera / DejaVu / Arev. Исходный TTF и полный текст лицензии:
[`tools/fonts/`](tools/fonts/). Файлы получены из
[набора шрифтов Matplotlib](https://github.com/matplotlib/matplotlib/tree/main/lib/matplotlib/mpl-data/fonts/ttf).

## Зависимости (устанавливаются отдельно)

- Arduino core for ESP32 — Espressif.
- Adafruit GFX, Adafruit ST7735 and ST7789, Adafruit BusIO — Adafruit.
- ArduinoJson 7 — Benoît Blanchon.
- Node.js 20+ и встроенные модули — мост без npm-зависимостей.
- Pillow — только для повторной генерации шрифта.

Лицензии этих библиотек остаются у их авторов; исходники библиотек здесь
не перепубликуются. Это любительская сборка, не официальный продукт
OpenAI, OpenCode, Espressif или Adafruit.
