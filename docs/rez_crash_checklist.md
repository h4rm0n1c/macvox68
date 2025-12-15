# Rez Crash Checklist (Retro68)

Use this checklist whenever Retro68 Rez fails, asserts, or produces nonsense. Follow it in order.

1) Capture the exact command line
   - Ninja:  ninja -v
   - Make:   make VERBOSE=1
   Copy/paste the full Rez invocation and the first error. Do not paraphrase.

2) Confirm which Rez is running
   - which Rez
   - Rez --help
   Ensure it is the Retro68 Rez you expect (not MPW Rez, not another Rez on PATH).

3) Eliminate shell metacharacter breakage (especially '&')
   If any -I include path contains '&' (e.g. Interfaces&Libraries), it MUST be quoted or avoided.
   Symptoms of shell breakage:
     /bin/sh: 1: Libraries/Interfaces/RIncludes: not found
   Fix options:
   - Quote the include path (recommended if you must keep the name):
     -I"/path/Interfaces&Libraries/Interfaces/RIncludes"
   - Prefer a safe symlink (recommended for builds and CMake variables):
     cd "/home/harri/OldMacStuff/B2/Shared"
     ln -s "Interfaces&Libraries" InterfacesAndLibraries
     Then use:
     /home/harri/OldMacStuff/B2/Shared/InterfacesAndLibraries/...

4) Re-run Rez manually with debug enabled
   Run the same command but add --debug so you can see where it dies:
     /path/to/Rez --debug input.r -I/path/to/Retro68/.../RIncludes -o out.rsrc
   Note the last resource type it prints (e.g. DLOG, DITL) before the crash.

5) If it dies on DITL: lock the known-good DITL syntax
   Retro68 Rez is extremely sensitive to DITL formatting. Use the “two-line item” form:
     resource 'DITL' (128) {
         {
             { 160, 230, 180, 310 },
             Button { enabled, "Quit" };

             { 40, 10, 56, 310 },
             EditText { enabled, "Edit Text Item" };
         }
     };
   Rules:
   - Each item is TWO statements: a rect line, then an item line.
   - Item lines end with a semicolon.
   - Do not refactor into “one compound per item” forms unless you have tested with Retro68 Rez.
   Debug signature of “DITL not being recognized as items”:
   - In --debug, the first pass may show the item-count-minus-one field as 0xFFFF, then Rez asserts.

6) Reduce to a minimal test file (prove Rez + Dialogs.r works)
   Create dialog_min.r with only:
     #include "Types.r"
     #include "Dialogs.r"

     resource 'DLOG' (128) {
         { 50, 100, 240, 420 },
         dBoxProc,
         visible,
         noGoAway,
         0,
         128,
         "Test",
         centerMainScreen
     };

     resource 'DITL' (128) {
         {
             { 160, 230, 180, 310 },
             Button { enabled, "Quit" };
         }
     };

   Compile using only Retro68 RIncludes:
     Rez --debug dialog_min.r -I/path/to/Retro68/.../RIncludes -o dialog_min.rsrc.bin

   Interpretation:
   - If dialog_min.r fails: your Rez/toolchain/include set is broken or mismatched; stop editing project resources and fix toolchain/includes first.
   - If dialog_min.r works: the problem is in your extra includes/resources; proceed to step 7.

7) Reintroduce includes and resources one chunk at a time
   Start from the minimal file and add back, in this order, compiling after each addition:
   - Add includes: Finder.r, Icons.r, Processes.r (one at a time)
   - Add resources: SIZE, BNDL, FREF, ICN# (one at a time)
   Stop at the first failure. The last change is the trigger.

8) Avoid mixing multiple RIncludes trees unless you must
   Mixing Apple MPW RIncludes and Retro68 RIncludes can cause template/macro mismatches and obscure asserts.
   Recommended default for this repo:
   - Use only Retro68:  .../m68k-apple-macos/RIncludes
   Only add additional -I paths if you have a concrete reason and have tested the result.

9) Encoding sanity check (rare, but real)
   Ensure .r files are:
   - ASCII or UTF-8 without BOM
   - No smart quotes
   - Prefer LF line endings
   Rez errors will not reliably tell you if encoding is the problem.

10) Harden the build system (CMake/Ninja)
   In CMake add_custom_command() that invokes Rez:
   - Pass args as separate tokens (not one shell string)
   - Use VERBATIM
   - Ensure any path containing '&' is either quoted or replaced by a safe symlink path
   This prevents Ninja/shell splitting and makes builds reproducible.

11) Prefer code over resources where practical
   Rez is required for Finder integration and classic resource-based UI (DLOG/DITL/MENU/MBAR).
   But for basic UI:
   - Use NewWindow, NewControl, etc.
   Fewer resources means fewer Rez failure points.

12) Success signature
   With --debug, a successful run ends with:
     Writing N resources.
   For a DITL with X items, the second pass should emit the count as X-1 (e.g. 7 items -> 6).
