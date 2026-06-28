#pragma once
#include <cstdint>
#include <vector>

// === Set-associative L1 Cache ===
// 
// Stores only tags, answers if there was hit or miss, and tracks the number of 
// hits, misses, and accesses. Used for both I-cache (indexed by the instruction PC)
// and D-cache (indexed by the effective memory address)
//
//   num_sets = cache_size / (associativity * line_size)
//   block    = address  >> log2(line_size)  // drop the byte offset
//   set      = block &  (num_sets - 1)      // which set to probe
//   tag      = block >> log2(num_sets)      // identity stored in the way

class Cache {
private:
    // Store these parameters to avoid recomputing their log during access call
    int associativity;
    int block_shift;
    int tag_shift;

    uint64_t set_mask;
    std::vector<std::vector<uint64_t>> sets;

public:
    Cache(int cache_size, int line_size, int associativity);
    
    // Returns true on hit and false on miss
    bool access(uint64_t address);

    long long accesses = 0;
    long long misses = 0;
    long long hits = 0;
};
