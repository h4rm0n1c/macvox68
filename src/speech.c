#include "speech.h"
#include "vox_parser.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <Gestalt.h>
#include <Memory.h>
#include <Speech.h>

#ifndef gestaltSpeechAttr
#define gestaltSpeechAttr 'ttsc'
#endif
#ifndef gestaltSpeechMgrPresent
#define gestaltSpeechMgrPresent 0
#endif

static Boolean gSpeechPresent = false;
static SpeechChannel gChannel = NULL;
static Ptr gText = NULL;
static OSErr gLastError = noErr;
static short gVolume = 100, gRate = 100, gPitch = 0;
static Fixed gDefaultRate, gDefaultPitch;

OSErr speech_set_settings(short volume, short rate, short pitch)
{
    if (volume < 0 || volume > 100 || rate < 50 || rate > 200 || pitch < -12 || pitch > 12)
        return gLastError = paramErr;
    gVolume = volume; gRate = rate; gPitch = pitch;
    return noErr;
}

static Fixed speech_scale(Fixed value, short percent)
{
    /* Avoid overflowing 32-bit Fixed when multiplying a normal WPM value. */
    return (value / 100) * percent + ((value % 100) * percent) / 100;
}


static void speech_release_text(void)
{
    if (gText)
        DisposePtr(gText);
    gText = NULL;
}

void speech_init(void)
{
    long response = 0;
    OSErr err = Gestalt(gestaltSpeechAttr, &response);
    gSpeechPresent = err == noErr && (response & (1L << gestaltSpeechMgrPresent));
    gLastError = err;
}

Boolean speech_available(void)
{
    return gSpeechPresent;
}

OSErr speech_last_error(void)
{
    return gLastError;
}

Boolean speech_is_busy(void)
{
    SpeechStatusInfo status;
    OSErr err;
    if (!gChannel || !gText)
        return false;
    err = GetSpeechInfo(gChannel, soStatus, &status);
    if (err != noErr)
    {
        /* An uncertain channel may still read its buffer. Keep it until a
           successful stop/disposal; never free on a failed status query. */
        gLastError = err;
        return true;
    }
    if (!status.outputBusy)
        speech_release_text();
    return status.outputBusy;
}

OSErr speech_stop(void)
{
    OSErr err = noErr;
    if (gChannel)
        err = StopSpeech(gChannel);
    if (err == noErr)
        speech_release_text();
    return gLastError = err;
}

OSErr speech_shutdown(void)
{
    OSErr err = noErr;
    /* Disposal stops output synchronously (Inside Macintosh, corpus 23660).
       Keep the text alive until the synthesizer can no longer read it. */
    if (gChannel)
        err = DisposeSpeechChannel(gChannel);
    if (err == noErr)
    {
        gChannel = NULL;
        speech_release_text();
    }
    return gLastError = err;
}

static OSErr speech_prepare_channel(void)
{
    OSErr err;
    OSType mode = modeText;
    if (!gSpeechPresent) return gLastError = unimpErr;
    err = speech_stop();
    if (err != noErr) return err;
    if (!gChannel)
    {
        err = NewSpeechChannel(NULL, &gChannel);
        if (err != noErr)
            return gLastError = err;
        err = GetSpeechRate(gChannel, &gDefaultRate);
        if (err == noErr) err = GetSpeechPitch(gChannel, &gDefaultPitch);
        if (err != noErr)
        {
            (void)speech_shutdown();
            return gLastError = err;
        }
    }
    {
        Fixed volume = speech_scale(0x10000L, gVolume);
        err = SetSpeechInfo(gChannel, soVolume, &volume);
        if (err == noErr) err = SetSpeechRate(gChannel, speech_scale(gDefaultRate, gRate));
        if (err == noErr) err = SetSpeechPitch(gChannel, gDefaultPitch + (long)gPitch * 65536L);
        if (err != noErr)
        {
            (void)speech_shutdown();
            return gLastError = err;
        }
    }
    /* A cancelled phoneme utterance may never reach its trailing TEXT command.
       Set the mode directly before every new conversion/utterance (corpus 23628). */
    err = SetSpeechInfo(gChannel, soInputMode, &mode);
    return gLastError = err;
}

static OSErr speech_submit(const void *text, long length, Boolean commands)
{
    OSErr err;
    DelimiterInfo delimiters = {{0, 0}, {0, 0}};
    if (commands)
    {
        delimiters.startDelimiter[0] = delimiters.startDelimiter[1] = '[';
        delimiters.endDelimiter[0] = delimiters.endDelimiter[1] = ']';
    }
    if (length < 0 || (length > 0 && !text))
        return gLastError = paramErr;
    if (!gSpeechPresent)
        return gLastError = unimpErr;
    if (length == 0) return speech_stop();
    err = speech_prepare_channel();
    if (err != noErr) return err;
    /* Only generated HL VOX output enables embedded commands. Clean is literal.
       Zero delimiters disable command processing (corpus 23640).
       Retry configuration even if a previous disposal attempt failed. */
    err = SetSpeechInfo(gChannel, soCommandDelimiter, &delimiters);
    if (err != noErr)
    {
        (void)speech_shutdown();
        return gLastError = err;
    }
    /* NewPtr is nonrelocatable. SpeakText reads asynchronously, including at
       interrupt time; retain this copy until idle, successful stop or disposal.
       No interrupt callbacks or borrowed TextEdit handles (corpus 23645). */
    gText = NewPtr(length);
    if (!gText)
        return gLastError = memFullErr;
    BlockMoveData(text, gText, length);
    err = SpeakText(gChannel, gText, length);
    if (err != noErr)
    {
        /* Conservatively stop before releasing even if a synthesizer accepted
           some input before reporting an error. */
        (void)speech_stop();
    }
    return gLastError = err;
}

OSErr speech_speak_text(const void *text, long length)
{
    return speech_submit(text, length, false);
}

/* TextToPhonemes runs with command delimiters disabled, using this channel's
   voice and current settings. Return malloc ownership to the renderer. */
static char *speech_convert_beat(const char *beat, void *context)
{
    OSErr *error = (OSErr *)context;
    Handle phonemes = NewHandle(16384);
    char *copy = NULL;
    long count = 0;
    if (!phonemes) { *error = memFullErr; return NULL; }
    *error = TextToPhonemes(gChannel, beat, (long)strlen(beat), phonemes, &count);
    if (*error == noErr) {
        if (count <= 0 || count > 16384 || count > GetHandleSize(phonemes))
            *error = paramErr;
        else {
            copy = (char *)malloc((size_t)count + 1);
            if (!copy) *error = memFullErr;
            else {
                HLock(phonemes);
                BlockMoveData(*phonemes, copy, count);
                HUnlock(phonemes);
                copy[count] = 0;
                if (memchr(copy, 0, (size_t)count)) {
                    free(copy); copy = NULL; *error = paramErr;
                }
            }
        }
    }
    DisposeHandle(phonemes);
    return copy;
}

OSErr speech_speak_vox(const void *text, long length)
{
    Ptr input;
    Handle formatted;
    OSErr err;
    long i;
    Boolean nonblank = false;
    if (length < 0 || length > 32767 || (length && !text))
        return gLastError = paramErr;
    if (!gSpeechPresent)
        return gLastError = unimpErr;
    for (i = 0; i < length; ++i)
    {
        unsigned char c = ((const unsigned char *)text)[i];
        if (!c) return gLastError = paramErr;
        if (!isspace(c)) nonblank = true;
    }
    if (!nonblank)
        return speech_stop();
    input = NewPtr(length + 1);
    if (!input) return gLastError = memFullErr;
    BlockMoveData(text, input, length);
    input[length] = 0;
    err = speech_prepare_channel();
    if (err == noErr) {
        SpeechVersionInfo version = {0};
        /* Only the measured MacInTalk 3 provider uses this native alphabet.
           Other synthesizers retain the existing text adapter. */
        if (GetSpeechInfo(gChannel, soSynthType, &version) == noErr &&
            version.synthManufacturer == 'mtk3') {
            DelimiterInfo disabled = {{0, 0}, {0, 0}};
            err = SetSpeechInfo(gChannel, soCommandDelimiter, &disabled);
            formatted = err == noErr ? vox_format_native_text(input, speech_convert_beat, &err) : NULL;
        } else formatted = vox_format_text(input, true);
    } else formatted = NULL;
    DisposePtr(input);
    if (err != noErr) {
        if (formatted) DisposeHandle(formatted);
        return gLastError = err;
    }
    if (!formatted) return gLastError = memFullErr;
    HLock(formatted);
    err = speech_submit(*formatted, (long)strlen(*formatted), true);
    HUnlock(formatted);
    DisposeHandle(formatted);
    return gLastError = err;
}
