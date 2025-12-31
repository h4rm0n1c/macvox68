#ifndef VOX_PARSER_H
#define VOX_PARSER_H

#include <Types.h>

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

#ifdef __cplusplus
}
#endif

#endif /* VOX_PARSER_H */
