#include "vox_parser.h"

#include <Memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Boolean append_text(char **buffer, size_t *cap, size_t *len, const char *text)
{
    size_t need;
    if (!buffer || !cap || !len || !text)
        return false;

    need = strlen(text);
    if (*len + need + 1 > *cap) {
        size_t new_cap = (*cap == 0) ? 256 : *cap;
        while (new_cap < *len + need + 1)
            new_cap *= 2;

        char *grown = (char *)realloc(*buffer, new_cap);
        if (!grown)
            return false;
        *buffer = grown;
        *cap = new_cap;
    }

    BlockMoveData(text, *buffer + *len, need);
    *len += need;
    (*buffer)[*len] = '\0';
    return true;
}

static Boolean append_char(char **buffer, size_t *cap, size_t *len, char c)
{
    char tmp[2];
    tmp[0] = c;
    tmp[1] = '\0';
    return append_text(buffer, cap, len, tmp);
}

static Boolean emit_break(char **buffer, size_t *cap, size_t *len)
{
    /* Speech Manager expects milliseconds; FlexTalk breaks are usually ~250ms */
    return append_text(buffer, cap, len, "[[slnc 250]] ");
}

static Boolean emit_version_prefix(char **buffer, size_t *cap, size_t *len)
{
    return append_text(buffer, cap, len, "[[vers 1]] ");
}

static Boolean emit_comment(char **buffer, size_t *cap, size_t *len, const char *tag)
{
    if (!tag)
        return true;
    return append_text(buffer, cap, len, "[[cmnt ") &&
           append_text(buffer, cap, len, tag) &&
           append_text(buffer, cap, len, "]] ");
}

static Boolean translate_token(const char *tok, size_t tok_len,
                               char **buffer, size_t *cap, size_t *len)
{
    if (tok_len == 0)
        return true;

    /* Recognize a handful of FlexTalk escape patterns used by the VOX parser */
    if (tok_len == 4 && strncmp(tok, "\\!br", 4) == 0)
        return emit_break(buffer, cap, len);

    if (tok_len >= 4 && strncmp(tok, "\\!sf", 4) == 0) {
        /* Silent frame: interpret the following digits as centiseconds. */
        long millis = 0;
        const char *p = tok + 4;
        while (*p >= '0' && *p <= '9') {
            millis = (millis * 10) + (*p - '0');
            ++p;
        }
        millis *= 10; /* centiseconds to ms */

        char tmp[32];
        (void)snprintf(tmp, sizeof(tmp), "[[slnc %ld]] ", millis);
        return append_text(buffer, cap, len, tmp);
    }

    if (tok_len == 4 && strncmp(tok, "\\!wH1", 4) == 0)
        return emit_comment(buffer, cap, len, "vox-start");
    if (tok_len == 4 && strncmp(tok, "\\!wH0", 4) == 0)
        return emit_comment(buffer, cap, len, "vox-end");

    /* Default: pass text through untouched */
    return append_text(buffer, cap, len, tok);
}

Handle vox_format_text(const char *text, Boolean wrap_with_version)
{
    Handle h = NULL;
    char *buffer = NULL;
    size_t cap = 0;
    size_t len = 0;

    if (!text)
        return NULL;

    if (wrap_with_version) {
        if (!emit_version_prefix(&buffer, &cap, &len))
            goto done;
    }

    /* Walk the input once, turning FlexTalk escapes into Speech Manager markup */
    {
        const char *p = text;
        while (*p) {
            if (p[0] == '\\' && p[1] == '!') {
                /* capture escape token */
                const char *start = p;
                p += 2;
                while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
                    ++p;
                if (!translate_token(start, (size_t)(p - start), &buffer, &cap, &len))
                    goto done;
                continue;
            }

            if (!append_char(&buffer, &cap, &len, *p))
                goto done;
            ++p;
        }
    }

    if (!emit_break(&buffer, &cap, &len))
        goto done;

    h = NewHandle(len + 1);
    if (h && *h) {
        BlockMoveData(buffer, *h, len + 1);
    }

done:
    free(buffer);
    return h;
}
