#ifndef VOX_PARSER_H
#define VOX_PARSER_H

#ifdef VOX_PARSER_STANDALONE
#include <stddef.h>
typedef unsigned char **Handle;
typedef int Boolean;
#ifndef true
#define true 1
#define false 0
#endif
#else
#include <Types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Format a UTF-8/ASCII text buffer into a Speech Manager-ready Handle.
 * The current implementation focuses on translating FlexTalk-style
 * breaks and envelope markers into embedded commands while keeping
 * the original text intact for the classic Mac OS Speech Manager.
 */
Handle vox_format_text(const char *text, Boolean wrap_with_version);

/*
 * Full pipeline with selectable output style. When `speech_manager_mode`
 * is false, the returned handle contains the raw FlexTalk-compatible
 * markers (for parity checking against the upstream VOX parser). When true,
 * the handle contains embedded Speech Manager commands.
 */
Handle vox_format_text_mode(const char *text,
                            Boolean wrap_with_version,
                            Boolean speech_manager_mode);

#ifdef VOX_PARSER_STANDALONE
/* Convenience for host-side checks: returns a heap string that must be freed. */
char *vox_process_buffer(const char *text, Boolean wrap_vox_tags);
#endif

#ifdef __cplusplus
}
#endif

#endif /* VOX_PARSER_H */
