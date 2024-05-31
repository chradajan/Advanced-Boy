#pragma once

#include <cstdint>

namespace Audio
{
struct SOUND1CNT_L
{
    uint16_t step : 3;
    uint16_t direction : 1;
    uint16_t pace : 3;
    uint16_t : 9;
};

struct SOUND1CNT_H
{
    uint16_t initialLengthTimer : 6;
    uint16_t waveDuty : 2;
    uint16_t pace : 3;
    uint16_t direction : 1;
    uint16_t initialVolume : 4;
};

struct SOUND1CNT_X
{
    uint16_t period : 11;
    uint16_t : 3;
    uint16_t lengthEnable : 1;
    uint16_t trigger : 1;
};

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
    uint16_t chan1On : 1;
    uint16_t chan2On : 1;
    uint16_t chan3On : 1;
    uint16_t chan4On : 1;
    uint16_t : 3;
    uint16_t masterEnable : 1;
    uint16_t : 8;
};

struct SOUNDBIAS
{
    uint16_t : 1;
    uint16_t biasLevel : 9;
    uint16_t : 4;
    uint16_t samplingCycle : 2;
};
}
