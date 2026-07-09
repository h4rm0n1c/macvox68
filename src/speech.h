#ifndef SPEECH_H
#define SPEECH_H

#include <Types.h>

void speech_init(void);
Boolean speech_available(void);
Boolean speech_is_busy(void);
Boolean speech_speak_test_phrase(void);
void speech_stop(void);

#endif /* SPEECH_H */
