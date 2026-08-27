#include "systems/AudioSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace
{
constexpr int kSampleRate = 22050;
constexpr float kPi = 3.14159265358979323846F;

std::int16_t toSample(const float value)
{
    const float clamped = std::clamp(value, -1.0F, 1.0F);
    return static_cast<std::int16_t>(clamped * 32767.0F);
}

Sound soundFromSamples(std::vector<std::int16_t>& samples)
{
    Wave wave{};
    wave.frameCount = static_cast<unsigned int>(samples.size());
    wave.sampleRate = kSampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples.data();
    return LoadSoundFromWave(wave);
}
} // namespace

AudioSystem::~AudioSystem()
{
    shutdown();
}

void AudioSystem::initialize()
{
    if (initialized_)
    {
        return;
    }

    InitAudioDevice();
    if (!IsAudioDeviceReady())
    {
        return;
    }

    attack_ = makeTone(520.0F, 0.09F, -0.72F, 0.12F);
    hit_ = makeTone(115.0F, 0.14F, -0.55F, 0.38F);
    pickup_ = makeTone(620.0F, 0.18F, 0.85F);
    rage_ = makeTone(170.0F, 0.52F, 2.10F, 0.08F);
    bossCue_ = makeTone(92.0F, 0.82F, -0.18F, 0.10F);
    victory_ = makeTone(440.0F, 0.92F, 1.25F);
    ambient_ = makeAmbientLoop();

    SetSoundVolume(attack_, 0.36F);
    SetSoundVolume(hit_, 0.46F);
    SetSoundVolume(pickup_, 0.32F);
    SetSoundVolume(rage_, 0.42F);
    SetSoundVolume(bossCue_, 0.48F);
    SetSoundVolume(victory_, 0.48F);
    SetSoundVolume(ambient_, 0.13F);
    initialized_ = true;
}

void AudioSystem::shutdown()
{
    if (!initialized_)
    {
        return;
    }

    UnloadSound(attack_);
    UnloadSound(hit_);
    UnloadSound(pickup_);
    UnloadSound(rage_);
    UnloadSound(bossCue_);
    UnloadSound(victory_);
    UnloadSound(ambient_);
    CloseAudioDevice();
    initialized_ = false;
}

void AudioSystem::updateAmbient(const bool shouldPlay)
{
    if (!initialized_)
    {
        return;
    }
    if (shouldPlay && !IsSoundPlaying(ambient_))
    {
        PlaySound(ambient_);
    }
    else if (!shouldPlay && IsSoundPlaying(ambient_))
    {
        StopSound(ambient_);
    }
}

void AudioSystem::playAttack() const { if (initialized_) PlaySound(attack_); }
void AudioSystem::playHit() const { if (initialized_) PlaySound(hit_); }
void AudioSystem::playPickup() const { if (initialized_) PlaySound(pickup_); }
void AudioSystem::playRage() const { if (initialized_) PlaySound(rage_); }
void AudioSystem::playBossCue() const { if (initialized_) PlaySound(bossCue_); }
void AudioSystem::playVictory() const { if (initialized_) PlaySound(victory_); }

Sound AudioSystem::makeTone(const float frequency, const float duration, const float sweep, const float noiseAmount)
{
    const int frameCount = static_cast<int>(duration * kSampleRate);
    std::vector<std::int16_t> samples(static_cast<std::size_t>(frameCount));
    std::mt19937 random(static_cast<unsigned int>(frequency * 100.0F + duration * 1000.0F));
    std::uniform_real_distribution<float> noise(-1.0F, 1.0F);
    float phase = 0.0F;

    for (int frame = 0; frame < frameCount; ++frame)
    {
        const float time = static_cast<float>(frame) / kSampleRate;
        const float progress = time / duration;
        const float currentFrequency = frequency * (1.0F + sweep * progress);
        phase += 2.0F * kPi * currentFrequency / kSampleRate;
        const float attackEnvelope = std::min(1.0F, time / 0.012F);
        const float releaseEnvelope = std::clamp((duration - time) / std::min(0.12F, duration * 0.65F), 0.0F, 1.0F);
        const float envelope = attackEnvelope * releaseEnvelope;
        const float tone = std::sin(phase) * 0.72F + std::sin(phase * 2.0F) * 0.18F;
        samples[static_cast<std::size_t>(frame)] = toSample((tone + noise(random) * noiseAmount) * envelope * 0.55F);
    }
    return soundFromSamples(samples);
}

Sound AudioSystem::makeAmbientLoop()
{
    constexpr float duration = 4.0F;
    constexpr float notes[]{110.0F, 146.83F, 164.81F, 146.83F, 110.0F, 130.81F, 164.81F, 196.0F};
    const int frameCount = static_cast<int>(duration * kSampleRate);
    std::vector<std::int16_t> samples(static_cast<std::size_t>(frameCount));
    float phase = 0.0F;
    float bassPhase = 0.0F;
    for (int frame = 0; frame < frameCount; ++frame)
    {
        const float time = static_cast<float>(frame) / kSampleRate;
        const int noteIndex = std::min(7, static_cast<int>(time / 0.5F));
        const float withinNote = std::fmod(time, 0.5F);
        const float noteEnvelope = std::clamp((0.48F - withinNote) / 0.08F, 0.0F, 1.0F);
        phase += 2.0F * kPi * notes[noteIndex] / kSampleRate;
        bassPhase += 2.0F * kPi * 55.0F / kSampleRate;
        const float lead = (std::sin(phase) >= 0.0F ? 1.0F : -1.0F) * 0.10F * noteEnvelope;
        const float bass = std::sin(bassPhase) * 0.13F;
        samples[static_cast<std::size_t>(frame)] = toSample(lead + bass);
    }
    return soundFromSamples(samples);
}

