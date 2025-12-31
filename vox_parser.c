#define _POSIX_C_SOURCE 200809L

#include "vox_parser.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifdef VOX_PARSER_STANDALONE
#include <stdint.h>
#define BlockMoveData(src, dst, len) memmove((dst), (src), (len))
static Handle NewHandle(size_t size)
{
    unsigned char **h = (unsigned char **)malloc(sizeof(unsigned char *));
    if (!h)
        return NULL;
    *h = (unsigned char *)calloc(1, size);
    if (!*h) {
        free(h);
        return NULL;
    }
    return (Handle)h;
}
#else
#include <Memory.h>
#endif

#ifndef Boolean
#define Boolean int
#endif

#define ARRAY_GROWTH 8

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} StringBuilder;

static void sb_init(StringBuilder *sb)
{
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

static Boolean sb_reserve(StringBuilder *sb, size_t add)
{
    if (sb->len + add + 1 <= sb->cap)
        return true;
    size_t new_cap = sb->cap ? sb->cap : 128;
    while (new_cap < sb->len + add + 1)
        new_cap *= 2;
    char *p = (char *)realloc(sb->data, new_cap);
    if (!p)
        return false;
    sb->data = p;
    sb->cap = new_cap;
    return true;
}

static Boolean sb_append_str(StringBuilder *sb, const char *text)
{
    if (!text)
        return true;
    size_t n = strlen(text);
    if (!sb_reserve(sb, n))
        return false;
    BlockMoveData(text, sb->data + sb->len, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
    return true;
}

static Boolean sb_append_char(StringBuilder *sb, char c)
{
    if (!sb_reserve(sb, 1))
        return false;
    sb->data[sb->len++] = c;
    sb->data[sb->len] = '\0';
    return true;
}

static char *sb_steal(StringBuilder *sb)
{
    char *out = sb->data;
    sb->data = NULL;
    sb->len = sb->cap = 0;
    return out;
}

static char *dup_slice(const char *start, size_t len)
{
    char *out = (char *)malloc(len + 1);
    if (!out)
        return NULL;
    BlockMoveData(start, out, len);
    out[len] = '\0';
    return out;
}

static char *str_trim(const char *s)
{
    if (!s)
        return NULL;
    const char *a = s;
    while (*a && isspace((unsigned char)*a))
        ++a;
    const char *b = s + strlen(s);
    while (b > a && isspace((unsigned char)b[-1]))
        --b;
    return dup_slice(a, (size_t)(b - a));
}

static void str_to_lower(char *s)
{
    if (!s)
        return;
    for (; *s; ++s)
        *s = (char)tolower((unsigned char)*s);
}

static Boolean is_all_digits(const char *t)
{
    if (!t || !*t)
        return false;
    for (; *t; ++t) {
        if (!isdigit((unsigned char)*t))
            return false;
    }
    return true;
}

static Boolean is_titlecase_word(const char *t)
{
    if (!t || !*t)
        return false;
    if (!isalpha((unsigned char)t[0]) || !isupper((unsigned char)t[0]))
        return false;
    return true;
}

static char *strip_trailing_punct(const char *t)
{
    size_t n = strlen(t);
    while (n > 0) {
        char c = t[n - 1];
        if (c == ',' || c == '.' || c == ';' || c == ':' || c == '!' || c == '?' || c == '"' || c == '\'')
            --n;
        else
            break;
    }
    return dup_slice(t, n);
}

static Boolean ends_with_br(const char *s)
{
    size_t n = strlen(s);
    if (n < 4)
        return false;
    return strstr(s + (n > 5 ? n - 5 : 0), "\\!br") != NULL && strcmp(s + n - 4, "\\!br") == 0;
}

typedef struct {
    char **items;
    size_t count;
    size_t cap;
} StringVec;

static void vec_init(StringVec *v)
{
    v->items = NULL;
    v->count = 0;
    v->cap = 0;
}

static Boolean vec_push(StringVec *v, char *item)
{
    if (v->count + 1 > v->cap) {
        size_t new_cap = v->cap ? v->cap * 2 : ARRAY_GROWTH;
        char **n = (char **)realloc(v->items, new_cap * sizeof(char *));
        if (!n)
            return false;
        v->items = n;
        v->cap = new_cap;
    }
    v->items[v->count++] = item;
    return true;
}

static void vec_free(StringVec *v)
{
    if (!v)
        return;
    for (size_t i = 0; i < v->count; ++i)
        free(v->items[i]);
    free(v->items);
    v->items = NULL;
    v->count = v->cap = 0;
}

static StringVec tokenize_ws(const char *s)
{
    StringVec out;
    vec_init(&out);
    if (!s)
        return out;

    const char *p = s;
    while (*p) {
        while (*p && isspace((unsigned char)*p))
            ++p;
        if (!*p)
            break;
        const char *start = p;
        while (*p && !isspace((unsigned char)*p))
            ++p;
        (void)vec_push(&out, dup_slice(start, (size_t)(p - start)));
    }
    return out;
}

static StringVec split_sentences(const char *in)
{
    StringVec out;
    vec_init(&out);
    if (!in)
        return out;

    StringBuilder cur;
    sb_init(&cur);
    const char *p = in;
    while (*p) {
        char c = *p;
        sb_append_char(&cur, c);
        if (c == '.' || c == '!' || c == '?') {
            const char *q = p + 1;
            while (*q == '"' || *q == '\'') {
                sb_append_char(&cur, *q);
                ++q;
            }
            char *trimmed = str_trim(cur.data);
            if (trimmed && *trimmed)
                vec_push(&out, trimmed);
            else
                free(trimmed);
            cur.len = 0;
            if (cur.data)
                cur.data[0] = '\0';
            p = q - 1;
        }
        ++p;
    }
    char *tail = str_trim(cur.data ? cur.data : "");
    if (tail && *tail)
        vec_push(&out, tail);
    else
        free(tail);
    free(cur.data);
    return out;
}

static Boolean prefix_matches(const char *s, const char *prefix)
{
    size_t n = strlen(prefix);
    return strncasecmp(s, prefix, n) == 0;
}

static char *apply_leadin_break(char *s)
{
    static const char *phrases[] = {
        "Now ",
        "Please ",
        "A reminder ",
        "On behalf of ",
        "Good morning",
        "Good evening",
        "This automated train",
        "This automated tram",
        NULL
    };

    if (!s)
        return NULL;
    for (int i = 0; phrases[i]; ++i) {
        const char *p = phrases[i];
        if (prefix_matches(s, p)) {
            size_t n = strlen(p);
            StringBuilder sb;
            sb_init(&sb);
            sb_append_str(&sb, phrases[i]);
            sb_append_str(&sb, " \\!br ");
            sb_append_str(&sb, s + n);
            free(s);
            return sb_steal(&sb);
        }
    }
    return s;
}

static Boolean is_tag(const char *w)
{
    return strcmp(w, "\\!br") == 0 || strcmp(w, "\\!wH1") == 0 || strcmp(w, "\\!wH0") == 0;
}

static char *strip_tail_core(const char *x, char **tail_out)
{
    size_t n = strlen(x);
    size_t end = n;
    while (end > 0) {
        char c = x[end - 1];
        if (c == ',' || c == '.' || c == ';' || c == ':' || c == '!' || c == '?' || c == '"' || c == '\'')
            --end;
        else
            break;
    }
    char *core = dup_slice(x, end);
    if (tail_out)
        *tail_out = dup_slice(x + end, n - end);
    return core;
}

static Boolean is_single_letter(const char *w)
{
    char *core = strip_tail_core(w, NULL);
    Boolean res = false;
    if (core) {
        size_t n = strlen(core);
        if (n == 1 && isalpha((unsigned char)core[0]) && isupper((unsigned char)core[0]))
            res = true;
        else if (n == 2 && isalpha((unsigned char)core[0]) && isupper((unsigned char)core[0]) && core[1] == ':')
            res = true;
    }
    free(core);
    return res;
}

static Boolean is_number_token(const char *w)
{
    char *core = strip_tail_core(w, NULL);
    Boolean res = core && is_all_digits(core);
    free(core);
    return res;
}

static char *apply_thee_rule(char *s)
{
    StringVec tok = tokenize_ws(s);
    if (tok.count == 0) {
        vec_free(&tok);
        return s;
    }

    StringBuilder out;
    sb_init(&out);

    for (size_t i = 0; i < tok.count; ++i) {
        const char *word = tok.items[i];
        char *core_tail = NULL;
        char *core = strip_tail_core(word, &core_tail);
        char *lower = core ? strdup(core) : NULL;
        if (lower)
            str_to_lower(lower);

        Boolean make_thee = false;
        if (core && lower && strcmp(lower, "the") == 0) {
            /* backward */
            for (int back = (int)i - 1; back >= 0; --back) {
                if (is_tag(tok.items[back]))
                    continue;
                char *prev_lower = strdup(tok.items[back]);
                if (prev_lower) {
                    char *tmp = strip_trailing_punct(prev_lower);
                    free(prev_lower);
                    prev_lower = tmp;
                }
                if (prev_lower) {
                    str_to_lower(prev_lower);
                    if (strcmp(prev_lower, "to") == 0 || strcmp(prev_lower, "for") == 0 ||
                        strcmp(prev_lower, "of") == 0 || strcmp(prev_lower, "in") == 0 ||
                        strcmp(prev_lower, "on") == 0 || strcmp(prev_lower, "at") == 0) {
                        make_thee = true;
                    }
                    if (!make_thee && strcmp(prev_lower, "of") == 0) {
                        char *p2 = NULL;
                        char *p3 = NULL;
                        if (back - 1 >= 0) p2 = strdup(tok.items[back - 1]);
                        if (back - 2 >= 0) p3 = strdup(tok.items[back - 2]);
                        if (p2 && p3) {
                            str_to_lower(p2);
                            str_to_lower(p3);
                            if (strcmp(p2, "behalf") == 0 && strcmp(p3, "on") == 0)
                                make_thee = true;
                        }
                        free(p2);
                        free(p3);
                    }
                }
                free(prev_lower);
                break;
            }
            /* forward */
            if (!make_thee) {
                for (size_t f = i + 1; f < tok.count; ++f) {
                    if (is_tag(tok.items[f]))
                        continue;
                    if (is_titlecase_word(tok.items[f]) || is_single_letter(tok.items[f]) || is_number_token(tok.items[f]))
                        make_thee = true;
                    break;
                }
            }
        }

        if (out.len > 0)
            sb_append_char(&out, ' ');
        if (core) {
            sb_append_str(&out, make_thee ? "thee" : core);
            if (core_tail)
                sb_append_str(&out, core_tail);
        } else {
            sb_append_str(&out, word);
        }

        free(core_tail);
        free(core);
        free(lower);
    }

    vec_free(&tok);
    free(s);
    return sb_steal(&out);
}

static Boolean meridiem_token(const char *p, size_t *consumed, Boolean *is_pm)
{
    if (strncasecmp(p, "AM", 2) == 0) {
        *consumed = 2;
        *is_pm = false;
        return true;
    }
    if (strncasecmp(p, "A.M.", 4) == 0) {
        *consumed = 4;
        *is_pm = false;
        return true;
    }
    if (strncasecmp(p, "PM", 2) == 0) {
        *consumed = 2;
        *is_pm = true;
        return true;
    }
    if (strncasecmp(p, "P.M.", 4) == 0) {
        *consumed = 4;
        *is_pm = true;
        return true;
    }
    return false;
}

static char *apply_time_numbers_degrees(char *s)
{
    if (!s)
        return NULL;

    StringBuilder sb;
    sb_init(&sb);

    const char *p = s;
    while (*p) {
        /* 12h time like 8:47 AM */
        if (isdigit((unsigned char)*p)) {
            size_t hh_digits = 0;
            int hh = 0;
            while (hh_digits < 2 && isdigit((unsigned char)p[hh_digits])) {
                hh = (hh * 10) + (p[hh_digits] - '0');
                ++hh_digits;
            }
            if (hh_digits > 0 && (p[hh_digits] == ':' || p[hh_digits] == '.') &&
                isdigit((unsigned char)p[hh_digits + 1]) && isdigit((unsigned char)p[hh_digits + 2])) {
                int mm = (p[hh_digits + 1] - '0') * 10 + (p[hh_digits + 2] - '0');
                const char *after = p + hh_digits + 3;
                while (*after == ' ')
                    ++after;
                size_t mdig = 0;
                Boolean is_pm = false;
                if (meridiem_token(after, &mdig, &is_pm)) {
                    const char *ap1 = is_pm ? "P:" : "Ay:";
                    char tmp[64];
                    snprintf(tmp, sizeof(tmp), "%d \\!br %d \\!br %s \\!br M:", hh, mm, ap1);
                    sb_append_str(&sb, tmp);
                    p = after + mdig;
                    while (*p == '.')
                        ++p;
                    continue;
                }
            }
        }

        if (p[0] == 'T' || p[0] == 't') {
            if (strncasecmp(p, "The time is ", 12) == 0) {
                sb_append_str(&sb, "The time is \\!br ");
                p += 12;
                continue;
            }
        }

        if (isdigit((unsigned char)p[0]) && isdigit((unsigned char)p[1]) &&
            isdigit((unsigned char)p[2]) && p[3] == '0' && p[4] == '0' &&
            strncasecmp(p + 5, " hours", 6) == 0) {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "%c%c \\!br hundred \\!br hours", p[0], p[1]);
            sb_append_str(&sb, tmp);
            p += 11; /* 5 digits + space + hours */
            continue;
        }

        if (isdigit((unsigned char)p[0]) && isdigit((unsigned char)p[1]) && isdigit((unsigned char)p[2])) {
            if (!(p[3] && isdigit((unsigned char)p[3]))) {
                int b = p[1] - '0';
                int c = p[2] - '0';
                if (!(b == 0 && c == 0)) {
                    sb_append_char(&sb, (char)p[0]);
                    sb_append_char(&sb, '0');
                    sb_append_char(&sb, '0');
                    sb_append_str(&sb, " \\!br and \\!br ");
                    if (b == 0)
                        sb_append_char(&sb, (char)('0' + c));
                    else {
                        sb_append_char(&sb, (char)('0' + b));
                        sb_append_char(&sb, (char)('0' + c));
                    }
                    p += 3;
                    continue;
                }
            }
        }

        if (isdigit((unsigned char)*p)) {
            const char *deg = p;
            while (isdigit((unsigned char)*deg))
                ++deg;
            const char *after_deg = deg;
            while (*after_deg == ' ')
                ++after_deg;
            if (strncasecmp(after_deg, "degrees", 7) == 0) {
                char *num = dup_slice(p, (size_t)(deg - p));
                if (num) {
                    sb_append_str(&sb, num);
                    free(num);
                }
                sb_append_str(&sb, " \\!br degrees \\!br");
                p = after_deg + 7;
                continue;
            }
        }

        if (p[0] == 'M' && p[1] == ':' && p[2] == '.') {
            sb_append_str(&sb, "M:");
            p += 3;
            continue;
        }

        sb_append_char(&sb, *p);
        ++p;
    }

    free(s);
    return sb_steal(&sb);
}

typedef enum { WT_LIGHT, WT_MEDIUM, WT_HEAVY } Weight;

static Boolean is_stop(const char *tl)
{
    static const char *words[] = {
        "the", "thee", "a", "an", "to", "for", "of", "in", "on", "at", "by", "with", "from",
        "as", "is", "are", "was", "were", "this", "that", NULL};
    for (int i = 0; words[i]; ++i) {
        if (strcmp(tl, words[i]) == 0)
            return true;
    }
    return false;
}

static Boolean is_lightverb(const char *tl)
{
    static const char *words[] = {
        "welcome", "wait", "stand", "provide", "provided", "return", "remain", "maintain", "maintained",
        "verify", "contact", "board", "arriving", "inbound", "bound", "commence", "commences", NULL};
    for (int i = 0; words[i]; ++i) {
        if (strcmp(tl, words[i]) == 0)
            return true;
    }
    return false;
}

static Boolean is_unit(const char *tl)
{
    static const char *words[] = {"degrees", "hours", "percent", "%", NULL};
    for (int i = 0; words[i]; ++i) {
        if (strcmp(tl, words[i]) == 0)
            return true;
    }
    return false;
}

static int syllables(const char *w)
{
    if (!w || !*w)
        return 0;
    size_t n = strlen(w);
    char *s = dup_slice(w, n);
    if (!s)
        return 0;
    str_to_lower(s);
    int count = 0;
    Boolean in = false;
    for (size_t i = 0; s[i]; ++i) {
        char c = s[i];
        Boolean isv = (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y');
        if (isv) {
            if (!in) {
                ++count;
                in = true;
            }
        } else {
            in = false;
        }
    }
    if (count > 1 && n > 1 && s[n - 1] == 'e' && !(n > 2 && s[n - 2] == 'l'))
        --count;
    if (n > 2 && s[n - 1] == 'e' && s[n - 2] == 'l')
        ++count;
    free(s);
    if (count < 1)
        count = 1;
    return count;
}

static Weight weight_token(const char *tok)
{
    if (!tok || !*tok)
        return WT_LIGHT;
    char *pfree = strip_trailing_punct(tok);
    if (!pfree)
        return WT_LIGHT;
    char *lower = strdup(pfree);
    if (lower)
        str_to_lower(lower);
    if (is_stop(lower) || is_lightverb(lower)) {
        free(pfree);
        free(lower);
        return WT_LIGHT;
    }
    if (is_all_digits(pfree) || strchr(pfree, '-') || is_titlecase_word(pfree) || is_unit(lower)) {
        free(pfree);
        free(lower);
        return WT_HEAVY;
    }
    int syl = syllables(pfree);
    Weight w = (syl >= 3 || strlen(pfree) >= 8) ? WT_HEAVY : WT_MEDIUM;
    free(pfree);
    free(lower);
    return w;
}

static Boolean is_and_tok(const char *t)
{
    char *b = strip_trailing_punct(t);
    if (!b)
        return false;
    str_to_lower(b);
    Boolean res = strcmp(b, "and") == 0;
    free(b);
    return res;
}

static Boolean starts_with_punct(const char *t)
{
    if (!t || !*t)
        return false;
    char c = t[0];
    return (c == '.' || c == ',' || c == ';' || c == ':' || c == '!' || c == '?' || c == ')');
}

static Boolean is_open_punct(const char *t)
{
    if (!t || !*t)
        return false;
    char c = t[0];
    return (c == '(' || c == '"' || c == '\'');
}

static void emit_titlecase_run(const StringVec *v, size_t i, size_t j, StringBuilder *out)
{
    if (out->len > 0 && !ends_with_br(out->data))
        sb_append_str(out, " \\!br");
    for (size_t k = i; k < j; ++k) {
        sb_append_char(out, ' ');
        sb_append_str(out, v->items[k]);
        sb_append_str(out, " \\!br");
    }
}

static char *build_beats(const char *sentence)
{
    StringVec v = tokenize_ws(sentence);
    if (v.count == 0)
        return strdup(sentence);

    StringBuilder out;
    sb_init(&out);
    Boolean since_break_all_light = true;
    Weight prev_w = WT_LIGHT;
    int content_run = 0;

    #define RESET_AFTER_BREAK() \
        do { since_break_all_light = true; content_run = 0; prev_w = WT_LIGHT; } while (0)

    for (size_t i = 0; i < v.count;) {
        char *bare = strip_trailing_punct(v.items[i]);

        /* Proper noun / Area/Level/Sector */
        if (strcmp(v.items[i], "Area") == 0 || strcmp(v.items[i], "Level") == 0 || strcmp(v.items[i], "Sector") == 0) {
            size_t j = i + 1;
            while (j < v.count) {
                char *bj = strip_trailing_punct(v.items[j]);
                Boolean ok = (bj && (is_all_digits(bj) || is_titlecase_word(bj)));
                free(bj);
                if (ok)
                    ++j;
                else
                    break;
            }
            if (j > i + 1) {
                emit_titlecase_run(&v, i, j, &out);
                i = j;
                RESET_AFTER_BREAK();
                free(bare);
                continue;
            }
        }
        if (is_titlecase_word(bare)) {
            size_t j = i;
            while (j < v.count) {
                char *bj = strip_trailing_punct(v.items[j]);
                Boolean ok = is_titlecase_word(bj);
                free(bj);
                if (ok)
                    ++j;
                else
                    break;
            }
            if (j >= i + 2) {
                emit_titlecase_run(&v, i, j, &out);
                i = j;
                RESET_AFTER_BREAK();
                free(bare);
                continue;
            }
        }

        /* and between heavier items */
        if (is_and_tok(v.items[i]) && i > 0 && i + 1 < v.count) {
            Weight lw = weight_token(v.items[i - 1]);
            Weight rw = weight_token(v.items[i + 1]);
            if (lw >= WT_MEDIUM && rw >= WT_MEDIUM) {
                if (!ends_with_br(out.data))
                    sb_append_str(&out, " \\!br");
                sb_append_str(&out, " and \\!br");
                ++i;
                RESET_AFTER_BREAK();
                free(bare);
                continue;
            }
        }

        /* standalone dash */
        if (strcmp(v.items[i], "-") == 0 || strcmp(v.items[i], "–") == 0 || strcmp(v.items[i], "—") == 0) {
            if (!ends_with_br(out.data))
                sb_append_str(&out, " \\!br");
            RESET_AFTER_BREAK();
            ++i;
            free(bare);
            continue;
        }

        /* linker isolation */
        {
            char *lower = strdup(bare ? bare : "");
            if (lower)
                str_to_lower(lower);
            Boolean in_head = (i < 2);
            if (!in_head && (strcmp(lower, "at") == 0 || strcmp(lower, "in") == 0 || strcmp(lower, "on") == 0 ||
                             strcmp(lower, "to") == 0 || strcmp(lower, "of") == 0 || strcmp(lower, "for") == 0 ||
                             strcmp(lower, "from") == 0 || strcmp(lower, "by") == 0)) {
                if (!ends_with_br(out.data))
                    sb_append_str(&out, " \\!br");
                RESET_AFTER_BREAK();
                if (out.len > 0 && !starts_with_punct(v.items[i]) && !is_open_punct(v.items[i]))
                    sb_append_char(&out, ' ');
                sb_append_str(&out, v.items[i]);
                if (!ends_with_br(out.data))
                    sb_append_str(&out, " \\!br");
                RESET_AFTER_BREAK();
                ++i;
                free(lower);
                free(bare);
                continue;
            }
            free(lower);
        }

        /* pre-copula break before "is" */
        {
            char *lower = strdup(bare ? bare : "");
            if (lower)
                str_to_lower(lower);
            Boolean in_head = (i < 2);
            if (!in_head && lower && strcmp(lower, "is") == 0) {
                size_t k = i + 1;
                while (k < v.count && is_tag(v.items[k]))
                    ++k;
                Boolean trigger = false;
                if (k < v.count) {
                    if (is_titlecase_word(v.items[k]) || is_number_token(v.items[k]) || is_single_letter(v.items[k]) || weight_token(v.items[k]) == WT_HEAVY) {
                        trigger = true;
                    }
                    char *b2 = strip_trailing_punct(v.items[k]);
                    if (b2) {
                        str_to_lower(b2);
                        if (strcmp(b2, "the") == 0 || strcmp(b2, "thee") == 0 || strcmp(b2, "a") == 0 || strcmp(b2, "an") == 0)
                            trigger = true;
                    }
                    free(b2);
                }
                if (trigger && !ends_with_br(out.data)) {
                    sb_append_str(&out, " \\!br");
                    RESET_AFTER_BREAK();
                }
            }
            free(lower);
        }

        /* append token */
        if (out.len > 0 && !starts_with_punct(v.items[i]) && !is_open_punct(v.items[i]))
            sb_append_char(&out, ' ');
        sb_append_str(&out, v.items[i]);

        /* punctuation boundary */
        size_t len_tok = strlen(v.items[i]);
        if (len_tok > 0) {
            char last = v.items[i][len_tok - 1];
            if ((last == ',' || last == ';') && !ends_with_br(out.data)) {
                sb_append_str(&out, " \\!br");
                RESET_AFTER_BREAK();
                ++i;
                free(bare);
                continue;
            }
        }

        Weight w_eff = weight_token(v.items[i]);
        Boolean in_head = (i < 2);
        if (i == 0 && is_titlecase_word(v.items[i])) {
            if (i + 1 < v.count && !is_titlecase_word(v.items[i + 1]) && w_eff == WT_HEAVY)
                w_eff = WT_MEDIUM;
        }

        if (w_eff == WT_HEAVY && content_run >= 1) {
            int k = (int)i - 1;
            while (k >= 0 && is_tag(v.items[k]))
                --k;
            Boolean exempt = false;
            if (k >= 0) {
                if (is_titlecase_word(v.items[k]) || is_number_token(v.items[k]) || is_single_letter(v.items[k]))
                    exempt = true;
            }
            if (!exempt) {
                if (!ends_with_br(out.data))
                    sb_append_str(&out, " \\!br");
                RESET_AFTER_BREAK();
            }
        }

        if (w_eff == WT_HEAVY && content_run >= 2) {
            if (!ends_with_br(out.data))
                sb_append_str(&out, " \\!br");
            RESET_AFTER_BREAK();
        }

        if (w_eff == WT_HEAVY && since_break_all_light && !in_head) {
            if (out.len > 0 && !ends_with_br(out.data))
                sb_append_str(&out, " \\!br");
            RESET_AFTER_BREAK();
        }

        if (i > 0 && prev_w == WT_HEAVY && w_eff == WT_HEAVY && !in_head) {
            if (!ends_with_br(out.data))
                sb_append_str(&out, " \\!br");
            RESET_AFTER_BREAK();
        }

        if (w_eff == WT_LIGHT) {
            /* stay */
        } else {
            ++content_run;
            since_break_all_light = false;
        }
        prev_w = w_eff;
        ++i;
        free(bare);
    }

#undef RESET_AFTER_BREAK

    vec_free(&v);
    return str_trim(sb_steal(&out));
}

static char *tidy(char *s)
{
    if (!s)
        return NULL;
    StringBuilder sb;
    sb_init(&sb);

    const char *p = s;
    Boolean last_space = false;
    while (*p) {
        if (isspace((unsigned char)*p)) {
            if (!last_space)
                sb_append_char(&sb, ' ');
            last_space = true;
        } else {
            sb_append_char(&sb, *p);
            last_space = false;
        }
        ++p;
    }

    char *mid = sb_steal(&sb);
    free(s);

    /* normalize tag spacing */
    StringVec tok = tokenize_ws(mid);
    sb_init(&sb);
    for (size_t i = 0; i < tok.count; ++i) {
        if (i > 0)
            sb_append_char(&sb, ' ');
        sb_append_str(&sb, tok.items[i]);
    }
    vec_free(&tok);
    free(mid);
    return str_trim(sb_steal(&sb));
}

static char *normalize_letter_tokens(char *s)
{
    StringVec tok = tokenize_ws(s);
    if (tok.count == 0) {
        vec_free(&tok);
        return s;
    }

    StringBuilder out;
    sb_init(&out);

    for (size_t i = 0; i < tok.count; ++i) {
        if (is_tag(tok.items[i]))
            goto append_original;

        char *core = NULL;
        char *tail = NULL;
        Boolean had_colon = false;
        {
            size_t n = strlen(tok.items[i]);
            size_t end = n;
            while (end > 0) {
                char c = tok.items[i][end - 1];
                if (c == ':' && !had_colon) {
                    had_colon = true;
                    --end;
                    continue;
                }
                if (c == ',' || c == '.' || c == ';' || c == '!' || c == '?' || c == '"' || c == '\'') {
                    --end;
                    continue;
                }
                break;
            }
            core = dup_slice(tok.items[i], end);
            tail = dup_slice(tok.items[i] + end, n - end);
        }

        Boolean near_left = (i == 0) || is_tag(tok.items[i - 1]);
        Boolean near_right = (i + 1 >= tok.count) || is_tag(tok.items[i + 1]);
        Boolean near_bound = near_left || near_right;

        size_t core_len = core ? strlen(core) : 0;
        if ((tok.items[i] && strchr(tok.items[i], ':')) ||
            (core_len > 1 && !(core_len == 2 && core[1] == ':'))) {
            if (out.len > 0)
                sb_append_char(&out, ' ');
            sb_append_str(&out, tok.items[i]);
            free(core);
            free(tail);
            continue;
        }

        if (core && (strcmp(core, "A") == 0 || strcmp(core, "a") == 0) && (near_bound || had_colon)) {
            free(core);
            core = strdup("Ay");
            had_colon = true;
        } else if (core && strlen(core) == 1 && isalpha((unsigned char)core[0]) && isupper((unsigned char)core[0]) && near_bound) {
            had_colon = true;
        }

        if (out.len > 0)
            sb_append_char(&out, ' ');
        sb_append_str(&out, core ? core : tok.items[i]);
        if (had_colon)
            sb_append_char(&out, ':');
        if (tail)
            sb_append_str(&out, tail);

        free(core);
        free(tail);
        continue;

    append_original:
        if (out.len > 0)
            sb_append_char(&out, ' ');
        sb_append_str(&out, tok.items[i]);
    }

    vec_free(&tok);
    free(s);
    return sb_steal(&out);
}

static char *apply_leadin_and_thee_and_numbers(char *sent)
{
    sent = apply_thee_rule(sent);
    sent = apply_leadin_break(sent);
    sent = apply_time_numbers_degrees(sent);
    return sent;
}

char *vox_process_buffer(const char *text, Boolean wrap_vox_tags)
{
    StringVec sents = split_sentences(text);
    StringBuilder out;
    sb_init(&out);

    for (size_t i = 0; i < sents.count; ++i) {
        char *sent = sents.items[i];
        sents.items[i] = NULL; /* ownership transferred */
        if (!sent || !*sent)
            continue;
        sent = apply_leadin_and_thee_and_numbers(sent);
        char *beats = build_beats(sent);
        free(sent);
        beats = normalize_letter_tokens(beats);
        beats = tidy(beats);
        size_t len = strlen(beats);
        if (len > 0 && beats[len - 1] != ' ')
            sb_append_char(&out, ' ');
        sb_append_str(&out, beats);
        sb_append_str(&out, "\\!sf500 \\!br");
        free(beats);
        if (i + 1 < sents.count)
            sb_append_char(&out, ' ');
    }

    vec_free(&sents);

    char *result = str_trim(sb_steal(&out));
    if (result && *result) {
        if (wrap_vox_tags) {
            StringBuilder wrapped;
            sb_init(&wrapped);
            sb_append_str(&wrapped, "\\!wH1 ");
            sb_append_str(&wrapped, result);
            sb_append_str(&wrapped, " \\!wH0 ");
            free(result);
            result = sb_steal(&wrapped);
        } else {
            StringBuilder spaced;
            sb_init(&spaced);
            sb_append_str(&spaced, result);
            sb_append_char(&spaced, ' ');
            free(result);
            result = sb_steal(&spaced);
        }
    }

    return result;
}

static Boolean emit_break(StringBuilder *sb)
{
    return sb_append_str(sb, "[[slnc 250]] ");
}

static Boolean emit_version_prefix(StringBuilder *sb)
{
    return sb_append_str(sb, "[[vers 1]] ");
}

static Boolean emit_comment(StringBuilder *sb, const char *tag)
{
    if (!tag)
        return true;
    return sb_append_str(sb, "[[cmnt ") && sb_append_str(sb, tag) && sb_append_str(sb, "]] ");
}

static Boolean translate_token(const char *tok, size_t tok_len, StringBuilder *sb)
{
    if (tok_len == 0)
        return true;

    if (tok_len == 4 && strncmp(tok, "\\!br", 4) == 0)
        return emit_break(sb);

    if (tok_len >= 4 && strncmp(tok, "\\!sf", 4) == 0) {
        long millis = 0;
        const char *p = tok + 4;
        while (*p >= '0' && *p <= '9') {
            millis = (millis * 10) + (*p - '0');
            ++p;
        }
        millis *= 10;
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "[[slnc %ld]] ", millis);
        return sb_append_str(sb, tmp);
    }

    if (tok_len == 4 && strncmp(tok, "\\!wH1", 4) == 0)
        return emit_comment(sb, "vox-start");
    if (tok_len == 4 && strncmp(tok, "\\!wH0", 4) == 0)
        return emit_comment(sb, "vox-end");

    /* Unknown FlexTalk escape: drop it so only Speech Manager metadata remains. */
    return true;
}

static Handle emit_speech_manager_text(const char *text, Boolean wrap_with_version)
{
    Handle h = NULL;
    StringBuilder sb;
    sb_init(&sb);

    if (wrap_with_version) {
        if (!emit_version_prefix(&sb))
            goto done;
    }

    const char *p = text;
    while (*p) {
        if (p[0] == '\\' && p[1] == '!') {
            const char *start = p;
            p += 2;
            while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
                ++p;
            if (!translate_token(start, (size_t)(p - start), &sb))
                goto done;
            continue;
        }
        sb_append_char(&sb, *p);
        ++p;
    }
    emit_break(&sb);

    h = NewHandle(sb.len + 1);
    if (h && *h)
        BlockMoveData(sb.data, *h, sb.len + 1);

done:
    free(sb.data);
    return h;
}

Handle vox_format_text(const char *text, Boolean wrap_with_version)
{
    if (!text)
        return NULL;
    char *vox = vox_process_buffer(text, true);
    if (!vox)
        return NULL;
    Handle h = emit_speech_manager_text(vox, wrap_with_version);
    free(vox);
    return h;
}

