// midiplay

// Copyright 2022 by Steve Clark

#ifndef __SYNTH_H__
#define __SYNTH_H__

#define VOICES  24

typedef unsigned short  WORD;
typedef unsigned char   BYTE;
typedef unsigned int    UINT;

void Synth_Generate(short *, int);
void Synth_KeyOn(int);
void Synth_KeyOff(int);
void Synth_SetFrequency(int, int, int);
void Synth_SetVolume(int, int);
void Synth_SetPan(int, int);

#endif

// midiplay