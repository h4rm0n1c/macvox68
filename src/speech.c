#include "speech.h"
#include "vox_parser.h"
#include <string.h>
#include <ctype.h>

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
    err = speech_stop();
    if (err != noErr || length == 0)
        return err;
    if (!gChannel)
    {
        err = NewSpeechChannel(NULL, &gChannel);
        if (err != noErr)
            return gLastError = err;
    }
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
    formatted = vox_format_text(input, true);
    DisposePtr(input);
    if (!formatted) return gLastError = memFullErr;
    HLock(formatted);
    err = speech_submit(*formatted, (long)strlen(*formatted), true);
    HUnlock(formatted);
    DisposeHandle(formatted);
    return gLastError = err;
}
