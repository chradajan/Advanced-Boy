#pragma once

#include <cstdint>

namespace Audio
{
struct SOUNDCNT
{
    // |--------------|
    // |  SOUNDCNT_L  |
    // |--------------|

    // NR50
    uint64_t psgRightMasterVolume_ : 3;
    uint64_t : 1;
    uint64_t psgLeftMasterVolume_ : 3;
    uint64_t : 1;

    // NR51
    uint64_t chan1EnabledRight_ : 1;
    uint64_t chan2EnabledRight_ : 1;
    uint64_t chan3EnabledRight_ : 1;
    uint64_t chan4EnabledRight_ : 1;
    uint64_t chan1EnabledLeft_ : 1;
    uint64_t chan2EnabledLeft_ : 1;
    uint64_t chan3EnabledLeft_ : 1;
    uint64_t chan4EnabledLeft_ : 1;

    // |--------------|
    // |  SOUNDCNT_H  |
    // |--------------|

    uint64_t psgVolume_ : 2;
    uint64_t dmaSoundAVolume_ : 1;
    uint64_t dmaSoundBVolume_ : 1;
    uint64_t : 4;

    uint64_t dmaSoundAEnabledRight_ : 1;
    uint64_t dmaSoundAEnabledLeft_ : 1;
    uint64_t dmaSoundATimerSelect_ : 1;
    uint64_t dmaSoundAResetFifo_ : 1;

    uint64_t dmaSoundBEnabledRight_ : 1;
    uint64_t dmaSoundBEnabledLeft_ : 1;
    uint64_t dmaSoundBTimerSelect_ : 1;
    uint64_t dmaSoundBResetFifo_ : 1;

    // |--------------|
    // |  SOUNDCNT_X  |
    // |--------------|

    // NR52
    uint64_t chan1On_ : 1;  // Read only
    uint64_t chan2On_ : 1;  // Read only
    uint64_t chan3On_ : 1;  // Read only
    uint64_t chan4On_ : 1;  // Read only
    uint64_t : 3;
    uint64_t psgFifoMasterEnable_ : 1;
    uint64_t : 24;
};

struct SOUNDBIAS
{
    uint16_t : 1;
    uint16_t biasLevel_ : 9;
    uint16_t : 4;
    uint16_t samplingCycle_ : 2;
};
}
