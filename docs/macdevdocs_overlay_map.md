# `/opt/MacDevDocs` overlay map (January 2025 refresh)

The `/opt/MacDevDocs` tree now mirrors multiple Apple Developer CD snapshots (1992–1997) plus the Appearance SDK bundle. Use this
as a quick locator for where to look before diving into the archive. All content is **reference-only**; never copy files into the
repo.

## High-level layout
- **Developer CDs by year/edition**: `1992-03-DevCD`, `1992-07-DevCD`, `1993-01-DevCD`, `1993-12-DevCD`, `1994-03-DevCD`,
  `1994-12-DevCD-RL`, `1995-08-DevCD-TC`, `1996-07-DevCD-SSW`, `1996-12-DevCD-RL`, `1997-01-DevCD-01`, `1997-01-DevCD-02`.
  Each disc preserves its original `Reference Library`, `Technical Documentation`, `Utilities`, and `What's New?` layout.
- **Acrobat cross-disc indexes**: Later discs include `Acrobat Indexes` directories (e.g., `1995-08-DevCD-TC/Acrobat Indexes`,
  `1996-12-DevCD-RL/Acrobat Indexes`, `1997-01-DevCD-02/Acrobat Indexes`). These hold prebuilt Acrobat catalog files that can be
  pointed at when searching PDFs locally.
- **Appearance SDK drop**: `1997-AppearanceSDK` contains the Appearance Manager SDK (Read Me, versions list, and sample code)
  alongside flattened `.finf.bin`/`.rsrc.bin` sidecars for Finder metadata.

## Quick topic pointers for MacVox68
- **Speech Manager + Sound**: Use Inside Macintosh volumes and tech notes under the `Technical Documentation/Inside Macintosh` and
  `Technical Documentation/Mac Tech Notes` branches on the 1993–1996 discs. Search those PDFs with the Acrobat indexes from the
  same disc when available.
- **MacTCP / networking**: Check `Technical Documentation` on the mid-90s discs (1994–1996) for Inside Macintosh: Networking and
  MacTCP guides, and cross-reference sample clients in `1995-08-DevCD-TC/Sample Code`.
- **UI controls / Appearance**: The Appearance SDK bundle (`1997-AppearanceSDK`) and the `Reference Library` folders on the
  1996–1997 discs include updated Human Interface Guidelines and Appearance Manager notes. Look here for slider/push button idioms
  before mirroring them in `window_ui.c`.
- **Sample code sets**: `1995-08-DevCD-TC/Sample Code` aggregates AOCE and QuickTime utilities, menu/Palette samples, and
  other toolbox demos we can mine for Rez/control patterns without importing their assets into the repo.

## Usage reminders
- Treat `/opt/MacDevDocs` as read-only. Use `pdftotext` or `rg` against the tree for lookup, and keep any learnings in the
  repository’s `docs/` notes instead of copying source files.
