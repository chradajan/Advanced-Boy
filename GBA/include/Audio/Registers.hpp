#pragma once

#include <cstdint>

namespace Audio
{
struct SOUNDCNT_L
{
    uint16_t psgRightMasterVolume : 3;
    uint16_t : 1;
    uint16_t psgLeftMasterVolume : 3;
    uint16_t : 1;
    uint16_t chan1EnableRight : 1;
    uint16_t chan2EnableRight : 1;
    uint16_t chan3EnableRight : 1;
    uint16_t chan4EnableRight : 1;
    uint16_t chan1EnableLeft : 1;
    uint16_t chan2EnableLeft : 1;
    uint16_t chan3EnableLeft : 1;
    uint16_t chan4EnableLeft : 1;
};

struct SOUNDCNT_H
{
    uint16_t psgVolume : 2;
    uint16_t dmaVolumeA : 1;
    uint16_t dmaVolumeB : 1;
    uint16_t :  4;
    uint16_t dmaEnableRightA : 1;
    uint16_t dmaEnableLeftA : 1;
    uint16_t dmaTimerSelectA : 1;
    uint16_t dmaResetA : 1;
    uint16_t dmaEnableRightB : 1;
    uint16_t dmaEnableLeftB : 1;
    uint16_t dmaTimerSelectB : 1;
    uint16_t dmaResetB : 1;
};

struct SOUNDCNT_X
{
    uint32_t chan1On : 1;
    uint32_t chan2On : 1;
    uint32_t chan3On : 1;
    uint32_t chan4On : 1;
    uint32_t : 3;
    uint32_t masterEnable : 1;
    uint32_t : 24;
};

struct SOUNDBIAS
{
    uint32_t : 1;
    uint32_t biasLevel : 9;
    uint32_t : 4;
    uint32_t samplingCycle : 2;
    uint32_t : 16;
};
}
