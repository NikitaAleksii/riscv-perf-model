#include "cache.hpp"
#include <cassert>

// Custom log2 for integers due to different implementations of log2 functions
// which can truncate the result incorrectly
namespace {
    int ilog2(int x) {
        int r = 0;
        while (x > 1) {
            x >>= 1;
            r++;
        }
        return r;
    }
}

Cache::Cache(int cache_size, int line_size, int associativity) 
    : associativity(associativity), block_shift(ilog2(line_size)) {
    assert((line_size & (line_size - 1)) == 0 && "line_size must be a power of 2");
    assert(associativity > 0 && "associativity must be positive");
    assert(cache_size % (line_size * associativity) == 0 && "cache_size must divide evenly into sets");

    int num_sets = cache_size / (line_size * associativity);
    assert((num_sets & (num_sets - 1)) == 0 && "num_sets must be a power of 2");

    set_mask = static_cast<uint64_t>(num_sets) - 1;
    tag_shift = ilog2(num_sets);

    sets.assign(num_sets, {});
}

bool Cache::access(uint64_t address) {
    accesses++;

    uint64_t block = address >> block_shift;
    uint64_t set = block & set_mask;
    uint64_t tag = block >> tag_shift;

    auto& lines = sets[set];

    // Scan the lines of the set for a matching set
    for (auto it = lines.begin(); it != lines.end(); ++it) {
        // HIT
        if (*it == tag) {
            lines.erase(it);
            lines.insert(lines.begin(), tag);
            hits++;
            return true;
        }
    }

    // MISS
    lines.insert(lines.begin(), tag);
    if (static_cast<int>(lines.size()) > associativity) {
        lines.pop_back();
    }
    misses++;
    return false;
}