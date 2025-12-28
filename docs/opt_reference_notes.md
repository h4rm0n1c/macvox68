# `/opt` reference caches quick guide

The container includes classic Mac reference material under `/opt` for lookup-only usage. Treat these caches as read-only helpers; do not vendor their contents into the repository.

## What is available
- **Retro68**: `/opt/Retro68` reference checkout for build scripts and sample patterns.
- **Interfaces&Libraries**: `/opt/Interfaces&Libraries` headers/libs from MPW era (quoted path needed when passed to Rez or the compiler).
- **MPW**: `/opt/MPW` tools and documentation.
- **MacDevDocs**: `/opt/MacDevDocs` developer documentation archive with multiple eras of Inside Macintosh (early volumes through post-System 7 revisions), Tech Notes, Developer Notes, *develop* magazine issues, and full sample projects including resource fork binaries. The overlay now spans 1992–1997 Developer CDs plus an Appearance SDK drop, so the Inside Mac revisions and sample projects cover nearly the entire System 7 lifecycle.
- **Latest Notes From Apple**: `/opt/Latest Notes From Apple` developer notes.
- **MacExamples / MacExamples_TextOnly**: `/opt/MacExamples` and `/opt/MacExamples_TextOnly` extracted sample code and resource sidecars.
- **sys71src**: `/opt/sys71src` System 7.1 source reference drop (read-only) that can clarify Toolbox behavior beyond the API docs.

## How to use these caches effectively
- Prefer these local references over web searches when looking up Toolbox API signatures, Rez idioms, or classic UI patterns.
- Use `rg` (ripgrep) in the text-only examples to quickly locate relevant sample code, e.g. `rg -g "*.r" TrackControl /opt/MacExamples_TextOnly`.
- For PDFs (Inside Macintosh, developer notes), use `pdftotext` or `pdfinfo` from Poppler to search without exporting content into the repo. The later MacDevDocs discs ship Acrobat catalog indexes (look for `Acrobat Indexes` folders) that can accelerate PDF searches if you have an Acrobat-compatible viewer handy.
- Keep resource idioms consistent with the examples; avoid modernizing patterns that conflict with classic System 7 expectations.
- Skim `docs/macdevdocs_overlay_map.md` before diving into `/opt/MacDevDocs` to jump to the right disc for a topic (Speech Manager, MacTCP, Appearance Manager, etc.).

## Reminders
- Do not copy or commit files from `/opt`.
- Builds should stay out-of-tree; `/opt` is only for reference lookups.
