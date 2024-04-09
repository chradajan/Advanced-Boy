#pragma once

#include <cstdint>

namespace Graphics
{
/// @brief LCD Control (R/W) - 0400'0000h
union DISPCNT
{
    DISPCNT(uint16_t halfword) : halfword_(halfword) {}

    struct
    {
        uint16_t bgMode : 3;
        uint16_t cgbMode : 1;
        uint16_t displayFrameSelect : 1;
        uint16_t hBlankIntervalFree : 1;
        uint16_t objCharacterVramMapping : 1;
        uint16_t forceBlank : 1;
        uint16_t screenDisplayBg0 : 1;
        uint16_t screenDisplayBg1 : 1;
        uint16_t screenDisplayBg2 : 1;
        uint16_t screenDisplayBg3 : 1;
        uint16_t screenDisplayObj : 1;
        uint16_t window0Display : 1;
        uint16_t window1Display : 1;
        uint16_t objWindowDisplay : 1;
    } flags_;

    uint16_t halfword_;
};

/// @brief General LCD Status (R/W) - 0400'0004h
union DISPSTAT
{
    DISPSTAT(uint16_t halfword) : halfword_(halfword) {}

    struct
    {
        uint16_t vBlank : 1;
        uint16_t hBlank : 1;
        uint16_t vCounter : 1;
        uint16_t vBlankIrqEnable : 1;
        uint16_t hBlankIrqEnable : 1;
        uint16_t vCounterIrqEnable : 1;
        uint16_t : 2;
        uint16_t vCountSetting : 8;
    } flags_;

    uint16_t halfword_;
};

/// @brief Vertical Counter (R) - 0400'0006
union VCOUNT
{
    VCOUNT(uint16_t halfword) : halfword_(halfword) {}

    struct
    {
        uint16_t currentScanline : 8;
        uint16_t : 8;
    } flags_;

    uint16_t halfword_;
};
}
