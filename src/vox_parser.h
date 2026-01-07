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
 * The VOX prosody pipeline (sentence split, “thee” heuristic, lead-in
 * breaks, time/number expansion, beat construction, letter normalization,
 * tidy/wrap) runs first, then FlexTalk markers are translated into
 * Speech Manager embedded commands. FlexTalk escape emission is no longer
 * exposed; only Speech Manager metadata is returned.
 */
Handle vox_format_text(const char *text, Boolean wrap_with_version);

#ifdef VOX_PARSER_STANDALONE
/* Convenience for host-side checks: returns a heap string that must be freed. */
char *vox_process_buffer(const char *text, Boolean wrap_vox_tags);
#endif

#ifdef __cplusplus
}
#endif

#endif /* VOX_PARSER_H */
