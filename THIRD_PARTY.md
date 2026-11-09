# Что использовалось

## Пиксельный офис

Основа демо: [underactive/pixel-agents-esp32](https://github.com/underactive/pixel-agents-esp32),
v0.14.2, commit `6ca74e56b1f236d434ed2eda9bca374dc4c6359e`.
Copyright (c) 2026 TARS Industrial Technical Solutions, tarsindustrial.tech.

Взяты машина состояний, поиск пути, генератор бесплатных персонажей, мебели и
облачков. Локально добавлены рендерер для Adafruit ST7789, наша распиновка,
автономная хореография и подписи CODEX/OPENCODE. Платный Office Interior
Tileset, картинки собаки и мини-роботов не включены.

**Весь `firmware/PixelOfficeDemo/` — производная работа под PolyForm
Noncommercial 1.0.0, не MIT.** Это разрешает некоммерческое использование
с соблюдением условий. Коммерческое использование проверяйте отдельно с
правообладателем. Полный текст и подробности:
[LICENSE.upstream](firmware/PixelOfficeDemo/LICENSE.upstream),
[THIRD_PARTY.md](firmware/PixelOfficeDemo/THIRD_PARTY.md).
Тесты в `pixel-office-demo/test/` также распространяются на этих условиях.

Исходный проект ссылается на
[pablodelucca/pixel-agents](https://github.com/pablodelucca/pixel-agents).

## Обычный монитор

`bridge.mjs`, `firmware/AgentMonitor/` (кроме сторонних глифов), тесты,
утилиты и документация — код этой сборки, MIT.
Для сравнения поведения источников событий изучались
[agent-office/runtime-observers](https://github.com/NosytLabs/agent-office/blob/main/docs/runtime-observers.md)
и companion Pixel Agents; код companion в этот репозиторий не включён.

## Шрифт

`firmware/AgentMonitor/font_ru.h` сгенерирован из DejaVu Sans Mono:
Bitstream Vera / DejaVu / Arev. Исходный TTF и полный текст лицензии:
[`tools/fonts/`](tools/fonts/). Файлы получены из
[набора шрифтов Matplotlib](https://github.com/matplotlib/matplotlib/tree/main/lib/matplotlib/mpl-data/fonts/ttf).
Проприетарный системный шрифт macOS для публичной версии не используется.

## Зависимости (устанавливаются отдельно)

- Arduino core for ESP32 — Espressif.
- Adafruit GFX, Adafruit ST7735 and ST7789, Adafruit BusIO — Adafruit.
- ArduinoJson 7 — Benoît Blanchon (обычный монитор).
- Node.js 20+ и встроенные модули — мост без npm-зависимостей.
- Pillow — только для повторной генерации шрифта.

Лицензии этих библиотек остаются у их авторов; исходники библиотек здесь
не перепубликуются. Это любительская сборка, не официальный продукт
OpenAI, OpenCode, Espressif или Adafruit.
