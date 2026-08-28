"""
PlatformIO SCons Pre-Build Script: Asset Preparation
Purpose: Combine, compress, and stage web assets for embedding into firmware.

This script runs BEFORE PlatformIO's build phase via:
    extra_scripts = pre:../SharedLib/WebAssets/prepare_assets.py

It handles:
    1. Direct file compression (help.json, swaggerui.html, favicon.ico/svg)
    2. JavaScript combination (project common.js + 4 shared libraries)
    3. CSS combination (3 shared stylesheets + project style.css)
    4. Automatic dependency tracking via SCons
    5. Build cache invalidation if .gz files change

Verbosity Control:
    Set VERBOSE = True in the script for detailed output
    Default: VERBOSE = False (minimal output, errors and warnings only)
"""

import os
import gzip
import shutil
from pathlib import Path
from datetime import datetime

# Import SCons environment
try:
    Import("env")
    SCONS_ENV = env
    IN_SCONS_CONTEXT = True
except NameError:
    SCONS_ENV = None
    IN_SCONS_CONTEXT = False

# Configuration
VERBOSE = False  # Set to True for detailed output during development/debugging
GZIP_LEVEL = 9  # Maximum compression

def log(msg, level='INFO'):
    """Print log message with timestamp if verbose mode is enabled."""
    if VERBOSE or level in ['ERROR', 'WARNING']:
        timestamp = datetime.now().strftime('%H:%M:%S')
        prefix = f"[{timestamp}] [{level}]" if VERBOSE else f"[{level}]"
        print(f"{prefix} AssetPrep: {msg}")

def get_shared_dir():
    """Get path to SharedLib/WebAssets directory."""
    # This script is at: source/SharedLib/WebAssets/prepare_assets.py
    # Current working directory during pre-build: the project directory (e.g., source/ProtonPack)
    # So SharedLib/WebAssets is at: ../SharedLib/WebAssets
    return Path("../SharedLib/WebAssets")

def get_assets_dir():
    """Get path to project's assets directory."""
    return Path("assets")

def ensure_assets_dir():
    """Ensure assets directory exists."""
    assets_dir = get_assets_dir()
    if not assets_dir.exists():
        log(f"Creating assets directory: {assets_dir.absolute()}", 'WARNING')
        assets_dir.mkdir(parents=True, exist_ok=True)
    return assets_dir

def compress_file(source_path, output_path, description=""):
    """
    Compress a single file using gzip.
    
    Args:
        source_path: Path to source file
        output_path: Path to output .gz file
        description: Optional description for logging
    
    Returns:
        True if compression was performed, False if skipped
    """
    source = Path(source_path)
    output = Path(output_path)
    
    if not source.exists():
        log(f"Source not found: {source.absolute()}", 'WARNING')
        return False
    
    # Check if compression is needed
    needs_compression = (
        not output.exists() or
        source.stat().st_mtime > output.stat().st_mtime
    )
    
    if not needs_compression:
        log(f"Up-to-date: {description or output.name}")
        return False
    
    try:
        log(f"Compressing: {description or source.name} -> {output.name}")
        with open(source, 'rb') as f_input:
            with gzip.open(output, 'wb', compresslevel=GZIP_LEVEL) as f_output:
                shutil.copyfileobj(f_input, f_output)
        log(f"  Compressed: {output.stat().st_size} bytes", 'INFO')
        return True
    except Exception as e:
        log(f"Failed to compress {source.name}: {e}", 'ERROR')
        return False

def combine_and_compress_javascript():
    """
    Combine project-specific and shared JavaScript files, then compress.
    
    Order:
        1. ../SharedLib/WebAssets/JavaScript/api.js (shared)
        2. ../SharedLib/WebAssets/JavaScript/dom.js (shared)
        3. ../SharedLib/WebAssets/JavaScript/help.js (shared)
        4. ../SharedLib/WebAssets/JavaScript/utils.js (shared)
        5. assets/common.js (project-specific)
    
    Output: assets/common.js.gz
    """
    assets_dir = get_assets_dir()
    shared_dir = get_shared_dir()
    
    device_js = assets_dir / "common.js"
    combined_js = assets_dir / "combined.js"
    final_gz = assets_dir / "common.js.gz"
    
    js_sources = [
        shared_dir / "JavaScript" / "api.js",
        shared_dir / "JavaScript" / "dom.js",
        shared_dir / "JavaScript" / "help.js",
        shared_dir / "JavaScript" / "utils.js",
        device_js  # Project-specific code last
    ]
    
    # Check which files exist
    existing_sources = [f for f in js_sources if f.exists()]
    
    if not existing_sources:
        log("No JavaScript source files found, skipping", 'WARNING')
        return False
    
    # Check if rebuild is needed
    needs_rebuild = (
        not final_gz.exists() or
        any(f.stat().st_mtime > final_gz.stat().st_mtime for f in existing_sources)
    )
    
    if not needs_rebuild:
        log("JavaScript: up-to-date")
        return False
    
    log("Combining JavaScript files...")
    
    try:
        with open(combined_js, 'w', encoding='utf-8') as output:
            output.write("/* Combined JavaScript - Generated automatically */\n")
            output.write("/* DO NOT EDIT - Modify source files instead */\n\n")
            
            for js_file in existing_sources:
                log(f"  Including: {js_file.name}", 'INFO')
                output.write(f"/* === {js_file.name} === */\n")
                try:
                    with open(js_file, 'r', encoding='utf-8') as input_file:
                        content = input_file.read().strip()
                        if content:
                            output.write(content)
                            output.write("\n\n")
                except Exception as e:
                    log(f"Warning: Could not read {js_file.name}: {e}", 'WARNING')
        
        log(f"Combined JS file: {combined_js.stat().st_size} bytes")
        
        # Compress the combined file
        with open(combined_js, 'rb') as f_input:
            with gzip.open(final_gz, 'wb', compresslevel=GZIP_LEVEL) as f_output:
                shutil.copyfileobj(f_input, f_output)
        
        log(f"JavaScript compressed: {final_gz.stat().st_size} bytes")
        combined_js.unlink()  # Clean up intermediate file
        
        # Register SCons dependency if in build context
        if IN_SCONS_CONTEXT:
            SCONS_ENV.Depends(str(final_gz), [str(f) for f in existing_sources])
            log(f"Registered SCons dependencies for {final_gz.name}", 'INFO')
        
        return True
        
    except Exception as e:
        log(f"Failed to combine/compress JavaScript: {e}", 'ERROR')
        return False

def combine_and_compress_css():
    """
    Combine shared and project-specific CSS files, then compress.
    
    Order:
        1. ../SharedLib/WebAssets/StyleSheets/base.css (shared)
        2. ../SharedLib/WebAssets/StyleSheets/controls.css (shared)
        3. ../SharedLib/WebAssets/StyleSheets/animations.css (shared)
        4. assets/style.css (project-specific)
    
    Output: assets/style.css.gz
    """
    assets_dir = get_assets_dir()
    shared_dir = get_shared_dir()
    
    device_css = assets_dir / "style.css"
    combined_css = assets_dir / "combined.css"
    final_gz = assets_dir / "style.css.gz"
    
    css_sources = [
        shared_dir / "StyleSheets" / "base.css",
        shared_dir / "StyleSheets" / "controls.css",
        shared_dir / "StyleSheets" / "animations.css",
        device_css
    ]
    
    # Check which files exist
    existing_sources = [f for f in css_sources if f.exists()]
    
    if not existing_sources:
        log("No CSS source files found, skipping", 'WARNING')
        return False
    
    # Check if rebuild is needed
    needs_rebuild = (
        not final_gz.exists() or
        any(f.stat().st_mtime > final_gz.stat().st_mtime for f in existing_sources)
    )
    
    if not needs_rebuild:
        log("CSS: up-to-date")
        return False
    
    log("Combining CSS files...")
    
    try:
        with open(combined_css, 'w', encoding='utf-8') as output:
            output.write("/* Combined CSS - Generated automatically */\n")
            output.write("/* DO NOT EDIT - Modify source files instead */\n\n")
            
            for css_file in existing_sources:
                log(f"  Including: {css_file.name}", 'INFO')
                output.write(f"/* === {css_file.name} === */\n")
                try:
                    with open(css_file, 'r', encoding='utf-8') as input_file:
                        content = input_file.read().strip()
                        if content:
                            output.write(content)
                            output.write("\n\n")
                except Exception as e:
                    log(f"Warning: Could not read {css_file.name}: {e}", 'WARNING')
        
        log(f"Combined CSS file: {combined_css.stat().st_size} bytes")
        
        # Compress the combined file
        with open(combined_css, 'rb') as f_input:
            with gzip.open(final_gz, 'wb', compresslevel=GZIP_LEVEL) as f_output:
                shutil.copyfileobj(f_input, f_output)
        
        log(f"CSS compressed: {final_gz.stat().st_size} bytes")
        combined_css.unlink()  # Clean up intermediate file
        
        # Register SCons dependency if in build context
        if IN_SCONS_CONTEXT:
            SCONS_ENV.Depends(str(final_gz), [str(f) for f in existing_sources])
            log(f"Registered SCons dependencies for {final_gz.name}", 'INFO')
        
        return True
        
    except Exception as e:
        log(f"Failed to combine/compress CSS: {e}", 'ERROR')
        return False

def compress_direct_files():
    """
    Compress direct files (no combining).
    
    Files:
        - ../SharedLib/WebAssets/Data/help.json -> assets/help.json.gz
        - ../SharedLib/WebAssets/HTML/swaggerui.html -> assets/swaggerui.html.gz
        - ../SharedLib/WebAssets/Images/favicon.ico -> assets/favicon.ico.gz
        - ../SharedLib/WebAssets/Images/favicon.svg -> assets/favicon.svg.gz
    """
    assets_dir = get_assets_dir()
    shared_dir = get_shared_dir()
    
    direct_files = [
        (shared_dir / "Data" / "help.json", assets_dir / "help.json.gz", "help.json"),
        (shared_dir / "HTML" / "swaggerui.html", assets_dir / "swaggerui.html.gz", "swaggerui.html"),
        (shared_dir / "Images" / "favicon.ico", assets_dir / "favicon.ico.gz", "favicon.ico"),
        (shared_dir / "Images" / "favicon.svg", assets_dir / "favicon.svg.gz", "favicon.svg"),
    ]
    
    results = []
    for source, output, description in direct_files:
        compressed = compress_file(source, output, description)
        results.append(compressed)
        
        # Register SCons dependency if in build context
        if IN_SCONS_CONTEXT and compressed:
            SCONS_ENV.Depends(str(output), str(source))
    
    if any(results):
        log("Direct Files: updated")
        return True
    else:
        log("Direct Files: up-to-date")
        return False

def compress_project_files():
    """
    Compress project-specific files in the assets directory.
    
    These are files that exist in the project's assets/ folder and need
    to be compressed for web serving (HTML, JS, etc.).
    
    Files compressed:
        - *.html files
        - *.svg files
        - *.ico files
        - index.js, three.min.js (explicitly)
    """
    assets_dir = get_assets_dir()
    
    if not assets_dir.exists():
        log("Assets directory not found, skipping project file compression", 'WARNING')
        return False
    
    # Define extensions that should be compressed
    compress_extensions = {'.html', '.svg', '.ico', '.stl'}
    compress_files = {'index.js', 'three.min.js'}
    
    results = []
    for file_path in assets_dir.iterdir():
        if not file_path.is_file():
            continue
        
        # Skip if already a .gz file
        if file_path.suffix == '.gz':
            continue
        
        # Check if this file should be compressed
        should_compress = (
            file_path.suffix.lower() in compress_extensions or
            file_path.name in compress_files
        )
        
        if not should_compress:
            continue
        
        output_path = file_path.with_suffix(file_path.suffix + '.gz')
        
        # Check if compression is needed
        needs_compression = (
            not output_path.exists() or
            file_path.stat().st_mtime > output_path.stat().st_mtime
        )
        
        if needs_compression:
            try:
                log(f"Compressing: {file_path.name} -> {output_path.name}")
                with open(file_path, 'rb') as f_input:
                    with gzip.open(output_path, 'wb', compresslevel=GZIP_LEVEL) as f_output:
                        shutil.copyfileobj(f_input, f_output)
                log(f"  Compressed: {output_path.stat().st_size} bytes", 'INFO')
                results.append(True)
                
                # Register SCons dependency if in build context
                if IN_SCONS_CONTEXT:
                    SCONS_ENV.Depends(str(output_path), str(file_path))
            except Exception as e:
                log(f"Failed to compress {file_path.name}: {e}", 'ERROR')
                results.append(False)
        else:
            log(f"Up-to-date: {output_path.name}", 'INFO')
    
    if any(results):
        log("Project Files: updated")
        return True
    else:
        log("Project Files: up-to-date")
        return False

def clear_build_cache():
    """
    Clear PlatformIO build cache if .gz files changed.
    
    This forces a fresh link phase, ensuring new symbols are picked up.
    The cache is at: .pio/buildcache/
    """
    cache_dir = Path(".pio/buildcache")
    
    if not cache_dir.exists():
        return
    
    try:
        log(f"Clearing build cache: {cache_dir.absolute()}")
        shutil.rmtree(cache_dir)
        cache_dir.mkdir(parents=True, exist_ok=True)
        log("Build cache cleared")
    except Exception as e:
        log(f"Warning: Could not clear build cache: {e}", 'WARNING')

# Main execution
def main():
    """Execute all asset preparation steps."""
    log("=" * 60)
    log("Starting Asset Preparation")
    log(f"Target Project Directory: {Path.cwd().absolute()}")
    log(f"Project Assets Directory: {get_assets_dir().absolute()}")
    log(f" Shared Assets Directory: {get_shared_dir().absolute()}")
    log(f"            Verbose Mode: {'ON' if VERBOSE else 'OFF'}")
    log("=" * 60)
    
    # Ensure assets directory exists
    ensure_assets_dir()
    
    # Execute preparation steps (order matters: standalone files first, then combined files overwrite them)
    results = []
    results.append(("Project Files", compress_project_files()))
    results.append((" Direct Files", compress_direct_files()))
    results.append(("   JavaScript", combine_and_compress_javascript()))
    results.append(("   StyleSheet", combine_and_compress_css()))
    
    # If anything changed, clear build cache
    if any(result[1] for result in results):
        clear_build_cache()
    
    # Summary
    log("=" * 60)
    for step, changed in results:
        status = "UPDATED" if changed else "up-to-date"
        log(f"{step}: {status}")
    log("Asset preparation complete")
    log("=" * 60)

# Run if in SCons context
if IN_SCONS_CONTEXT:
    # Skip during clean target, only run during actual builds
    if not SCONS_ENV.GetOption('clean'):
        main()
else:
    # Standalone execution for testing
    print("Running standalone (not in PlatformIO context)")
    print("Set VERBOSE = True in script for detailed output")
    main()
