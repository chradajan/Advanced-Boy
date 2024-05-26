#pragma once

#include <cstdint>

namespace Graphics
{
constexpr uint32_t DISPSTAT_ADDR = 0x0400'0004;
constexpr uint32_t VCOUNT_ADDR = 0x0400'0006;
constexpr uint32_t BG0CNT_ADDR = 0x0400'0008;
constexpr uint32_t BG0HOFS_ADDR = 0x0400'0010;
constexpr uint32_t WININ_ADDR = 0x0400'0048;
constexpr uint32_t MOSAIC_ADDR = 0x0400'004C;
constexpr uint32_t BLDCNT_ADDR = 0x0400'0050;
constexpr uint32_t BLDY_ADDR = 0x0400'0054;

/// @brief LCD Control (R/W) - 0400'0000h
union DISPCNT
{
    DISPCNT(uint16_t halfword) : halfword_(halfword) {}

    struct
    {
        uint16_t bgMode_ : 3;
        uint16_t cgbMode_ : 1;
        uint16_t displayFrameSelect_ : 1;
        uint16_t hBlankIntervalFree_ : 1;
        uint16_t objCharacterVramMapping_ : 1;
        uint16_t forceBlank_ : 1;
        uint16_t screenDisplayBg0_ : 1;
        uint16_t screenDisplayBg1_ : 1;
        uint16_t screenDisplayBg2_ : 1;
        uint16_t screenDisplayBg3_ : 1;
        uint16_t screenDisplayObj_ : 1;
        uint16_t window0Display_ : 1;
        uint16_t window1Display_ : 1;
        uint16_t objWindowDisplay_ : 1;
    } flags_;

    uint16_t halfword_;
};

/// @brief General LCD Status (R/W) - 0400'0004h
union DISPSTAT
{
    DISPSTAT(uint16_t halfword) : halfword_(halfword) {}

    struct
    {
        uint16_t vBlank_ : 1;
        uint16_t hBlank_ : 1;
        uint16_t vCounter_ : 1;
        uint16_t vBlankIrqEnable_ : 1;
        uint16_t hBlankIrqEnable_ : 1;
        uint16_t vCounterIrqEnable_ : 1;
        uint16_t : 2;
        uint16_t vCountSetting_ : 8;
    } flags_;

    uint16_t halfword_;
};

/// @brief Vertical Counter (R) - 0400'0006
union VCOUNT
{
    VCOUNT(uint16_t halfword) : halfword_(halfword) {}

    struct
    {
        uint16_t currentScanline_ : 8;
        uint16_t : 8;
    } flags_;

    uint16_t halfword_;
};

/// @brief Background control registers (R/W).
union BGCNT
{
    struct
    {
        uint16_t bgPriority_ : 2;
        uint16_t charBaseBlock_ : 2;
        uint16_t : 2;
        uint16_t mosaic_ : 1;
        uint16_t colorMode_ : 1;
        uint16_t screenBaseBlock_ : 5;
        uint16_t overflowMode_ : 1;
        uint16_t screenSize_ : 2;
    } flags_;

    uint16_t halfword_;
};

struct WININ
{
    uint16_t win0Bg0Enabled_ : 1;
    uint16_t win0Bg1Enabled_ : 1;
    uint16_t win0Bg2Enabled_ : 1;
    uint16_t win0Bg3Enabled_ : 1;
    uint16_t win0ObjEnabled_ : 1;
    uint16_t win0SpecialEffect_ : 1;
    uint16_t : 2;
    uint16_t win1Bg0Enabled_ : 1;
    uint16_t win1Bg1Enabled_ : 1;
    uint16_t win1Bg2Enabled_ : 1;
    uint16_t win1Bg3Enabled_ : 1;
    uint16_t win1ObjEnabled_ : 1;
    uint16_t win1SpecialEffect_ : 1;
    uint16_t : 2;
};

struct WINOUT
{
    uint16_t outsideBg0Enabled_ : 1;
    uint16_t outsideBg1Enabled_ : 1;
    uint16_t outsideBg2Enabled_ : 1;
    uint16_t outsideBg3Enabled_ : 1;
    uint16_t outsideObjEnabled_ : 1;
    uint16_t outsideSpecialEffect_ : 1;
    uint16_t : 2;
    uint16_t objWinBg0Enabled_ : 1;
    uint16_t objWinBg1Enabled_ : 1;
    uint16_t objWinBg2Enabled_ : 1;
    uint16_t objWinBg3Enabled_ : 1;
    uint16_t objWinObjEnabled_ : 1;
    uint16_t objWinSpecialEffect_ : 1;
    uint16_t : 2;
};

union BLDCNT
{
    struct
    {
        uint16_t bg0A : 1;
        uint16_t bg1A : 1;
        uint16_t bg2A : 1;
        uint16_t bg3A : 1;
        uint16_t objA : 1;
        uint16_t bdA : 1;
        uint16_t specialEffect : 2;
        uint16_t bg0B : 1;
        uint16_t bg1B : 1;
        uint16_t bg2B : 1;
        uint16_t bg3B : 1;
        uint16_t objB : 1;
        uint16_t bdB : 1;
        uint16_t : 2;
    } flags_;

    uint16_t halfword_;
};

union BLDALPHA
{
    struct
    {
        uint16_t evaCoefficient_ : 5;
        uint16_t : 3;
        uint16_t evbCoefficient_ : 5;
        uint16_t : 3;
    } flags_;

    uint16_t halfword_;
};

union BLDY
{
    struct
    {
        uint8_t evyCoefficient_ : 5;
        uint8_t : 3;
    } flags_;

    uint8_t byte_;
};
}
