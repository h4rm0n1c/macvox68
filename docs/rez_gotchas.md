# Rez gotchas (Retro68) — avoid losing hours

This project uses Retro68's `Rez` to compile `.r` files into resource blobs (e.g. `dialog.r.rsrc.bin`).

Retro68 Rez is **not** Apple MPW Rez. It is mostly compatible, but it has sharp edges.
When it fails, it may crash with assertions instead of printing a useful error.

## Known failure mode: Shell breaks `-I` paths containing `&`

If your include path contains `&` (e.g. `Interfaces&Libraries`), and the path is not quoted,
the shell treats `&` as “background command” and breaks the Rez invocation.

Symptoms:
- `/bin/sh: 1: Libraries/Interfaces/RIncludes: not found`
- Followed by an internal Rez assertion like:
  - `ArrayField::compile Assertion 'compound' failed`

Fix:
- **Always quote** any `-I` paths containing `&`, or
- Create a symlink with a safe name and use that in CMake:
  - `ln -s "Interfaces&Libraries" InterfacesAndLibraries`

## Known failure mode: DITL syntax can trigger Rez internal assertions

Retro68 Rez can assert with messages like:
- `SwitchField::compile Assertion 'caseExpr' failed`
- `ArrayField::compile Assertion 'compound' failed`

We hit this repeatedly when trying alternate DITL formatting.

### What works (known-good DITL form)

Use the DITL style from Retro68 dialog examples:

```rez
resource 'DITL' (128) {
    {
        { 160, 230, 180, 310 },
        Button { enabled, "Quit" };

        { 40, 10, 56, 310 },
        EditText { enabled, "Edit Text Item" };
    }
};
