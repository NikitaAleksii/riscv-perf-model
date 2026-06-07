#pragma once
#include <cstdint>
#include <vector>
#include <cmath>

struct Predictor {
    virtual ~Predictor() = default;
    virtual bool predict(uint64_t pc) = 0; // pure virtual
    virtual void update(uint64_t pc, bool taken) = 0;
};

// === Always Not Taken Predictor
struct AlwaysNotTakenPredictor: public Predictor {
    bool predict(uint64_t pc) override;
    void update(uint64_t pc, bool taken) override;
};

// === Bimodal Predictor ===
class BimodalPredictor: public Predictor {
public:
    explicit BimodalPredictor(int table_size = 8192);
    bool predict(uint64_t pc) override;
    void update(uint64_t pc, bool taken) override;
private:
    std::vector<uint8_t> counters;
    size_t index(uint64_t pc) {
        return (pc >> 2) & (counters.size() - 1);
    }
};

// === Gshare Predictor ===
class GsharePredictor: public Predictor {
public:
    explicit GsharePredictor(int table_size = 1024);
    bool predict(uint64_t pc) override;
    void update(uint64_t pc, bool taken) override;
private:
    std::vector<uint8_t> counters;
    uint64_t history = 0;
    int history_bits;
    uint64_t history_mask;
    size_t index(uint64_t pc) const {
        return ((pc >> 2) ^ history) & (counters.size() - 1);
    }
};