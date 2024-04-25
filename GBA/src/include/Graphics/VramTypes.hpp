#pragma once

#include <cstdint>
#include <System/MemoryMap.hpp>

namespace Graphics
{
constexpr uint32_t TEXT_TILE_MAP_SIZE = 2 * KiB;
constexpr uint32_t CHARBLOCK_SIZE = 16 * KiB;

struct TileMapEntry
{
    uint16_t tile_ : 10;
    uint16_t horizontalFlip_ : 1;
    uint16_t verticalFlip_ : 1;
    uint16_t palette_ : 4;
};

struct TileMap
{
    TileMapEntry tileData_[32][32];
};

struct TileData8bpp
{
    uint8_t paletteIndex_[8][8];
};

struct TileData4bpp
{
    struct PaletteIndex
    {
        uint8_t leftNibble_ : 4;
        uint8_t rightNibble_ : 4;
    };

    PaletteIndex paletteIndex_[8][4];
};
}
