# Pixel Office — автономное демо

[← Главная инструкция и распиновка](../README.md)

Отдельная прошивка [`firmware/PixelOfficeDemo/`](../firmware/PixelOfficeDemo/)
для ESP32-S3 + ST7789 320×240. **Это симуляция, не реальные задачи.**
Нет Wi-Fi, BLE, чтения чатов, companion-приложения или сетевых запросов.
После прошивки компьютер не нужен, достаточно USB-питания.

## Как запустить

Зависимости и установка Arduino CLI — в [корневом README](../README.md).
Все команды ниже выполняются из корня клонированного репозитория;
замените `<PORT>` на порт своей ESP32 из `arduino-cli board list`.

```sh
arduino-cli compile --fqbn esp32:esp32:esp32s3:UploadSpeed=460800 firmware/PixelOfficeDemo
arduino-cli upload --fqbn esp32:esp32:esp32s3:UploadSpeed=460800 -p <PORT> firmware/PixelOfficeDemo
```

Пины совпадают с обычным монитором: CS 47, RST 14, DC 21, MOSI 41, SCK 42.
Инициализация 240×320, rotation 1, inversion false.

После старта трое агентов идут к столам, остальные гуляют. Каждые 12 секунд
меняется состав работающих. Для кадра подождите 8–12 секунд после включения.
Условные имена `CODEX 1–3` и `OPENCODE 1–3` не связаны с сессиями программ.
Зелёная точка — работа/чтение, жёлтая — ходьба, серая — другие состояния.
Подписи смещаются при сближении. Внизу постоянно видно `DEMO / SIMULATED`.

## Вернуться к обычному монитору

Сначала настройте свой `firmware/AgentMonitor/config.h` согласно главной инструкции.
Проводку менять не нужно.

```sh
arduino-cli compile --fqbn esp32:esp32:esp32s3:UploadSpeed=460800 firmware/AgentMonitor
arduino-cli upload --fqbn esp32:esp32:esp32s3:UploadSpeed=460800 -p <PORT> firmware/AgentMonitor
```

## Снимок framebuffer по USB

Python 3, только стандартная библиотека. Не запускайте одновременно serial monitor.
Утилита проверена на macOS; использует POSIX `termios`, на Windows напрямую
не работает. Принимает только кадр демо 320×240.

```sh
python3 pixel-office-demo/capture.py --port <PORT> framebuffer.png
```

Считывает framebuffer за ~14 секунд; **на это время анимация останавливается**,
после передачи продолжается. Это изображение из памяти платы, не фотография TFT.
Serial: 115200 baud, запрос ASCII `s`, ответ `PXOF`, размеры и RGB565.

## Проверки

Сборка с подписями: 367494 байт flash, 28288 байт статических данных;
framebuffer 153600 байт выделяется из heap при старте. PSRAM не требуется
на проверенной ESP32-S3. Не проверено на обычном ESP32-WROOM/CYD.

`sh tools/test-cpp.sh` прогоняет 5 симулированных минут под ASan/UBSan:
6 персонажей, работа и ходьба, координаты в пределах карты, без обращений к NVS.
Прошивка с подписями загружена на физическую ESP32-S3.

## Основа и лицензия

[underactive/pixel-agents-esp32](https://github.com/underactive/pixel-agents-esp32),
машина состояний и бесплатные fallback-спрайты. Платный Office Tileset не включён.
Демо распространяется по **PolyForm Noncommercial 1.0.0**, не MIT:
[полный текст](../firmware/PixelOfficeDemo/LICENSE.upstream),
[авторство и изменения](../firmware/PixelOfficeDemo/THIRD_PARTY.md).
