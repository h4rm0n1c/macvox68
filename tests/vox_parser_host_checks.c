#include "vox_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int assert_contains(const char *label, const char *haystack, const char *needle)
{
    if (!haystack || !needle || !strstr(haystack, needle)) {
        fprintf(stderr, "%s: expected to find '%s'\n", label, needle);
        return 1;
    }
    return 0;
}

static int check_process_pipeline(void)
{
    int errors = 0;
    const char *input = "The time is 8:47 AM.";
    char *out = vox_process_buffer(input, 1);
    if (!out)
        return 1;
    errors += assert_contains("pipeline", out, "The time \\!br is \\!br");
    errors += assert_contains("pipeline", out, "\\!sf500 \\!br \\!wH0");
    free(out);
    return errors;
}

static int check_thee_and_breaks(void)
{
    int errors = 0;
    const char *input = "On behalf of The Board, please wait.";
    char *out = vox_process_buffer(input, 1);
    if (!out)
        return 1;
    errors += assert_contains("thee", out, "thee Board");
    errors += assert_contains("breaks", out, "\\!br of \\!br thee Board");
    free(out);
    return errors;
}

static int check_speech_manager_translation(void)
{
    int errors = 0;
    const char *input = "Hello there.";
    Handle h = vox_format_text(input, true);
    if (!h || !*h)
        return 1;
    char *buf = (char *)(*h);
    errors += assert_contains("speech-manager", buf, "[[vers 1]]");
    if (strstr(buf, "[[slnc 250]]")) ++errors;
    errors += assert_contains("sentence-boundary", buf, "Hello there.");
    errors += assert_contains("silence-ms", buf, "[[slnc 500]]");
    errors += assert_contains("wrapper", buf, "[[cmnt vox-start]]");
    errors += assert_contains("wrapper", buf, "[[cmnt vox-end]]");
    if (strstr(buf, "[[slnc 5000]]")) ++errors;
    if (strstr(buf, "\\!br"))
        errors += assert_contains("speech-manager", buf, "(no flex escapes expected)");
    free(*h);
    free(h);
    return errors;
}

static int check_reference_cadence(void)
{
    static const struct { const char *input, *expected; } cases[] = {
        {"A reminder to all personnel.", "Ay: reminder \\!br to \\!br all \\!br personnel. \\!sf500 \\!br "},
        {"This tram is departing.", "This tram \\!br is \\!br departing. \\!sf500 \\!br "},
        {"The time is 12:05 PM.", "The time \\!br is \\!br 12 \\!br 05 \\!br P: \\!br M: \\!sf500 \\!br "},
        {"It is 1300 hours.", "It is \\!br 13 \\!br hundred \\!br hours. \\!sf500 \\!br "},
        {"", ""}, {" ", ""}
    };
    int errors = 0;
    size_t i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        char *out = vox_process_buffer(cases[i].input, false);
        if (!out || strcmp(out, cases[i].expected)) {
            fprintf(stderr, "cadence '%s': got '%s'\n", cases[i].input, out ? out : "(null)");
            ++errors;
        }
        free(out);
    }
    for (i = 0; i < 2; ++i) {
        Handle blank = vox_format_text(" ", (Boolean)i);
        if (!blank || !*blank || (!i && **blank)) ++errors;
        if (blank) { free(*blank); free(blank); }
    }
    return errors;
}

int main(void)
{
    int errors = 0;
    errors += check_reference_cadence();
    {
        const char *times[] = {"The time is 08:45 AM.", "The time is 08:45 PM."};
        const char *letters[] = {"ay em", "pee em"};
        size_t i;
        for (i = 0; i < 2; ++i) {
            Handle time = vox_format_text(times[i], true);
            if (!time || !*time) ++errors;
            else {
                const char *text = (char *)*time;
                errors += assert_contains("meridiem-letter-pair", text, letters[i]);
                if (strstr(text, "Ay:") || strstr(text, "P:") || strstr(text, "M:")) ++errors;
                errors += assert_contains("meridiem-final-silence", text, "[[slnc 500]]");
            }
            if (time) { free(*time); free(time); }
        }
    }

    errors += check_process_pipeline();
    errors += check_thee_and_breaks();
    errors += check_speech_manager_translation();
    Handle beats = vox_format_text("This tram is departing. Please wait!", true);
    if (!beats || !*beats) ++errors;
    else {
        const char *text = (char *)*beats;
        errors += assert_contains("internal-word-boundary", text, "tram ");
        errors += assert_contains("source-full-stop", text, "departing.");
        errors += assert_contains("source-exclamation", text, "wait!");
        if (strstr(text, "tram.") || strstr(text, "is.") || strstr(text, "[[slnc 60]]")) ++errors;
    }
    if (beats) { free(*beats); free(beats); }

    Handle boundary = vox_format_text("On behalf of The Board, please wait.", true);
    if (!boundary || !*boundary) ++errors;
    else {
        const char *text = (char *)*boundary;
        if (strstr(text, "[[slnc 250]]") || strstr(text, ". .")) ++errors;
        errors += assert_contains("deduplicated-boundary", text, "of ");
        const char *pause = strstr(text, "[[slnc 500]]");
        if (!pause || strstr(pause + 12, "[[slnc 500]]")) ++errors;
    }
    if (boundary) { free(*boundary); free(boundary); }
    Handle h = vox_format_text("Hello [[[volm 0]] there.", true);
    if (!h || !*h || strstr((char *)*h, "[[volm")) ++errors;
    if (h) { free(*h); free(h); }
    if (errors) {
        fprintf(stderr, "vox_parser_host_checks: %d issues\n", errors);
        return 1;
    }
    printf("vox_parser_host_checks: ok\n");
    return 0;
}
