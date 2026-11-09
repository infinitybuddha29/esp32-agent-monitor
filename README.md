# ESP32 Agent Monitor

Отдельный TFT-экран для Codex и OpenCode на ESP32-S3 + ST7789.
Два независимых скетча: монитор реальных статусов и пиксельный офис для демонстрации.

| Режим | Что делает | Нужен компьютер? |
|---|---|---|
| **[AgentMonitor](firmware/AgentMonitor/)** | Показывает статусы сессий, названия чатов и время работы | Да, Mac с запущенным Node.js-мостом |
| **[PixelOfficeDemo](firmware/PixelOfficeDemo/)** | Шесть персонажей ходят по офису и работают за столами | Только для прошивки, дальше достаточно USB-питания |

**Важно:** офис — симуляция, он пока не подключён к реальным агентам.
Подписи CODEX/OPENCODE в нём условные. У демо есть **некоммерческое лицензионное
ограничение** сторонней основы; обычный монитор — MIT. [Подробнее об авторах и лицензиях](THIRD_PARTY.md).

Автор сборки: [Korolev Dev](https://t.me/korolevdeveloper).

![Пиксельный офис на ESP32: шесть персонажей с подписями CODEX и OPENCODE](docs/images/pixel-office.png)

*Реальный framebuffer платы 320×240; это демо, не статусы живых сессий.*

## Что понадобится

- ESP32-S3. На этой сборке проверена плата с 16 МБ Flash и 8 МБ встроенной PSRAM;
  скетчи собираются с настройкой PSRAM Disabled.
- SPI TFT с контроллером **ST7789, 240×320**, используется горизонтально 320×240.
- Провода и USB-кабель с передачей данных.
- [Arduino CLI](https://arduino.github.io/arduino-cli/) или Arduino IDE.
- Для обычного монитора: macOS, Node.js 20+, Codex CLI и/или OpenCode v2.

Это не готовая прошивка для любого CYD/LILYGO: у других плат могут отличаться
контроллер, пины, питание подсветки и параметры памяти.

## Подключение дисплея

| TFT ST7789 | ESP32-S3 |
|---|---|
| SCL / SCK / CLK | GPIO 42 |
| SDA / MOSI / DIN | GPIO 41 |
| CS | GPIO 47 |
| DC / A0 | GPIO 21 |
| RST / RES | GPIO 14 |
| GND | GND |
| VCC и BL / LED | По документации **вашего модуля**, см. [питание](docs/HARDWARE.md) |

Панель инициализируется как 240×320, `setRotation(1)`, `invertDisplay(false)`.
На другой панели цвета/ориентацию может понадобиться поменять.

## Быстрый старт: пиксельный офис

Самый простой способ повторить картинку: **Wi-Fi и Node.js не нужны**.

```sh
git clone https://github.com/infinitybuddha29/esp32-agent-monitor.git
cd esp32-agent-monitor
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.12
arduino-cli lib install "Adafruit GFX Library@1.13.0" "Adafruit ST7735 and ST7789 Library@1.12.0"
arduino-cli board list
```

Подставьте порт **своей ESP32** вместо `<PORT>`:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32s3:UploadSpeed=460800 firmware/PixelOfficeDemo
arduino-cli upload --fqbn esp32:esp32:esp32s3:UploadSpeed=460800 -p <PORT> firmware/PixelOfficeDemo
```

На macOS порт обычно `/dev/cu.usbserial-…` или `/dev/cu.usbmodem…`,
на Linux — `/dev/ttyUSB…` или `/dev/ttyACM…`, на Windows — `COM…`.
В Arduino IDE выберите **ESP32S3 Dev Module**, загрузите
`firmware/PixelOfficeDemo/PixelOfficeDemo.ino`; скорость загрузки — 460800.

После старта подождите 8–12 секунд: трое персонажей идут работать, остальные
гуляют. Каждые 12 секунд меняется состав работающих. Над ними подписи
`CODEX 1–3` и `OPENCODE 1–3`, внизу — `DEMO / SIMULATED`.
Подробности и UART-снимок экрана: [инструкция демо](pixel-office-demo/README.md).

## Обычный монитор реальных агентов

### 1. Запустить мост на Mac

В каталоге репозитория:

```sh
npm test
npm start
```

`npm install` не нужен: мост использует только встроенные модули Node.js.
Mac и ESP32 должны быть в одной **доверенной локальной сети**.
Мост привязывается к частному IPv4 интерфейса `en0` и отслеживает смену DHCP-адреса.
Пока Mac спит или мост остановлен, экран не получает обновления.

### 2. Настроить ESP32

```sh
cp firmware/AgentMonitor/config.example.h firmware/AgentMonitor/config.h
scutil --get LocalHostName
arduino-cli lib install "ArduinoJson@7.4.3"
```

В **локальном** `firmware/AgentMonitor/config.h` задайте:

```cpp
#pragma once
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define BRIDGE_HOST "YOUR_MAC_HOSTNAME"  // результат scutil, без .local
#define BRIDGE_PORT 8787
```

Этот файл игнорируется Git. Не публикуйте его или бинарную прошивку с паролем.

### 3. Загрузить монитор

```sh
arduino-cli compile --fqbn esp32:esp32:esp32s3:UploadSpeed=460800 firmware/AgentMonitor
arduino-cli upload --fqbn esp32:esp32:esp32s3:UploadSpeed=460800 -p <PORT> firmware/AgentMonitor
```

Этой же командой можно вернуться из демо в монитор. Менять проводку не нужно.
После прошивки USB нужен только для питания; данные приходят по Wi-Fi.

### Что отображается

Семь строк на странице: название чата, провайдер, статус и длительность работы.
Приоритет: нужен ответ → ошибка → работа → пауза → готово.
Длинные названия прокручиваются. Внизу — часы, связь, батарея Mac,
проверка интернета и состояние **только VPN Happ**.

- Codex: локальные `~/.codex/sessions/**/rollout-*.jsonl` и `session_index.jsonl`.
- OpenCode v2: **уже запущенный** shared service, команды `opencode service status`
  и `opencode api`; экспериментальные API могут меняться.
- DONE/ERROR требуют подтверждённого события/результата. Тишина или пропавшая
  сессия не считаются успешным завершением. UNKNOWN на экран не выводится.
- Обнаружение — polling, не точный журнал всех переходов. Редкие или длительно
  не обновляющиеся сессии могут не отображаться. История моста хранится в памяти.
- Мост не вызывает модели и сам не расходует их токены.

Настройки сети и источников: [docs/BRIDGE.md](docs/BRIDGE.md).

## Безопасность

Мост отдаёт метаданные по **HTTP без авторизации**. Любое устройство с доступом
к этому порту в вашей сети может прочитать названия чатов, проектов и ID сессий.
Не открывайте порт в интернет и не используйте в публичном Wi-Fi.
Промпты/ответы не возвращаются API, но названия чатов сами могут быть личными.
Проверка интернета делает HTTPS-запрос к Apple; метаданные агентов туда не уходят.
Подробнее: [SECURITY.md](SECURITY.md).

## Проверки и состав

```sh
npm test
sh tools/test-cpp.sh
```

Проверены тесты моста, прокрутки заголовков и 5 симулированных минут офиса под
AddressSanitizer/UBSan. Обе прошивки собирались Arduino CLI; демо с подписями
загружено на физическую ESP32-S3. Другие платы/ОС не проверены.
Исходники бинарных прошивок не заменяют проверку вашей распиновки.

**Библиотеки:** Arduino ESP32, Adafruit GFX + ST7789 + BusIO, ArduinoJson 7.
Шрифт кириллицы — DejaVu Sans Mono; сгенерированный bitmap уже включён.
Pillow нужен только для `tools/make-font.py`.

**Основа офиса:** [underactive/pixel-agents-esp32](https://github.com/underactive/pixel-agents-esp32),
который основан на [pablodelucca/pixel-agents](https://github.com/pablodelucca/pixel-agents).
Используются бесплатные fallback-спрайты, платный Office Tileset не включён.
Полные условия и границы MIT/Noncommercial: [THIRD_PARTY.md](THIRD_PARTY.md).
