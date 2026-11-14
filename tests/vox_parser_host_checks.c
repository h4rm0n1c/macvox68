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
    errors += assert_contains("breaks", out, "\\!br of \\!br \\!br");
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

int main(void)
{
    int errors = 0;
    errors += check_process_pipeline();
    errors += check_thee_and_breaks();
    errors += check_speech_manager_translation();
    Handle boundary = vox_format_text("On behalf of The Board, please wait.", true);
    if (!boundary || !*boundary) ++errors;
    else {
        const char *text = (char *)*boundary;
        if (strstr(text, "[[slnc 250]]") || strstr(text, ". .")) ++errors;
        errors += assert_contains("deduplicated-boundary", text, "of.");
        const char *pause = strstr(text, "[[slnc 500]]");
        if (!pause || strstr(pause + 12, "[[slnc")) ++errors;
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
