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

union DISPCNT
{
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
    };

    uint16_t value;
};

union DISPSTAT
{
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
    };

    uint16_t value;
};

union VCOUNT
{
    struct
    {
        uint16_t currentScanline : 8;
        uint16_t : 8;
    };

    uint16_t value;
};

union BGCNT
{
    struct
    {
        uint16_t bgPriority : 2;
        uint16_t charBaseBlock : 2;
        uint16_t : 2;
        uint16_t mosaic : 1;
        uint16_t colorMode : 1;
        uint16_t screenBaseBlock : 5;
        uint16_t overflowMode : 1;
        uint16_t screenSize : 2;
    };

    uint16_t value;
};

struct WININ
{
    uint16_t win0Bg0Enabled : 1;
    uint16_t win0Bg1Enabled : 1;
    uint16_t win0Bg2Enabled : 1;
    uint16_t win0Bg3Enabled : 1;
    uint16_t win0ObjEnabled : 1;
    uint16_t win0SpecialEffect : 1;
    uint16_t : 2;
    uint16_t win1Bg0Enabled : 1;
    uint16_t win1Bg1Enabled : 1;
    uint16_t win1Bg2Enabled : 1;
    uint16_t win1Bg3Enabled : 1;
    uint16_t win1ObjEnabled : 1;
    uint16_t win1SpecialEffect : 1;
    uint16_t : 2;
};

struct WINOUT
{
    uint16_t outsideBg0Enabled : 1;
    uint16_t outsideBg1Enabled : 1;
    uint16_t outsideBg2Enabled : 1;
    uint16_t outsideBg3Enabled : 1;
    uint16_t outsideObjEnabled : 1;
    uint16_t outsideSpecialEffect : 1;
    uint16_t : 2;
    uint16_t objWinBg0Enabled : 1;
    uint16_t objWinBg1Enabled : 1;
    uint16_t objWinBg2Enabled : 1;
    uint16_t objWinBg3Enabled : 1;
    uint16_t objWinObjEnabled : 1;
    uint16_t objWinSpecialEffect : 1;
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
    };

    uint16_t value;
};

union BLDALPHA
{
    struct
    {
        uint16_t evaCoefficient : 5;
        uint16_t : 3;
        uint16_t evbCoefficient : 5;
        uint16_t : 3;
    };

    uint16_t value;
};

union BLDY
{
    struct
    {
        uint16_t evyCoefficient : 5;
        uint16_t : 11;
    };

    uint16_t value;
};
}
