#include <EmuThread.hpp>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <AdvancedBoy.hpp>
#include <SDL2/SDL.h>
#include <QtCore/QThread>

namespace
{
void AudioCallback(void*, uint8_t* stream, int len)
{
    size_t cnt = len / sizeof(float);
    size_t availableSamples = ::AvailableSamplesCount();
    float* buffer = reinterpret_cast<float*>(stream);

    if (cnt > availableSamples)
    {
        ::DrainAudioBuffer(buffer, availableSamples);
        std::memset(&buffer[availableSamples], 0, sizeof(float) * cnt - availableSamples);
    }
    else
    {
        ::DrainAudioBuffer(buffer, cnt);
    }
}
}

EmuThread::EmuThread(fs::path biosPath, QObject* parent) :
    QThread(parent),
    gamePakSuccessfullyLoaded_(false)
{
    Initialize(biosPath);

    // Audio startup
    SDL_Init(SDL_INIT_AUDIO);
    SDL_AudioSpec audioSpec = {};
    audioSpec.freq = 32768;
    audioSpec.format = AUDIO_F32SYS;
    audioSpec.channels = 2;
    audioSpec.samples = 256;
    audioSpec.callback = &AudioCallback;
    audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &audioSpec, nullptr, 0);
}

void EmuThread::LoadROM(fs::path romPath)
{
    if (isRunning())
    {
        return;
    }

    gamePakSuccessfullyLoaded_ = ::InsertCartridge(romPath);
}

void EmuThread::Quit()
{
    ::PowerOff();
}

std::string EmuThread::RomTitle() const
{
    return ::RomTitle();
}

void EmuThread::StartAudioCallback()
{
    SDL_UnlockAudioDevice(audioDevice_);
    SDL_PauseAudioDevice(audioDevice_, 0);
}

void EmuThread::PauseAudioCallback()
{
    SDL_LockAudioDevice(audioDevice_);
    SDL_PauseAudioDevice(audioDevice_, 1);
}

void EmuThread::run()
{
    while (!isInterruptionRequested())
    {
        ::FillAudioBuffer();
        msleep(5);
    }
}
