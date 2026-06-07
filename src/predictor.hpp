#pragma once
#include <cstdint>
#include <vector>

struct Predictor {
    virtual ~Predictor() = default;
    virtual bool predict(uint64_t pc) = 0; // pure virtual
    virtual void update(uint64_t pc, bool taken) = 0;
};

struct AlwaysNotTakenPredictor: public Predictor {
    bool predict(uint64_t pc) override;
    void update(uint64_t pc, bool taken) override;
};

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

