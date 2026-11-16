#ifndef SPEECH_H
#define SPEECH_H
#include <Types.h>
void speech_init(void);
/* Applied at next utterance: amplitude 0..100%, rate 50..200% of voice default, pitch -12..12 semitones. */
OSErr speech_set_settings(short volume, short rate, short pitch);
Boolean speech_available(void);
/* Polls only our channel; reclaims completed input on the application thread. */
Boolean speech_is_busy(void);
/* Copies length MacRoman bytes; caller must keep source stable during the call. */
OSErr speech_speak_text(const void *text, long length);
/* Formats bounded MacRoman editor text as HL VOX, then copies for async speech. */
OSErr speech_speak_vox(const void *text, long length);
OSErr speech_stop(void);
OSErr speech_shutdown(void);
OSErr speech_last_error(void);
#endif
