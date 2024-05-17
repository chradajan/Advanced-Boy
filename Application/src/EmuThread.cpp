#include <EmuThread.hpp>
#include <cstdint>
#include <functional>
#include <string>
#include <AdvancedBoy.hpp>
#include <SDL2/SDL.h>
#include <QtCore/QtCore>

namespace
{
void AudioCallback(void*, uint8_t* stream, int bufferSize)
{
    int numSamples = (bufferSize / sizeof(float)) / 2;
    ::FillAudioBuffer(numSamples);
    ::DrainAudioBuffer(reinterpret_cast<float*>(stream));
}
}

EmuThread::EmuThread(fs::path romPath, fs::path biosPath, bool logging)
{
    Initialize(biosPath);

    gamePakSuccessfullyLoaded_ = InsertCartridge(romPath);
    EnableLogging(logging);

    // Audio startup
    SDL_Init(SDL_INIT_AUDIO);
    SDL_AudioSpec audioSpec = {};
    audioSpec.freq = 44100;
    audioSpec.format = AUDIO_F32SYS;
    audioSpec.channels = 2;
    audioSpec.samples = 512;
    audioSpec.callback = &AudioCallback;
    audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &audioSpec, nullptr, 0);
}

std::string EmuThread::RomTitle() const
{
    return ::RomTitle();
}

void EmuThread::Play()
{
    SDL_UnlockAudioDevice(audioDevice_);
    SDL_PauseAudioDevice(audioDevice_, 0);
}

void EmuThread::Pause()
{
    SDL_LockAudioDevice(audioDevice_);
    SDL_PauseAudioDevice(audioDevice_, 1);
    SDL_Delay(20);
}
