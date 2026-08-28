# Shared Web Assets

Contains files which are identical for every project, either acting as a distinct file or part of a larger library.

## Direct Files

These files should be pre-compressed using GZIP only if the source file has been modified (newer than the current .gz file):

- Data
  - help.json --> `help.json.gz`
- HTML
  - swaggerui.html --> `swaggerui.html.gz`
- Images
  - favicon.ico --> `favicon.ico.gz`
  - favicon.svg --> `favicon.svg.gz`

TODO: Determine where to place only the .gz file so that it is accessible to project builds.

## Combined Files

These files should be combined IN ORDER to produce a new file unique for each project. If ANY of the files in the shared WebAssets folder or a local files in a project has changed since the current per-project .gz file then the contents must be re-combined and compressed new for each project:

- JavaScript --> `<project>/assets/common.js.gz`
  - Shared:
    - api.js
    - dom.js
    - help.js
    - utils.js
  - Project:
    - common.js
- StyleSheets --> `<project>/assets/style.css.gz`
  - Shared:
    - base.css
    - controls.css
    - animation.css
  - Project:
    - style.css

## Common Include

To simplify the inclusion of common files a `CommonAssets.h` may be included in the Webhandler.h to define the `extern` statements which agree with the `board_build.embed_files` of the platformio.ini file per-project.

## SCons Build Process

### Overview

The SCons build process automatically manages asset preparation for each project during the PlatformIO build phase. A shared build script in `SharedLib/WebAssets/prepare_assets.py` handles all copy, combine, and compress operations with proper dependency tracking. This ensures:

- Files are automatically rebuilt only when sources change
- Single source of truth in SharedLib/WebAssets
- Consistent output across all projects
- Works identically in CLI, GitHub Actions, VSCode, and other build environments

### Architecture

**Shared Build Script Location:**
```
source/SharedLib/WebAssets/prepare_assets.py
```
This SCons-based script is referenced by all projects.

**Per-Project Configuration:**
Each project's `platformio.ini` includes:
```ini
extra_scripts = pre:../SharedLib/WebAssets/prepare_assets.py
```

**Output Location:**
All generated `.gz` files are placed in each project's `assets/` directory:
```
<project>/assets/
├── help.json.gz
├── swaggerui.html.gz
├── favicon.ico.gz
├── favicon.svg.gz
├── common.js.gz
└── style.css.gz
```

### Build Workflow

**Phase 1: Pre-Build Preparation (via SCons)**
1. Script runs BEFORE PlatformIO build
2. SCons checks dependency timestamps for each asset type
3. Executes only the required operations (see phases below)

**Phase 2: Direct File Compression**
For each direct file, if source is newer than output:
- Copy from `../SharedLib/WebAssets/Data/help.json` → `assets/help.json`
- Copy from `../SharedLib/WebAssets/HTML/swaggerui.html` → `assets/swaggerui.html`
- Copy from `../SharedLib/WebAssets/Images/favicon.ico` → `assets/favicon.ico`
- Copy from `../SharedLib/WebAssets/Images/favicon.svg` → `assets/favicon.svg`
- Compress each file: `file` → `file.gz`

**Phase 3: Combined File Generation**
For each combined file, if ANY source is newer than output:

*JavaScript Concatenation → Compression:*
- Combine in order:
  1. `assets/common.js` (project-specific)
  2. `../SharedLib/WebAssets/JavaScript/api.js` (shared)
  3. `../SharedLib/WebAssets/JavaScript/dom.js` (shared)
  4. `../SharedLib/WebAssets/JavaScript/help.js` (shared)
  5. `../SharedLib/WebAssets/JavaScript/utils.js` (shared)
- Output: `assets/common.js.gz`

*StyleSheet Concatenation → Compression:*
- Combine in order:
  1. `../SharedLib/WebAssets/StyleSheets/base.css` (shared)
  2. `../SharedLib/WebAssets/StyleSheets/controls.css` (shared)
  3. `../SharedLib/WebAssets/StyleSheets/animations.css` (shared)
  4. `assets/style.css` (project-specific)
- Output: `assets/style.css.gz`

**Phase 4: PlatformIO Embedding**
After script completes, PlatformIO's `board_build.embed_files` references only local files:
```ini
board_build.embed_files =
  assets/help.json.gz
  assets/swaggerui.html.gz
  assets/favicon.ico.gz
  assets/favicon.svg.gz
  assets/common.js.gz
  assets/style.css.gz
  assets/index.html.gz
  assets/index.js.gz
  assets/device.html.gz
  assets/network.html.gz
  assets/password.html.gz
```

### Symbol Name Generation

Because all embedded files are in the local `assets/` directory, binary symbols are generated consistently:
- `assets/help.json.gz` → `_binary_assets_help_json_gz_start`, `_binary_assets_help_json_gz_end`
- `assets/swaggerui.html.gz` → `_binary_assets_swaggerui_html_gz_start`, `_binary_assets_swaggerui_html_gz_end`
- `assets/favicon.ico.gz` → `_binary_assets_favicon_ico_gz_start`, `_binary_assets_favicon_ico_gz_end`
- `assets/favicon.svg.gz` → `_binary_assets_favicon_svg_gz_start`, `_binary_assets_favicon_svg_gz_end`
- `assets/common.js.gz` → `_binary_assets_common_js_gz_start`, `_binary_assets_common_js_gz_end`
- `assets/style.css.gz` → `_binary_assets_style_css_gz_start`, `_binary_assets_style_css_gz_end`

### Shared CommonAssets.h

All projects include a single shared header from `SharedLib/WebAssets/CommonAssets.h` (no per-project copies needed). The header declares extern symbols matching the generated binary names:

**Location:** `source/SharedLib/WebAssets/CommonAssets.h`

**Include in each project:**
```c
#include "../../SharedLib/WebAssets/CommonAssets.h"
```

**File contents:**
```c
#pragma once

// Direct files
extern const uint8_t _binary_assets_help_json_gz_start[];
extern const uint8_t _binary_assets_help_json_gz_end[];
extern const uint8_t _binary_assets_swaggerui_html_gz_start[];
extern const uint8_t _binary_assets_swaggerui_html_gz_end[];
extern const uint8_t _binary_assets_favicon_ico_gz_start[];
extern const uint8_t _binary_assets_favicon_ico_gz_end[];
extern const uint8_t _binary_assets_favicon_svg_gz_start[];
extern const uint8_t _binary_assets_favicon_svg_gz_end[];

// Combined files
extern const uint8_t _binary_assets_common_js_gz_start[];
extern const uint8_t _binary_assets_common_js_gz_end[];
extern const uint8_t _binary_assets_style_css_gz_start[];
extern const uint8_t _binary_assets_style_css_gz_end[];
```

### Dependency Tracking

SCons automatically tracks:
- **Direct files:** Source file mtime vs. output `.gz` mtime
- **Combined files:** All input source files vs. output `.gz` mtime
- **First build:** All operations execute (files don't exist)
- **Subsequent builds:** Only changed sources trigger rebuild

Example:
- Edit `../SharedLib/WebAssets/JavaScript/api.js` → Next build rebuilds `common.js.gz`
- Edit `assets/style.css` → Next build rebuilds `style.css.gz`
- No changes → Next build skips all operations (SCons handles this)

### Git Ignore

Add to project `.gitignore`:
```
assets/*.gz
```
This ensures only source files are committed; `.gz` files are generated fresh on each build across all environments (CLI, GitHub Actions, Docker, etc.).

### Implementation Checklist

- [ ] Create `source/SharedLib/WebAssets/prepare_assets.py` (SCons builder script)
- [ ] Verify `source/SharedLib/WebAssets/CommonAssets.h` has correct extern declarations
- [ ] Update each project's `platformio.ini` to reference the script with `pre:../SharedLib/WebAssets/prepare_assets.py`
- [ ] Update each project's `board_build.embed_files` to reference only `assets/*` files
- [ ] Add `#include "../../SharedLib/WebAssets/CommonAssets.h"` in each project's web handler
- [ ] Add `assets/*.gz` to `.gitignore` in each project
- [ ] Test build in CLI: `pio run --project-dir <project>`
- [ ] Verify symbol resolution during link phase
- [ ] Test in GitHub Actions workflow

