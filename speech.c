#include "speech.h"

#include <Gestalt.h>
#include <Memory.h>
#include <Speech.h>
#include <string.h>

#ifndef gestaltSpeechAttr
#define gestaltSpeechAttr 'ttsc'
#endif
#ifndef gestaltSpeechMgrPresent
#define gestaltSpeechMgrPresent 0
#endif

static Boolean gSpeechPresent = false;

void speech_init(void)
{
    long response = 0;
    OSErr err = Gestalt(gestaltSpeechAttr, &response);

    if (err == noErr && (response & (1 << gestaltSpeechMgrPresent)))
        gSpeechPresent = true;
    else
        gSpeechPresent = false;
}

Boolean speech_available(void)
{
    return gSpeechPresent;
}

Boolean speech_is_busy(void)
{
    if (!gSpeechPresent)
        return false;

    return SpeechBusy() != 0;
}

static Boolean speech_speak_pascal(const char *text)
{
    Str255 pascal;
    size_t len;

    if (!gSpeechPresent || !text)
        return false;

    len = strlen(text);
    if (len > 255)
        len = 255;

    pascal[0] = (unsigned char)len;
    BlockMoveData(text, &pascal[1], len);

    return SpeakString(pascal) == noErr;
}

Boolean speech_speak_test_phrase(void)
{
    return speech_speak_pascal("I've got balls of steel");
}

void speech_stop(void)
{
    if (!gSpeechPresent)
        return;

    (void)StopSpeech(NULL);
}
