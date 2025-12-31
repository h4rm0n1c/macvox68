#ifndef SPEECH_H
#define SPEECH_H

#include <Types.h>

#ifdef __cplusplus
extern "C" {
#endif

Boolean speech_init(void);
void    speech_shutdown(void);
void    speech_pump(void);

Boolean speech_is_available(void);
Boolean speech_is_busy(void);

Boolean speech_speak_text(const char *text, Size length);
void    speech_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* SPEECH_H */
