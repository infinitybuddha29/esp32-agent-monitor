#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "src/demo_renderer.h"

// Identical wiring and panel setup to AgentMonitor. No network configuration.
constexpr int TFT_CS = 47, TFT_RST = 14, TFT_DC = 21;
constexpr int TFT_MOSI = 41, TFT_SCK = 42;
constexpr uint32_t DEMO_STEP_MS = 12000;
Adafruit_ST7789 tft(&SPI, TFT_CS, TFT_DC, TFT_RST);
OfficeState office;
DemoRenderer renderer;
uint32_t lastFrame = 0, lastReport = 0;
int previousStep = -1;
unsigned frames = 0;

void setup() {
    Serial.begin(115200);
    SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
    tft.init(240, 320);
    tft.invertDisplay(false);
    tft.setRotation(1);
    tft.fillScreen(0);
    if (!renderer.begin(tft)) {
        tft.setTextColor(ST77XX_RED);
        tft.setCursor(12, 12);
        tft.print("DEMO: framebuffer allocation failed");
        Serial.println("DEMO ERROR: framebuffer allocation failed");
        while (true) delay(1000);
    }
    randomSeed(42);  // Repeatable choreography for filming.
    office.init();
    office.spawnAllCharacters();
    lastFrame = millis();
    Serial.println("PIXEL OFFICE DEMO ready; simulated agents; Wi-Fi/BLE disabled");
}

void loop() {
    uint32_t now = millis();
    if (now - lastFrame < FRAME_MS) {
        delay(1);
        return;
    }
    float dt = min((now - lastFrame) / 1000.0f, 0.1f);
    lastFrame = now;
    int step = (now / DEMO_STEP_MS) % 4;
    if (step != previousStep) {
        for (uint8_t id = 0; id < 4; ++id) {
            bool working = ((id + step) % 4) != 3;
            office.setAgentState(id, working ? CharState::TYPE : CharState::IDLE,
                                 working ? (id % 2 ? "Read" : "Edit") : "");
        }
        previousStep = step;
    }
    office.update(dt);
    renderer.render(office);
    ++frames;
    if (Serial.available() && Serial.read() == 's') {
        renderer.screenshot(Serial);
        lastFrame = millis();  // Do not fast-forward animation after serial capture.
    }
    if (now - lastReport >= 5000) {
        Serial.printf("DEMO alive: %u frames/5s, active=%d, heap=%u\n",
                      frames, office.getActiveAgentCount(), ESP.getFreeHeap());
        lastReport = now;
        frames = 0;
    }
}
