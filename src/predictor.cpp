#include "predictor.hpp"
#include <cassert>

// === AlwaysNotTaken Predictor ==
// 
// Always returns untaken branch.
bool AlwaysNotTakenPredictor::predict(uint64_t pc) {
    return false;
}

void AlwaysNotTakenPredictor::update(uint64_t pc, bool taken) {
    return;
}

// === 2-bit Bimodal Predictor ===
//
// 2-bit counter per PC: 0=SNT (strongly not taken) 
//                       1=WNT (weakly not taken) 
//                       2=WT  (weakly taken) 
//                       3=ST  (strongly taken) 
//
// Predicts taken when counter >= 2.
// table_size must be a power of 2.
BimodalPredictor::BimodalPredictor(int table_size) : counters(table_size, 1) {
    assert((table_size & (table_size - 1)) == 0 && "table_size must be a power of 2");
}

bool BimodalPredictor::predict(uint64_t pc) {
    return counters[index(pc)] >= 2;
}

void BimodalPredictor::update(uint64_t pc, bool taken) {
    uint8_t& counter = counters[index(pc)];
    if (taken) {
        if (counter < 3) counter += 1;
    } else {
        if (counter > 0) counter -= 1;
    }
}

