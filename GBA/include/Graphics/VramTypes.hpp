#pragma once

#include <cstdint>
#include <Utilities/MemoryUtilities.hpp>

namespace Graphics
{
constexpr uint32_t SCREENBLOCK_SIZE = 2 * KiB;
constexpr uint32_t CHARBLOCK_SIZE = 16 * KiB;
constexpr size_t OBJ_CHARBLOCK_ADDR = 4 * CHARBLOCK_SIZE;
constexpr size_t OBJ_PALETTE_ADDR = 0x0200;

struct ScreenBlockEntry
{
    uint16_t tile_ : 10;
    uint16_t horizontalFlip_ : 1;
    uint16_t verticalFlip_ : 1;
    uint16_t palette_ : 4;
};

struct ScreenBlock
{
    ScreenBlockEntry screenBlockEntry_[32][32];
};

struct TileData8bpp
{
    uint8_t paletteIndex_[8][8];
};

struct TileData4bpp
{
    struct PaletteData
    {
        uint8_t leftNibble_ : 4;
        uint8_t rightNibble_ : 4;
    };

    PaletteData paletteIndex_[8][4];
};

struct OamEntry
{
    struct
    {
        uint16_t yCoordinate_ : 8;
        uint16_t objMode_ : 2;
        uint16_t gfxMode_ : 2;
        uint16_t objMosaic_ : 1;
        uint16_t colorMode_ : 1;
        uint16_t objShape_ : 2;
    } attribute0_;

    union
    {
        struct
        {
            uint16_t : 9;
            uint16_t parameterSelection_ : 5;
            uint16_t : 2;
        } rotationOrScaling_;

        struct
        {
            uint16_t : 12;
            uint16_t horizontalFlip_ : 1;
            uint16_t verticalFlip_ : 1;
            uint16_t : 2;
        } noRotationOrScaling_;

        struct
        {
            uint16_t xCoordinate_ : 9;
            uint16_t : 5;
            uint16_t objSize_ : 2;
        } sharedFlags_;
    } attribute1_;

    struct
    {
        uint16_t tile_ : 10;
        uint16_t priority_ : 2;
        uint16_t palette_ : 4;
    } attribute2_;

    uint16_t rotationScalingData_;
};

struct TwoDim4bppMap
{
    TileData4bpp tileData_[32][32];
};

struct TwoDim8bppMap
{
    TileData8bpp tileData_[32][16];
};

struct AffineObjMatrix
{
    uint16_t filler0[3];
    int16_t pa_;

    uint16_t filler1[3];
    int16_t pb_;

    uint16_t filler2[3];
    int16_t pc_;

    uint16_t filler3[3];
    int16_t pd_;
};
}
