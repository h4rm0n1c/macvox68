# Rez gotchas (Retro68) — avoid losing hours

This project uses Retro68's `Rez` to compile `.r` files into resource blobs (e.g. `dialog.r.rsrc.bin`).

Retro68 Rez is **not** Apple MPW Rez. It is mostly compatible, but it has sharp edges.
When it fails, it may crash with assertions instead of printing a useful error.

---

## 1) Shell breaks `-I` paths containing `&`

If your include path contains `&` (e.g. `Interfaces&Libraries`) and the path is not quoted,
the shell treats `&` as “run previous command in background” and the Rez invocation gets split.

**Symptoms**
- `/bin/sh: 1: Libraries/Interfaces/RIncludes: not found`
- Followed by an internal Rez assertion, commonly:
  - `ArrayField::compile Assertion 'compound' failed`

**Cause**
- The `-I...Interfaces&Libraries...` argument gets split by the shell.
- Rez then runs with missing include paths and can crash in confusing ways.

**Fix**
- Always quote any `-I` path that contains `&`, e.g.:
  - `-I"/path/Interfaces&Libraries/Interfaces/RIncludes"`
- Or create a safe symlink and use that instead:
  - `cd "/home/harri/OldMacStuff/B2/Shared"`
  - `ln -s "Interfaces&Libraries" InterfacesAndLibraries`

**Recommended policy for this repo**
- Prefer the symlink path (no `&`) in build scripts and CMake variables.

---

## 2) Retro68 Rez can assert on DITL formatting

Retro68 Rez may abort with assertions like:
- `SwitchField::compile Assertion 'caseExpr' failed`
- `ArrayField::compile Assertion 'compound' failed`

We hit this repeatedly when using DITL formatting that Apple Rez might accept, but Retro68 Rez
did not handle (or produced internal `switch` evaluation asserts).

### Known-good DITL form (works with Retro68 Rez)

Use the DITL style from Retro68 dialog examples: **rect line then item line**, with a semicolon
after the item statement.

```rez
resource 'DITL' (128) {
    {
        { 160, 230, 180, 310 },
        Button { enabled, "Quit" };

        { 40, 10, 56, 310 },
        EditText { enabled, "Edit Text Item" };
    }
};

Each dialog item is expressed as TWO statements:

a rect: { top, left, bottom, right },

then an item statement: Button { ... };, EditText { ... };, etc.

Use semicolons after item statements (... };)

Avoid “single compound per item” rewrites unless tested with Retro68 Rez.

What the failure looked like

With Rez --debug, failure typically occurred as soon as it started RESOURCE 'DITL',
often after writing the initial item-count field as 0xFFFF during the first pass, then
aborting with SwitchField::compile assertions.

3) How to debug Rez quickly

Run Rez directly with --debug to see what resource it was processing:

/path/to/Rez --debug dialog.r \
  -I/path/to/Retro68/toolchain-full/m68k-apple-macos/RIncludes \
  -o dialog.r.rsrc.bin


If you see it completes DLOG then dies at DITL, it's almost always DITL formatting
(or an include/template mismatch).

4) Recommendation: keep Rez resources minimal

Rez is required for:

Finder integration (Type/Creator, bundle resources)

Dialogs/menus defined as resources (DLOG, DITL, MENU, MBAR, etc.)

However, prefer code-defined UI where practical:

Windows via NewWindow

Controls via NewControl

This reduces reliance on Rez and avoids losing time to Rez parser/template limitations.

5) Build-system rule: always use CMake VERBATIM for Rez commands

When invoking Rez via CMake add_custom_command():

pass arguments as separate tokens (not a single shell string)

include paths must be separate arguments

use VERBATIM so quoting/escaping is preserved for Ninja

This prevents shell splitting and makes builds reproducible.


### `docs/build_rules_resources.md`
```markdown
# Resource build rules (Retro68)

This document describes the rules for building `.r` resources with Retro68.

---

## 1) What we build

- `dialog.r` is compiled by Retro68 `Rez` into:
  - `dialog.r.rsrc.bin`

The build system then incorporates that output into the final application as appropriate
(for Retro68 setups, this is commonly handled by helper macros or link steps).

---

## 2) Canonical Rez command shape

Always include Retro68 RIncludes:

```sh
Rez dialog.r \
  -I/path/to/Retro68/toolchain-full/m68k-apple-macos/RIncludes \
  -o dialog.r.rsrc.bin

3) Include paths containing & must be quoted

If an include path contains & (e.g. Interfaces&Libraries), you MUST quote it:

Rez dialog.r \
  -I/path/to/Retro68/.../RIncludes \
  -I"/path/with/Interfaces&Libraries/Interfaces/RIncludes" \
  -o dialog.r.rsrc.bin


Recommended alternative: use a symlink with a safe name (no &) and point builds at that.

4) Known-good DITL syntax for Retro68 Rez

Retro68 Rez accepts the “two-line item” DITL syntax:

resource 'DITL' (128) {
    {
        { 160, 230, 180, 310 },
        Button { enabled, "Quit" };

        { 40, 10, 56, 310 },
        EditText { enabled, "Edit Text Item" };
    }
};


Do not “refactor” or reformat the DITL into other equivalent-looking forms unless tested.

5) CMake/Ninja safety rules

When generating Rez build steps with CMake:

Pass include args as separate argv entries.

Prefer -I"..." or separate -I + "..." tokens depending on your tooling.

Use VERBATIM in add_custom_command() to preserve quoting and avoid shell splitting.

Example pattern:

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dialog.r.rsrc.bin"
  COMMAND "${REZ}" "${CMAKE_CURRENT_SOURCE_DIR}/dialog.r"
          "-I${RETRO68_RINCLUDES}"
          -o "${CMAKE_CURRENT_BINARY_DIR}/dialog.r.rsrc.bin"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/dialog.r"
  VERBATIM
)

6) Quick manual compile test

From a build directory:

/home/harri/OldMacStuff/Retro68/toolchain-full/bin/Rez --debug \
  /home/harri/OldMacStuff/Retro68Projects/MacVox68/dialog.r \
  -I/home/harri/OldMacStuff/Retro68/toolchain-full/m68k-apple-macos/RIncludes \
  -o /home/harri/OldMacStuff/Retro68Projects/MacVox68-build/dialog.r.rsrc.bin


Use this when diagnosing build failures before editing CMake rules.


If you want a third file, I can also write `docs/rez_checklist.md` as a short “if Rez explodes, do these 6 steps” checklist for future-you (and for Codex).
