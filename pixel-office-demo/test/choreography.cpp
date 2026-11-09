#include "../../firmware/PixelOfficeDemo/src/office_state.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>

int main() {
    std::srand(42);
    OfficeState office;
    office.init();
    office.spawnAllCharacters();
    int previousStep = -1, workingFrames = 0, walkingFrames = 0;
    for (int frame = 0; frame < 15 * 60 * 5; ++frame) {
        int step = (frame / (15 * 12)) % 4;
        if (step != previousStep) {
            for (uint8_t id = 0; id < 4; ++id) {
                bool working = (id + step) % 4 != 3;
                office.setAgentState(id, working ? CharState::TYPE : CharState::IDLE,
                                     working ? (id % 2 ? "Read" : "Edit") : "");
            }
            previousStep = step;
        }
        office.update(1.0f / 15);
        assert(office.getCharacterCount() == 6);
        assert(!office.isConnected());
        for (int i = 0; i < MAX_DESK_AGENTS; ++i) {
            const Character& ch = office.getCharacters()[i];
            assert(ch.alive);
            assert(std::isfinite(ch.x) && std::isfinite(ch.y));
            assert(ch.x >= 0 && ch.x < SCREEN_W);
            assert(ch.y >= 0 && ch.y < GRID_ROWS * TILE_SIZE);
            assert(ch.pathLen <= 64 && ch.pathIdx <= ch.pathLen);
            assert(ch.palette < NUM_PALETTES);
            if (ch.state == CharState::TYPE || ch.state == CharState::READ) ++workingFrames;
            if (ch.state == CharState::WALK) ++walkingFrames;
        }
    }
    assert(workingFrames > 1000 && walkingFrames > 1000);
    printf("PASS: five simulated minutes, six agents, %d working / %d walking samples; no NVS/network\n",
           workingFrames, walkingFrames);
}
