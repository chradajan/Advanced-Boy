#pragma once

#include <cstdint>

union Gamepad
{
    /// @brief Set unused bits to 0 and all keys to 1 (released state).
    Gamepad() { halfword_ = 0x03FF; }

    struct
    {
        uint16_t A      : 1;
        uint16_t B      : 1;
        uint16_t Select : 1;
        uint16_t Start  : 1;
        uint16_t Right  : 1;
        uint16_t Left   : 1;
        uint16_t Up     : 1;
        uint16_t Down   : 1;
        uint16_t R      : 1;
        uint16_t L      : 1;
        uint16_t        : 4;
        uint16_t IRQ    : 1;
        uint16_t Cond   : 1;
    } buttons_;

    uint16_t halfword_;
};
