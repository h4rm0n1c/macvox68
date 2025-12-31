#include "speech.h"

#include <Memory.h>
#include <Speech.h>

static SpeechChannel gChannel = NULL;
static Handle        gActiveText = NULL;
static Boolean       gSpeechAvailable = false;

static void speech_dispose_active_text(void)
{
    if (gActiveText)
    {
        HUnlock(gActiveText);
        DisposeHandle(gActiveText);
        gActiveText = NULL;
    }
}

Boolean speech_init(void)
{
    OSErr err;

    err = InitSpeech();
    if (err != noErr)
        return false;

    err = NewSpeechChannel(NULL, &gChannel);
    if (err != noErr)
    {
        gChannel = NULL;
        return false;
    }

    gSpeechAvailable = true;
    return true;
}

void speech_shutdown(void)
{
    speech_stop();
    speech_dispose_active_text();

    if (gChannel)
    {
        DisposeSpeechChannel(gChannel);
        gChannel = NULL;
    }

    gSpeechAvailable = false;
}

void speech_pump(void)
{
    if (!gSpeechAvailable)
        return;

    SpeechBusySystemTask();

    if (!speech_is_busy())
        speech_dispose_active_text();
}

Boolean speech_is_available(void)
{
    return gSpeechAvailable;
}

Boolean speech_is_busy(void)
{
    if (!gSpeechAvailable || !gChannel)
        return false;

    return SpeechBusy();
}

Boolean speech_speak_text(const char *text, Size length)
{
    OSErr err;

    if (!gSpeechAvailable || !gChannel || !text || length <= 0)
        return false;

    speech_stop();
    speech_dispose_active_text();

    gActiveText = NewHandle(length);
    if (!gActiveText)
        return false;

    HLock(gActiveText);
    BlockMove(text, *gActiveText, length);

    err = SpeakText(gChannel, *gActiveText, length);
    if (err != noErr)
    {
        speech_dispose_active_text();
        return false;
    }

    return true;
}

void speech_stop(void)
{
    if (gChannel)
        StopSpeechChannel(gChannel);

    speech_dispose_active_text();
}
