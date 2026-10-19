#include "audio.h"
#include "assets.h"

#include <SDL.h>
#include <iostream>

Audio OpenAudio() {
    Audio audio = {false, nullptr, nullptr, nullptr, nullptr};

    audio.ok = SDL_InitSubSystem(SDL_INIT_AUDIO) == 0 &&
               Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == 0;
    if (!audio.ok) {
        std::cerr << "Audio disabled: " << Mix_GetError() << std::endl;
        return audio;
    }

    audio.jump = Mix_LoadWAV(AssetPath("jump.wav").c_str());
    audio.collision = Mix_LoadWAV(AssetPath("collision.wav").c_str());
    audio.shoot = Mix_LoadWAV(AssetPath("shoot.wav").c_str());
    audio.music = Mix_LoadMUS(AssetPath("background.mp3").c_str());

    // Set background music volume
    if (audio.music) {
        Mix_PlayMusic(audio.music, -1);
        Mix_VolumeMusic(MIX_MAX_VOLUME * 4 / 100);
    }
    return audio;
}

void CloseAudio(Audio& audio) {
    Mix_FreeChunk(audio.jump);
    Mix_FreeChunk(audio.collision);
    Mix_FreeChunk(audio.shoot);
    Mix_FreeMusic(audio.music);
    if (audio.ok) Mix_CloseAudio();
    audio = {false, nullptr, nullptr, nullptr, nullptr};
}

void PlaySound(Mix_Chunk* chunk) {
    if (chunk) Mix_PlayChannel(-1, chunk, 0);
}
