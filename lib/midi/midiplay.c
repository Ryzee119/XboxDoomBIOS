// midiplay

// Copyright 2022 by Steve Clark

#include "synth.h"

#include "midiplay.h"

#define ID(i, a)    (i[0] == a[0] && i[1] == a[1] && i[2] == a[2] && i[3] == a[3])

#define LE16(i)     ((i)[0] | ((i)[1] << 8))
#define BE16(i)     (((i)[0] << 8) | (i)[1])
#define BE32(i)     ((BE16(i) << 16) | BE16(i + 2))

// timer -----------------------------------------------------------------------
typedef struct
{
    int rate;
    int acc;
    int remainder;
    int divisor;
}
TIMER;

static TIMER    timerPhase, timerSecond, timerBeat;

// event -----------------------------------------------------------------------
#define NOTE_OFF        0
#define NOTE_PLAY       1

#define CC_01           1   // mod wheel
#define CC_06           6   // data entry coarse
#define CC_07           7   // volume
#define CC_0a           10  // pan
#define CC_0b           11  // expression
#define CC_40           64  // sustain
#define CC_64           100 // reg lsb
#define CC_65           101 // reg msb
#define CC_78           120 // all sounds off
#define CC_79           121 // reset controllers
#define CC_7b           123 // all notes off
#define CC_80           128 // MUS instrument change
#define CC_ff           255

typedef struct
{
    BYTE    channel;
    BYTE    data[3];
}
EVENT;

typedef struct _voice   VOICE;
struct _voice
{
    VOICE   *prev, *next;

    int     index;
    int     note;
    int     volume;
    int     playing;
};

typedef struct
{
    VOICE   voice;
    int     instrument;
    int     volume;
    int     pan;
    int     sustain;
    int     bend;
    int     bendRange;
    int     expression;
    int     rpn;
}
CHANNEL;

static UINT     volumeTable[128] =
{
    0x000, 0x001, 0x002, 0x004, 0x005, 0x007, 0x008, 0x009, 0x00b, 0x00c, 0x00e, 0x00f, 0x011, 0x012, 0x014, 0x015,
    0x017, 0x018, 0x01a, 0x01b, 0x01d, 0x01f, 0x020, 0x022, 0x023, 0x025, 0x027, 0x028, 0x02a, 0x02b, 0x02d, 0x02f,
    0x030, 0x032, 0x034, 0x035, 0x037, 0x039, 0x03b, 0x03c, 0x03e, 0x040, 0x041, 0x043, 0x045, 0x047, 0x049, 0x04a,
    0x04c, 0x04e, 0x050, 0x052, 0x054, 0x055, 0x057, 0x059, 0x05b, 0x05d, 0x05f, 0x061, 0x063, 0x065, 0x067, 0x069,
    0x06b, 0x06d, 0x06f, 0x071, 0x073, 0x075, 0x077, 0x079, 0x07b, 0x07d, 0x07f, 0x081, 0x083, 0x085, 0x087, 0x089,
    0x08c, 0x08e, 0x090, 0x092, 0x094, 0x097, 0x099, 0x09b, 0x09d, 0x0a0, 0x0a2, 0x0a4, 0x0a6, 0x0a9, 0x0ab, 0x0ad,
    0x0b0, 0x0b2, 0x0b5, 0x0b7, 0x0b9, 0x0bc, 0x0be, 0x0c1, 0x0c3, 0x0c6, 0x0c8, 0x0cb, 0x0cd, 0x0d0, 0x0d2, 0x0d5,
    0x0d7, 0x0da, 0x0dc, 0x0df, 0x0e2, 0x0e4, 0x0e7, 0x0ea, 0x0ec, 0x0ef, 0x0f2, 0x0f4, 0x0f7, 0x0fa, 0x0fd, 0x100
};

static CHANNEL  midChannel[16];
static VOICE    midVoice[VOICES], unusedVoice;

static EVENT    *eventData;

// control ---------------------------------------------------------------------
#define MICROSEC    1000000

typedef void (*DOEVENT)(void);

typedef struct
{
    BYTE    *data, *pos;
    int     clock; // accumulator
    BYTE    running;
    DOEVENT DoEvent;
    EVENT   event;
    int     done;
}
TRACK;

static TRACK    midTrack[65536], *curTrack, *endTrack;
static int      numTracks, numTracksEnded;

static BYTE     prevVolume[16]; // last known note volume on channel

static int      beatTicks = 96, beatTempo = 500000;
static int      playSamples;
static int      musicClock;
#ifdef MP_TIME
static int      timeTicks, timeTempo;
#endif
static DOEVENT  MusicEvents;

// controller map for MUS
static int      controllerMap[16] = // CMD_TYPE would suggest only 16
{
    CC_80, CC_ff, CC_01, CC_07, CC_0a, CC_0b, CC_ff, CC_ff,
    CC_40, CC_ff, CC_78, CC_7b, CC_ff, CC_ff, CC_79, CC_ff
};

static void (*AddEvent)(DOEVENT);

// midiplay --------------------------------------------------------------------
#define MUS_HDRSIZE     16
#define MID_HDRSIZE     14

static int  musicInit = 0;
static int  musicLooping;
static int  musicPlaying = 0;
static int  musicVolume = 0x100;

// timer -----------------------------------------------------------------------
static int Timer_Update(TIMER *timer)
{
    timer->acc += timer->remainder;
    if (timer->acc < timer->divisor)
    {
        return timer->rate;
    }

    timer->acc -= timer->divisor;

    return timer->rate + 1;
}

static void Timer_Set(TIMER *timer, int numerator, int divisor)
{
    timer->acc = 0;
    timer->rate = numerator / divisor;
    timer->remainder = numerator - timer->rate * divisor;
    timer->divisor = divisor;
}

// event -----------------------------------------------------------------------
static void ResetChannel(int channel)
{
    midChannel[channel].sustain = 0;
    midChannel[channel].bend = 0;
    midChannel[channel].expression = volumeTable[127];
    midChannel[channel].rpn = (127 << 7) | 127;
}

static void Voice_Off(VOICE *voice)
{
    Synth_KeyOff(voice->index);

    voice->prev->next = voice->next;
    voice->next->prev = voice->prev;

    voice->next = &unusedVoice;
    voice->prev = unusedVoice.prev;
    unusedVoice.prev = voice;
    voice->prev->next = voice;
}

static void VoiceVolume(CHANNEL *channel, VOICE *voice)
{
    Synth_SetVolume(voice->index, channel->volume * channel->expression * voice->volume >> 16);
}

static void AllSoundOff(CHANNEL *channel)
{
    while (channel->voice.next != &channel->voice)
    {
        Synth_SetVolume(channel->voice.next->index, 0x00);
        Voice_Off(channel->voice.next);
    }
}

static void ResetControllers()
{
    ResetChannel(eventData->channel);
}

static void Event_NoteOff()
{
    CHANNEL *channel = &midChannel[eventData->channel];
    int     note = eventData->data[0];
    //int         volume = eventData->data[1];
    VOICE   *voice = channel->voice.next;

    while (voice != &channel->voice)
    {
        if (voice->note == note)
        {
            if (channel->sustain)
            {
                voice->playing = NOTE_OFF;
            }
            else
            {
                voice = voice->prev;
                Voice_Off(voice->next);
            }
        }
        voice = voice->next;
    }
}

static void FrequencyStep(VOICE *voice, CHANNEL *channel)
{
    int index, octave;

    index = voice->note * 32 + channel->bend * channel->bendRange / 2;
    if (index < 0)
    {
        index = 0;
    }
    else if (index > 128 * 32 - 1)
    {
        index = 128 * 32 - 1;
    }

    octave = index / 384;
    index %= 384;

    Synth_SetFrequency(voice->index, index, octave);
}

static void Event_NoteOn()
{
    CHANNEL *channel = &midChannel[eventData->channel];
    int     note = eventData->data[0];
    int     volume = eventData->data[1];
    VOICE   *voice;

    voice = channel->voice.next;
    while (voice != &channel->voice)
    {
        if (voice->note == note)
        {
            voice->volume = volumeTable[volume];
            VoiceVolume(channel, voice);
            voice->playing = NOTE_PLAY;
        }
        voice = voice->next;
    }

    if (unusedVoice.next == &unusedVoice)
    {
        return;
    }

    voice = unusedVoice.next;
    unusedVoice.next = voice->next;
    voice->next->prev = &unusedVoice;
    voice->next = channel->voice.next;
    voice->next->prev = voice;
    channel->voice.next = voice;
    voice->prev = &channel->voice;

    voice->note = note;
    voice->volume = volumeTable[volume];
    VoiceVolume(channel, voice);
    Synth_SetPan(voice->index, channel->pan);
    FrequencyStep(voice, channel);
    Synth_KeyOn(voice->index);
    voice->playing = NOTE_PLAY;
}

static void Event_MuteNotes()
{
    CHANNEL *channel = &midChannel[eventData->channel];
    VOICE   *voice;

    if (!channel->sustain)
    {
        while (channel->voice.next != &channel->voice)
        {
            Voice_Off(channel->voice.next);
        }

        return;
    }

    voice = channel->voice.next;
    while (voice != &channel->voice)
    {
        voice->playing = NOTE_OFF;
        voice = voice->next;
    }
}

static void Event_PitchWheel()
{
    CHANNEL *channel = &midChannel[eventData->channel];
    int     bend = ((eventData->data[0] & 0x7f) | (eventData->data[1] << 7));
    VOICE   *voice = channel->voice.next;

    channel->bend = (bend >> 7) - 64; // 7 bit values

    while (voice != &channel->voice)
    {
        FrequencyStep(voice, channel);
        voice = voice->next;
    }
}

static void Event_Aftertouch()
{
    CHANNEL *channel = &midChannel[eventData->channel];
    int     note = eventData->data[0];
    int     volume = eventData->data[1];
    VOICE   *voice = channel->voice.next;

    while (voice != &channel->voice)
    {
        if (voice->note == note)
        {
            voice->volume = volumeTable[volume];
            VoiceVolume(channel, voice);
        }
        voice = voice->next;
    }
}

static void Event_Sustain()
{
    CHANNEL *channel = &midChannel[eventData->channel];
    int     sustain = eventData->data[1];
    VOICE   *voice;

    channel->sustain = sustain < 64 ? 0 : 1;

    if (channel->sustain)
    {
        return;
    }

    voice = channel->voice.next;
    while (voice != &channel->voice)
    {
        if (voice->playing == NOTE_OFF)
        {
            voice = voice->prev;
            Voice_Off(voice->next);
        }

        voice = voice->next;
    }
}

static void Event_ChannelVolume()
{
    CHANNEL *channel = &midChannel[eventData->channel];
    int     volume = eventData->data[1];
    VOICE   *voice = channel->voice.next;

    channel->volume = volumeTable[volume & 0x7f];

    while (voice != &channel->voice)
    {
        VoiceVolume(channel, voice);
        voice = voice->next;
    }
}

static void Event_Pan()
{
    CHANNEL *channel = &midChannel[eventData->channel];
    int     pan = eventData->data[1] & 0x7f;
    VOICE   *voice = channel->voice.next;

    channel->pan = pan;

    while (voice != &channel->voice)
    {
        Synth_SetPan(voice->index, pan);
        voice = voice->next;
    }
}

static void Event_ChannelAftertouch()
{
    CHANNEL *channel = &midChannel[eventData->channel];
    int     volume = eventData->data[0];
    VOICE   *voice = channel->voice.next;

    while (voice != &channel->voice)
    {
        voice->volume = volumeTable[volume];
        VoiceVolume(channel, voice);
        voice = voice->next;
    }
}

static void Event_Expression()
{
    CHANNEL *channel = &midChannel[eventData->channel];
    int     expression = eventData->data[1] & 0x7f;
    VOICE   *voice = channel->voice.next;

    channel->expression = volumeTable[expression];

    while (voice != &channel->voice)
    {
        VoiceVolume(channel, voice);
        voice = voice->next;
    }
}

static void Event_ChangeInstrument()
{
    CHANNEL *channel = &midChannel[eventData->channel];

    channel->instrument = eventData->data[1];
}

static void DataEntry_Coarse()
{
    CHANNEL *channel = &midChannel[eventData->channel];

    if (channel->rpn == 0)
    {
        channel->bendRange = eventData->data[1];
    }
}

static void Event_Message()
{
    switch (eventData->data[0])
    {
      case CC_01:
        break;

      case CC_06:
        DataEntry_Coarse();
        break;

      case CC_07:
        Event_ChannelVolume();
        break;

      case CC_0a:
        Event_Pan();
        break;

      case CC_0b:
        Event_Expression();
        break;

      case CC_40: // FIXME
        //Event_Sustain();
        break;

      case CC_64:
        midChannel[eventData->channel].rpn &= (127 << 7);
        midChannel[eventData->channel].rpn |= eventData->data[1];
        break;

      case CC_65:
        midChannel[eventData->channel].rpn &= 127;
        midChannel[eventData->channel].rpn |= (eventData->data[1] << 7);
        break;

      case CC_78:
        AllSoundOff(&midChannel[eventData->channel]);
        break;

      case CC_79:
        ResetControllers();
        break;

      case CC_7b:
        Event_MuteNotes();
        break;

      case CC_80:
        Event_ChangeInstrument();
        break;

      default: // event not handled
        break;
    }
}

// control ---------------------------------------------------------------------
#ifdef MP_TIME
static void UpdateScoreTime()
{
    timeTempo += beatTempo;
    timeTicks += (timeTempo / MICROSEC);
    timeTempo %= MICROSEC;
}
#endif
// sometimes, you just want nothing to happen
static void DoNothing()
{
}

static void InitTracks()
{
    int track, channel;

    for (track = 0; track < numTracks; track++)
    {
        midTrack[track].pos = midTrack[track].data;
        midTrack[track].done = 0;
        midTrack[track].clock = 0;
        midTrack[track].DoEvent = DoNothing;
    }

    for (channel = 0; channel < 16; channel++)
    {
        AllSoundOff(&midChannel[channel]);
        ResetChannel(channel);
        midChannel[channel].instrument = 0;
        midChannel[channel].volume = volumeTable[100];
        midChannel[channel].pan = 64;
        midChannel[channel].bendRange = 2;
        prevVolume[channel] = 0;
    }

    musicClock = 0;
    playSamples = 0;

    beatTempo = 500000;
    Timer_Set(&timerBeat, beatTempo, beatTicks);

    numTracksEnded = 0;
#ifdef MP_TIME
    timeTicks = timeTempo = 0;
#endif
}

static void SetTempo()
{
    // if the tempo changes, should playSamples be reset?
    beatTempo = (curTrack->event.data[0] << 16) | (curTrack->event.data[1] << 8) | curTrack->event.data[2];
    Timer_Set(&timerBeat, beatTempo, beatTicks);
}

static void EndOfTrack()
{
    curTrack->done = 1;
    numTracksEnded++;
}

static void EndOfMidiTrack()
{
    EndOfTrack();
    curTrack->DoEvent = DoNothing;
    if (numTracksEnded < numTracks)
    {
        return;
    }

    if (musicLooping == 0)
    {
        musicPlaying = 0;
        return;
    }

    InitTracks();
    // at this point we need to parse a new event from track 0
    curTrack = &midTrack[0];
    // eventData does not need to be set
}

static UINT GetLength()
{
    UINT    length;
    BYTE    data;

    // unrolled
    data = *curTrack->pos++;
    length = (data & 127);
    if (data & 128)
    {
        data = *curTrack->pos++;
        length = (length << 7) | (data & 127);
        if (data & 128)
        {
            data = *curTrack->pos++;
            length = (length << 7) | (data & 127);
            if (data & 128)
            {
                data = *curTrack->pos++;
                length = (length << 7) | (data & 127);
            }
        }
    }

    return length;
}
#ifdef MP_TIME
static void NoEvent(DOEVENT event)
{
    (void)event;
}
#endif
static void NewEvent(DOEVENT event)
{
    curTrack->DoEvent = event;
}

static int GetMusEvent(int *time)
{
    BYTE    data, last;

    curTrack->DoEvent = DoNothing;

    data = *curTrack->pos++;
    curTrack->event.channel = data & 0x0f;
    last = data & 0x80;

    *time = 0;

    switch (data & 0x70)
    {
      case 0x00: // release note
        curTrack->event.data[0] = (*curTrack->pos++ & 0x7f);
        AddEvent(Event_NoteOff);
        break;

      case 0x10: // play note
        data = *curTrack->pos++;
        curTrack->event.data[0] = (data & 0x7f);
        if (data & 0x80)
        {
            curTrack->event.data[1] = *curTrack->pos++ & 0x7f;
            // should the volume be saved if the value is 0?
            prevVolume[curTrack->event.channel] = curTrack->event.data[1];
        }
        else
        {
            curTrack->event.data[1] = prevVolume[curTrack->event.channel];
        }
        if (curTrack->event.data[1] == 0)
        {
            AddEvent(Event_NoteOff);
        }
        else
        {
            AddEvent(Event_NoteOn);
        }
        break;

      case 0x20: // pitch wheel (adjusted to 14 bit value)
        curTrack->event.data[0] = (*curTrack->pos & 0x1) << 6;
        curTrack->event.data[1] = *curTrack->pos++ >> 1;
        AddEvent(Event_PitchWheel);
        break;

      case 0x30: // system event
        curTrack->event.data[0] = controllerMap[*curTrack->pos++ & 0x0f];
        AddEvent(Event_Message);
        break;

      case 0x40: // change controller
        curTrack->event.data[0] = controllerMap[*curTrack->pos++ & 0x0f];
        curTrack->event.data[1] = *curTrack->pos++;
        AddEvent(Event_Message);
        break;

      case 0x50: // end of measure?
        break;

      case 0x60: // score end
        if (musicLooping)
        {
            AddEvent(InitTracks);
        }
        else
        {
            curTrack->DoEvent = EndOfTrack;
        }
        last = 1 - musicLooping;
        break;

      case 0x70:
        // requires one data byte but is unused
        curTrack->event.data[0] = *curTrack->pos++;
        break;
    }

    curTrack->DoEvent();

    if (last & 0x80)
    {
        *time = GetLength();
    }

    return last;
}

static void GetMidEvent()
{
    BYTE    data, event = 0x0;
    UINT    length;

    curTrack->DoEvent = DoNothing;

    data = *curTrack->pos;

    if (data & 0x80)
    {
        event = *curTrack->pos++;
    }
    else
    {
        data = curTrack->running;
    }

    curTrack->event.channel = data & 0x0f;

    switch (data & 0xf0)
    {
      case 0x80:
        curTrack->event.data[0] = *curTrack->pos++;
        curTrack->event.data[1] = *curTrack->pos++;
        AddEvent(Event_NoteOff);
        break;

      case 0x90:
        curTrack->event.data[0] = *curTrack->pos++;
        curTrack->event.data[1] = *curTrack->pos++;
        if (curTrack->event.data[1] == 0)
        {
            AddEvent(Event_NoteOff);
        }
        else
        {
            AddEvent(Event_NoteOn);
        }
        break;

      case 0xa0:
        curTrack->event.data[0] = *curTrack->pos++;
        curTrack->event.data[1] = *curTrack->pos++;
        AddEvent(Event_Aftertouch);
        break;

      case 0xb0: // controller message
        curTrack->event.data[0] = *curTrack->pos++;
        curTrack->event.data[1] = *curTrack->pos++;
        AddEvent(Event_Message);
        break;

      case 0xc0:
        // instrument number must be in 2nd byte of event.data
        //  as that's where MUS puts it
        curTrack->event.data[1] = *curTrack->pos++;
        AddEvent(Event_ChangeInstrument);
        break;

      case 0xd0:
        curTrack->event.data[0] = *curTrack->pos++;
        AddEvent(Event_ChannelAftertouch);
        break;

      case 0xe0:
        curTrack->event.data[0] = *curTrack->pos++;
        curTrack->event.data[1] = *curTrack->pos++;
        AddEvent(Event_PitchWheel);
        break;

      case 0xf0:
        if ((data & 0x0f) == 0xf) // meta event
        {
            data = *curTrack->pos++;
            if (data == 0x2f) // end of track
            {
                curTrack->DoEvent = EndOfMidiTrack;
                return;
            }
            else if (data == 0x51 && *curTrack->pos == 3)
            {
                curTrack->event.data[0] = curTrack->pos[1];
                curTrack->event.data[1] = curTrack->pos[2];
                curTrack->event.data[2] = curTrack->pos[3];
                curTrack->DoEvent = SetTempo;
            }
        }

        length = GetLength();
        curTrack->pos += length;
        return; // don't want to affect running events
    }

    if (event & 0x80)
    {
        curTrack->running = event;
    }
}

static void TrackMusEvents()
{
    int ticks;

    if (curTrack->done)
    {
        return;
    }

    if (curTrack->clock > musicClock)
    {
        return;
    }

    while (GetMusEvent(&ticks) == 0)
    {
    }

    curTrack->clock += ticks;
}

static void TrackMidEvents()
{
    int ticks;

    curTrack = &midTrack[0];

    do
    {
        if (curTrack->clock == musicClock)
        {
            do
            {
                eventData = &curTrack->event;

                curTrack->DoEvent();

                if (curTrack->done)
                {
                    break;
                }

                ticks = GetLength();
                curTrack->clock += ticks;
                GetMidEvent();
            }
            while (ticks == 0);
        }

        curTrack++;
    }
    while (curTrack <= endTrack);
}

static void UpdateEvents()
{
#ifdef MP_TIME
    UpdateScoreTime();
#endif
    MusicEvents();
    musicClock++;
}

static void LoadMusTrack(BYTE *data)
{
    midTrack[0].data = data;
    curTrack = &midTrack[0];
    eventData = &curTrack->event;

    numTracks = 1;

    MusicEvents = TrackMusEvents;
}

static int LoadMidTracks(int count, BYTE *data, int size)
{
    int track, length;

    for (track = 0; track < count; track++)
    {
        if (size < 8)
        {
            return 1;
        }

        if (!ID(data, "MTrk"))
        {
            return 1;
        }

        length = BE32(data + 4);

        data += 8;
        size -= 8;
        if (size < length)
        {
            return 1;
        }

        midTrack[track].data = data;
        data += length;
        size -= length;
    }

    numTracks = count;
    endTrack = &midTrack[numTracks - 1];

    MusicEvents = TrackMidEvents;

    return 0;
}

// midiplay --------------------------------------------------------------------
void Midiplay_Init(int samplerate)
{
    VOICE   *voice;
    CHANNEL *channel;
    int     index;

    Timer_Set(&timerPhase, 65536, samplerate);
    Timer_Set(&timerSecond, MICROSEC, samplerate);

    unusedVoice.next = &unusedVoice;
    unusedVoice.prev = &unusedVoice;
    voice = &midVoice[0];
    for (index = 0; index < VOICES; index++, voice++)
    {
        voice->index = index;
        voice->next = &unusedVoice;
        voice->prev = unusedVoice.prev;
        unusedVoice.prev = voice;
        voice->prev->next = voice;
    }

    channel = &midChannel[0];
    for (index = 0; index < 16; index++, channel++)
    {
        channel->voice.next = &channel->voice;
        channel->voice.prev = &channel->voice;
    }

#ifndef MP_TIME
    AddEvent = NewEvent;
#endif

    musicInit = 1;
}

int Midiplay_Load(void *data, int size)
{
    BYTE    *byte = (BYTE *)data;

    if (musicInit == 0)
    {
        return 0;
    }

    musicInit = 1;
    musicPlaying = 0;

    if (size < 4)
    {
        return 0;
    }

    if (ID(byte, "MUS" "\x1a"))
    {
        if (size < MUS_HDRSIZE)
        {
            return 0;
        }

        if (size < LE16(byte + 6) + LE16(byte + 4))
        {
            return 0;
        }

        beatTicks = LE16(byte + 14);
        if (beatTicks == 0)
        {
            beatTicks = 70;
        }

        LoadMusTrack(byte + LE16(byte + 6));
    }
    else if (ID(byte, "MThd"))
    {
        if (size < MID_HDRSIZE)
        {
            return 0;
        }

        beatTicks = BE16(byte + 12);
        if (beatTicks < 0)
        {
            return 0; // no support for SMPTE format
        }

        size -= MID_HDRSIZE;
        if (LoadMidTracks(BE16(byte + 10), byte + MID_HDRSIZE, size) == 1)
        {
            return 0; // track header failed
        }
    }
    else
    {
        return 0;
    }

    InitTracks();
    musicLooping = 0;
#ifdef MP_TIME
    AddEvent = NoEvent;
    while (numTracksEnded < numTracks)
    {
        UpdateEvents();
    }
    AddEvent = NewEvent;
#endif
    musicInit = 2;

    return 1;
}

void Midiplay_Play(int playing)
{
    // if there's nothing to play, don't force it
    if (musicInit < 2)
    {
        return;
    }

    if (numTracksEnded == numTracks)
    {
        InitTracks();
    }

    musicPlaying = playing;
}

void Midiplay_SetVolume(int volume)
{
    if (volume < 0)
    {
        volume = 0;
    }
    else if (volume > 127)
    {
        volume = 127;
    }

    musicVolume = volumeTable[volume];
}

int Midiplay_IsPlaying()
{
    if (musicInit < 2)
    {
        return 0;
    }

    if (numTracksEnded < numTracks)
    {
        return 1;
    }

    return 0;
}

void Midiplay_Output(short *output, int length)
{
    short   buffer[2];

    while (length)
    {
        if (musicPlaying)
        {
            playSamples -= Timer_Update(&timerSecond);
            if (playSamples < 0)
            {
                UpdateEvents();
                playSamples += Timer_Update(&timerBeat);
            }

            Synth_Generate(buffer, Timer_Update(&timerPhase));
            output[0] = buffer[0] * musicVolume >> 8;
            output[1] = buffer[1] * musicVolume >> 8;
        }
        else
        {
            output[0] = 0;
            output[1] = 0;
        }

        output += 2;
        length -= 2;
    }
}
#ifdef MP_TIME
int Midiplay_Time()
{
    if (musicInit < 2)
    {
        return 0;
    }

    return timeTicks * 10 / beatTicks;
}
#endif
void Midiplay_Loop(int looping)
{
    musicLooping = looping;
}

void Midiplay_Restart()
{
    InitTracks();
}

// midiplay