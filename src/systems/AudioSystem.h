#pragma once

#include <raylib.h>

class AudioSystem
{
public:
    AudioSystem() = default;
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    void initialize();
    void shutdown();
    void updateAmbient(bool shouldPlay);

    void playAttack() const;
    void playHit() const;
    void playPickup() const;
    void playRage() const;
    void playBossCue() const;
    void playVictory() const;

private:
    static Sound makeTone(float frequency, float duration, float sweep, float noiseAmount = 0.0F);
    static Sound makeAmbientLoop();

    Sound attack_{};
    Sound hit_{};
    Sound pickup_{};
    Sound rage_{};
    Sound bossCue_{};
    Sound victory_{};
    Sound ambient_{};
    bool initialized_ = false;
};

