#ifndef SALTARELLO_AUDIO_H
#define SALTARELLO_AUDIO_H

#include <SDL_mixer.h>

// Sounds and music. Audio is optional: if it cannot be opened every
// member stays NULL and playing is a no-op.
struct Audio {
    bool ok;
    Mix_Chunk* jump;
    Mix_Chunk* collision;
    Mix_Chunk* shoot;
    Mix_Music* music;
};

Audio OpenAudio();
void CloseAudio(Audio& audio);

// Plays a sound if audio is available (chunk may be NULL)
void PlaySound(Mix_Chunk* chunk);

#endif
