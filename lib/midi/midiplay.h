// midiplay

// Copyright 2022 by Steve Clark

#ifndef __MIDIPLAY_H__
#define __MIDIPLAY_H__

// comment this line if the score time is not required
#define MP_TIME

// Midiplay_Init
// samplerate   The samplerate of the audio output stream
void Midiplay_Init(int);

// Midiplay_Load
// data         Memory pointer to the music data
// size         Size of the music data in bytes

// Returns 1 on success, 0 on failure
int Midiplay_Load(void *, int);

// Midiplay_Play
// playing      0 = pause, 1 = play
void Midiplay_Play(int);

// Midiplay_Volume
// volume       Range 0 to 127, 0 = minimum, 127 = maximum
void Midiplay_SetVolume(int);

// Midiplay_Output
// buffer       Pointer to pre-allocated buffer
// length       Length of buffer

// Music is rendered as stereo 16bit signed samples
void Midiplay_Output(short *, int);

// Midiplay_IsPlaying
// Returns 0 if not playing
// Paused/looping music will still return 1
int Midiplay_IsPlaying(void);

// Midiplay_Time
// Returns time in 1/10ths second

// Calling this immediately after Midiplay_Load returns the length of the music
// Calling this while Midiplay_IsPlaying returns the current position
#ifdef MP_TIME
int Midiplay_Time(void);
#endif
// Midiplay_Loop
// Changes looping mode (0 = once, 1 = repeating)
void Midiplay_Loop(int);

// Midiplay_Restart
// Restarts playback to beginning of music
void Midiplay_Restart(void);

#endif

// midiplay