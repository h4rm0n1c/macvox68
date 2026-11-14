# Parser buffer ownership fix — 2026-11-14

The three existing host checks reproduced nine leaked allocations totaling
1,152 bytes under AddressSanitizer with leak detection enabled.

`sb_steal()` transfers ownership of its buffer to the caller. `str_trim()`
allocates a separate copy and borrows its input. Passing the former directly
to the latter lost the original allocation in `build_beats()`, `tidy()`, and
`vox_process_buffer()`.

Those three calls now use `str_trim_owned()`, which frees the transferred
buffer after trimming, including when allocating the trimmed copy fails.
The borrowing behavior of `str_trim()` remains unchanged for other callers.

Validation from the workspace root:

```sh
cc -DVOX_PARSER_STANDALONE -g -O1 -Wall -Wextra \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  Retro68Projects/MacVox68/tests/vox_parser_host_checks.c \
  Retro68Projects/MacVox68/vox_parser.c -o /tmp/macvox-parser-sanitized
ASAN_OPTIONS=detect_leaks=1 /tmp/macvox-parser-sanitized
```

All three checks pass without sanitizer diagnostics after the fix. This
validates the exercised host paths; it does not establish parser parity with
VoxTalk, general allocation-failure safety, or guest runtime correctness.
