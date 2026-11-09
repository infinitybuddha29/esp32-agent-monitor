#include "demo_renderer.h"
#include "sprites/characters.h"
#include "sprites/furniture.h"
#include "sprites/bubbles.h"

bool DemoRenderer::begin(Adafruit_ST7789& display) {
    _display = &display;
    _canvas.setTextWrap(false);
    return _canvas.getBuffer() != nullptr;
}

void DemoRenderer::sprite(int x, int y, const uint16_t* data, int w, int h, bool flip) {
    if (y + h <= 0 || y >= SCREEN_H || x + w <= 0 || x >= SCREEN_W) return;
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            uint16_t color = pgm_read_word(data + row * w + (flip ? w - 1 - col : col));
            if (color) _canvas.drawPixel(x + col, y + row, color);
        }
    }
}

void DemoRenderer::furniture() {
    // Use only free, programmatically drawn fallback furniture from upstream.
    for (const auto& ws : WORKSTATIONS) {
        sprite(ws.seatCol * TILE_SIZE, ws.seatRow * TILE_SIZE,
               SPRITE_CHAIR, SPRITE_CHAIR_W, SPRITE_CHAIR_H);
        sprite(ws.deskCol * TILE_SIZE, ws.deskRow * TILE_SIZE,
               SPRITE_DESK, SPRITE_DESK_W, SPRITE_DESK_H);
    }
    for (int col : {1, 4, 6, 14, 16, 18}) {
        int row = col >= 14 ? 5 : 0;
        sprite(col * TILE_SIZE, row * TILE_SIZE,
               SPRITE_BOOKSHELF, SPRITE_BOOKSHELF_W, SPRITE_BOOKSHELF_H);
    }
    sprite(12 * TILE_SIZE, 4, SPRITE_COOLER, SPRITE_COOLER_W, SPRITE_COOLER_H);
    sprite(15 * TILE_SIZE, 7, SPRITE_WHITEBOARD, SPRITE_WHITEBOARD_W, SPRITE_WHITEBOARD_H);
    for (int col : {0, 9, 18}) {
        sprite(col * TILE_SIZE, 195, SPRITE_PLANT, SPRITE_PLANT_W, SPRITE_PLANT_H);
    }
    sprite(8 * TILE_SIZE, 8, SPRITE_PLANT, SPRITE_PLANT_W, SPRITE_PLANT_H);
}

void DemoRenderer::character(const Character& ch) {
    int frame = 1;
    if (ch.state == CharState::WALK) {
        static constexpr int walk[] = {0, 1, 2, 1};
        frame = walk[ch.frame % 4];
    } else if (ch.state == CharState::TYPE) {
        frame = 3 + ch.frame % 2;
    } else if (ch.state == CharState::READ ||
               (ch.state == CharState::ACTIVITY && ch.idleActivity == IdleActivity::READING)) {
        frame = 5 + ch.frame % 2;
    }
    int direction = ch.dir == Dir::LEFT ? int(Dir::RIGHT) : int(ch.dir);
    int index = direction * FRAMES_PER_DIR + frame;
    if (index < 0 || index >= CHAR_TEMPLATE_COUNT) return;
    int sitting = ch.state == CharState::TYPE || ch.state == CharState::READ
                  ? SITTING_OFFSET_PX : 0;
    int x = int(ch.x) - CHAR_W / 2;
    int y = int(ch.y) + sitting - CHAR_H;
    _canvas.fillEllipse(int(ch.x), int(ch.y) - 1, 6, 2, 0x3186);
    const uint8_t* pixels = CHAR_TEMPLATES[index];
    const uint16_t* palette = CHAR_PALETTES[ch.palette % NUM_PALETTES];
    for (int row = 0; row < CHAR_H; ++row) {
        for (int col = 0; col < CHAR_W; ++col) {
            int src = ch.dir == Dir::LEFT ? CHAR_W - 1 - col : col;
            uint8_t value = pgm_read_byte(pixels + row * CHAR_W + src);
            if (value > 0 && value <= PALETTE_COLORS) {
                _canvas.drawPixel(x + col, y + row, pgm_read_word(palette + value - 1));
            }
        }
    }
    if (ch.bubbleType == 1) {
        sprite(int(ch.x) - SPRITE_BUBBLE_PERMISSION_W / 2,
               y - SPRITE_BUBBLE_PERMISSION_H - 2,
               SPRITE_BUBBLE_PERMISSION, SPRITE_BUBBLE_PERMISSION_W, SPRITE_BUBBLE_PERMISSION_H);
    } else if (ch.bubbleType == 2) {
        sprite(int(ch.x) - SPRITE_BUBBLE_WAITING_W / 2,
               y - SPRITE_BUBBLE_WAITING_H - 2,
               SPRITE_BUBBLE_WAITING, SPRITE_BUBBLE_WAITING_W, SPRITE_BUBBLE_WAITING_H);
    }
}

void DemoRenderer::labels(const Character* chars) {
    static const char* names[MAX_DESK_AGENTS] = {
        "CODEX 1", "OPENCODE 1", "CODEX 2", "OPENCODE 2", "CODEX 3", "OPENCODE 3"
    };
    struct Rect { int x, y, w, h; } occupied[MAX_DESK_AGENTS];
    int count = 0;
    for (int i = 0; i < MAX_DESK_AGENTS; ++i) {
        const Character& ch = chars[i];
        if (!ch.alive) continue;
        int sitting = ch.state == CharState::TYPE || ch.state == CharState::READ
                      ? SITTING_OFFSET_PX : 0;
        int headX = int(ch.x), headY = int(ch.y) + sitting - CHAR_H;
        int w = int(strlen(names[i])) * 6 + 12;
        Rect tag{max(1, min(SCREEN_W - w - 1, headX - w / 2)),
                 max(1, headY - 12), w, 11};
        // Prefer immediately above the head, then move up/sideways to keep names readable.
        bool placed = false;
        for (int attempt = 0; attempt < 36 && !placed; ++attempt) {
            int dx = attempt < 12 ? 0 : attempt < 24 ? -w / 2 : w / 2;
            int dy = (attempt % 12) * 12;
            tag.x = max(1, min(SCREEN_W - w - 1, headX - w / 2 + dx));
            tag.y = max(1, min(212, headY - 12 - dy));
            placed = true;
            for (int j = 0; j < count; ++j) {
                const Rect& other = occupied[j];
                if (tag.x < other.x + other.w + 2 && tag.x + tag.w + 2 > other.x &&
                    tag.y < other.y + other.h + 2 && tag.y + tag.h + 2 > other.y) {
                    placed = false;
                    break;
                }
            }
        }
        occupied[count++] = tag;
        uint16_t provider = i % 2 ? 0x7FF3 : 0x7DFF;
        bool working = ch.state == CharState::TYPE || ch.state == CharState::READ;
        uint16_t status = working ? 0x07E0 : ch.state == CharState::WALK ? 0xFFE0 : 0x8410;
        _canvas.drawLine(headX, max(0, headY), tag.x + tag.w / 2, tag.y + tag.h, provider);
        _canvas.fillRect(tag.x, tag.y, tag.w, tag.h, 0x1082);
        _canvas.drawRect(tag.x, tag.y, tag.w, tag.h, provider);
        _canvas.fillRect(tag.x + 3, tag.y + 4, 3, 3, status);
        _canvas.setTextSize(1);
        _canvas.setTextColor(COLOR_TEXT);
        _canvas.setCursor(tag.x + 9, tag.y + 2);
        _canvas.print(names[i]);
    }
}

void DemoRenderer::render(OfficeState& office) {
    _canvas.fillScreen(COLOR_BG);
    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            _canvas.fillRect(col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE,
                             (row + col) % 2 ? COLOR_FLOOR : COLOR_FLOOR_ALT);
        }
    }
    furniture();
    const Character* chars = office.getCharacters();
    int order[MAX_DESK_AGENTS];
    for (int i = 0; i < MAX_DESK_AGENTS; ++i) order[i] = i;
    for (int i = 1; i < MAX_DESK_AGENTS; ++i) {
        int current = order[i], j = i - 1;
        while (j >= 0 && chars[order[j]].y > chars[current].y) {
            order[j + 1] = order[j];
            --j;
        }
        order[j + 1] = current;
    }
    for (int i : order) if (chars[i].alive) character(chars[i]);
    // Foreground monitors: visible even when someone is sitting at the desk.
    for (const auto& ws : WORKSTATIONS) {
        sprite(ws.deskCol * TILE_SIZE + 8, ws.deskRow * TILE_SIZE,
               SPRITE_PC, SPRITE_PC_W, SPRITE_PC_H);
    }
    labels(chars);
    _canvas.fillRect(0, 224, SCREEN_W, 16, COLOR_STATUS);
    _canvas.setTextSize(1);
    _canvas.setTextColor(0x7FF3);
    _canvas.setCursor(5, 229);
    _canvas.print("PIXEL OFFICE");
    _canvas.setTextColor(COLOR_TEXT);
    _canvas.setCursor(153, 229);
    _canvas.print("DEMO / SIMULATED");
    _display->drawRGBBitmap(0, 0, _canvas.getBuffer(), SCREEN_W, SCREEN_H);
}

void DemoRenderer::screenshot(Stream& stream) {
    // PXOF + big-endian dimensions, followed by little-endian RGB565 pixels.
    const uint8_t header[] = {'P', 'X', 'O', 'F', SCREEN_W >> 8, SCREEN_W & 255,
                             SCREEN_H >> 8, SCREEN_H & 255};
    stream.write(header, sizeof(header));
    stream.write(reinterpret_cast<const uint8_t*>(_canvas.getBuffer()), SCREEN_W * SCREEN_H * 2);
    stream.flush();
}
