#include <cmath>
#include <cstdint>
#include <iostream>

using namespace std;

extern "C" {

uint32_t log_wrapper(uint32_t n) {
    return std::log2(n);
}

uint32_t get_rightmost_unset_bit(uint32_t n) {
    for (uint32_t index = 0; index < 32; ++index) {
        if ((n & (1 << index)) == 0) {
            return 1 << index;
        }
    }
    return 0;
}
}
