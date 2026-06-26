#!/bin/bash

# Perform a compile of all/single binaries using their respective architectures.
#
# This script discovers and validates the PlatformIO environment before compilation.
# It allows developers to explicitly specify which Python interpreter and/or pio
# executable to use, solving version compatibility issues (e.g., when the system
# has Python 3.14 but PlatformIO only supports up to 3.13).
#
# The script can:
# - Report the resolved PlatformIO configuration (run with 'env' target)
# - Use a specific PlatformIO executable (PIO_CMD=/path/to/pio)
# - Use a specific Python interpreter with PlatformIO (PIO_PYTHON=/path/to/python)
# - Compile all devices or individual targets
#
# All child compile scripts inherit the chosen PlatformIO configuration, ensuring
# consistent Python/pio usage throughout the build process.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

echo ""

# Current build timestamp and major version to be reflected in the builds for ESP32.
MJVER="${MJVER:=V6}"
TIMESTAMP="${TIMESTAMP:=$(date +"%Y%m%d%H%M%S")}"
ENV_INFO_PAUSE_SECONDS="${ENV_INFO_PAUSE_SECONDS:=5}"

TARGET="all"
ENV_OVERRIDES=()
PIO_MODE=""
PIO_RESOLVED_COMMAND=""
PIO_SYSTEM_INFO=""
PIO_ACTUAL_PYTHON=""
PIO_ACTUAL_PYTHON_VERSION="unknown"
SHOW_USAGE_BEFORE_ENV=0

usage() {
  cat <<EOF
Usage: ./compile.sh [all|help|env|pack|wand|att|attenuator|blast|blaster|gizmo|stream|pstt|trap|toast|toaster] [NAME=VALUE ...]

Compile Targets:
  all          Run tests and compile every device (default)
  help         Show this help message (-h, --help)
  env          Report the resolved build environment and exit
  pack         Compile Proton Pack
  wand         Compile Neutrona Wand
  att[enuator] Compile Attenuator
  blast[er]    Compile Single-Shot Blaster
  gizmo        Compile Belt Gizmo
  stream       Compile Stream Effects
  pstt         Compile PSTT
  trap         Compile Ghost Trap Base and Cartridge
  toast[er]    Compile Dancing Toaster

Environment Overrides:
  NAME=VALUE pairs are exported for this invocation and inherited by all sourced compile scripts.
  Use PIO_CMD=/path/to/pio to pin a specific PlatformIO executable (eg. from VSCode).
  Use PIO_PYTHON=/path/to/python to run PlatformIO with a specific Python interpreter.

Full Usage Examples:
  ./compile.sh
  ./compile.sh env
  ./compile.sh env PIO_CMD=/Users/me/.local/bin/pio
  ./compile.sh env PIO_PYTHON=/Users/me/.pyenv/versions/3.13.3/bin/python
  ./compile.sh all
  ./compile.sh pack
  ./compile.sh trap PIO_CMD=/opt/homebrew/bin/pio
EOF
}

apply_environment_overrides() {
  local arg

  for arg in "$@"; do
    if [[ "$arg" == *=* ]]; then
      export "$arg"
      ENV_OVERRIDES+=("$arg")
    elif [ "$TARGET" = "all" ]; then
      TARGET="$arg"
    else
      echo "ERROR: Unexpected argument '$arg'"
      usage
      exit 1
    fi
  done
}

resolve_target() {
  case "$TARGET" in
    all|env|pack|wand|gizmo|stream|pstt|trap)
      ;;
    att|attenuator)
      TARGET="attenuator"
      ;;
    blast|blaster)
      TARGET="blaster"
      ;;
    toast|toaster)
      TARGET="toaster"
      ;;
    -h|--help|help)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: Unknown target '$TARGET'"
      usage
      exit 1
      ;;
  esac
}

resolve_platformio() {
  if [ -n "$PIO_PYTHON" ] && [ -n "$PIO_CMD" ]; then
    echo "ERROR: Set either PIO_CMD or PIO_PYTHON, not both"
    exit 1
  fi

  if [ -n "$PIO_PYTHON" ]; then
    if [ ! -x "$PIO_PYTHON" ]; then
      echo "ERROR: PIO_PYTHON is not executable: $PIO_PYTHON"
      exit 1
    fi
    if ! "$PIO_PYTHON" -m platformio --version >/dev/null 2>&1; then
      echo "ERROR: $PIO_PYTHON cannot run 'python -m platformio'"
      exit 1
    fi
    PIO_MODE="python-module"
    PIO_ACTUAL_PYTHON="$PIO_PYTHON"
    return
  fi

  if [ -n "$PIO_CMD" ]; then
    if [ ! -x "$PIO_CMD" ]; then
      echo "ERROR: PIO_CMD is not executable: $PIO_CMD"
      exit 1
    fi
    PIO_RESOLVED_COMMAND="$PIO_CMD"
  else
    PIO_RESOLVED_COMMAND="$(type -P pio 2>/dev/null || true)"

    if [ -z "$PIO_RESOLVED_COMMAND" ]; then
      if [ -x "$HOME/.platformio/penv/bin/python" ]; then
        if "$HOME/.platformio/penv/bin/python" -m platformio --version >/dev/null 2>&1; then
          PIO_MODE="python-module"
          PIO_ACTUAL_PYTHON="$HOME/.platformio/penv/bin/python"
          return
        fi
      fi

      if [ -x "$HOME/.local/bin/pio" ]; then
        PIO_RESOLVED_COMMAND="$HOME/.local/bin/pio"
      fi
    fi
  fi

  if [ -z "$PIO_RESOLVED_COMMAND" ]; then
    echo "ERROR: PlatformIO not found in PATH, ~/.platformio/penv/, or ~/.local/bin/"
    echo ""
    echo "Please ensure PlatformIO is installed, or explicitly set:"
    echo "  PIO_CMD=/path/to/pio"
    echo "  PIO_PYTHON=/path/to/python"
    exit 1
  fi

  PIO_MODE="command"

  local shebang_line
  shebang_line="$(sed -n '1p' "$PIO_RESOLVED_COMMAND" | tr '\0' '\n' || true)"
  if [[ "$shebang_line" == '#!'* ]]; then
    PIO_ACTUAL_PYTHON="${shebang_line#\#!}"
  else
    PIO_ACTUAL_PYTHON="unknown"
  fi
}

pio() {
  if [ "$PIO_MODE" = "python-module" ]; then
    "$PIO_PYTHON" -m platformio "$@"
  else
    command "$PIO_RESOLVED_COMMAND" "$@"
  fi
}

collect_platformio_system_info() {
  PIO_SYSTEM_INFO="$(pio system info 2>&1)"

  if [ "$PIO_MODE" = "command" ]; then
    local system_python
    system_python="$(printf '%s\n' "$PIO_SYSTEM_INFO" | awk -F'  +' '/Python Executable/ {print $NF; exit}')"
    if [ -n "$system_python" ]; then
      PIO_ACTUAL_PYTHON="$system_python"
    fi
  fi

  if [ -n "$PIO_ACTUAL_PYTHON" ] && [ "$PIO_ACTUAL_PYTHON" != "unknown" ] && [ -x "$PIO_ACTUAL_PYTHON" ]; then
    PIO_ACTUAL_PYTHON_VERSION="$("$PIO_ACTUAL_PYTHON" --version 2>&1)"
  fi
}

report_environment() {
  echo "============================================================"
  echo "Build Environment Information  -  Compile Target: $TARGET"
  echo "============================================================"
  echo "Script Directory: $SCRIPT_DIR"

  if [ ${#ENV_OVERRIDES[@]} -gt 0 ]; then
    echo "Environment Overrides: ${ENV_OVERRIDES[*]}"
  fi

  if [ "$PIO_MODE" = "python-module" ]; then
    echo "PlatformIO Python: $PIO_PYTHON"
    echo "PlatformIO Python Version: $PIO_ACTUAL_PYTHON_VERSION"
    echo "PlatformIO Command: $PIO_PYTHON -m platformio"
    echo "PlatformIO Invocation: using pio-specified Python to run PlatformIO as a module"
  else
    echo "PlatformIO Command: $PIO_RESOLVED_COMMAND"
    echo "PlatformIO Launcher Python: $PIO_ACTUAL_PYTHON"
    echo "PlatformIO Launcher Python Version: $PIO_ACTUAL_PYTHON_VERSION"
    echo "PlatformIO Invocation: using pio launcher directly by user choice or from PATH"
  fi

  if command -v python3 >/dev/null 2>&1; then
    echo "Shell python3: $(command -v python3)"
    echo "Shell python3 version: $(python3 --version 2>&1)"
  else
    echo "Shell python3: not found"
  fi

  if [ -n "$PATH" ]; then
    echo "PATH: $PATH"
  fi

  echo ""
  echo "PlatformIO System Info:"
  printf '%s\n' "$PIO_SYSTEM_INFO"
  echo "============================================================"
  echo ""
}

run_script() {
  local script_name="$1"
  source "$SCRIPT_DIR/$script_name"
  echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
}

compile_all() {
  run_script "run_tests.sh"
  run_script "compile_pack.sh"
  run_script "compile_wand.sh"
  run_script "compile_attenuator.sh"
  run_script "compile_blaster.sh"
  run_script "compile_gizmo.sh"
  run_script "compile_stream.sh"
  run_script "compile_pstt.sh"
  run_script "compile_trap_base.sh"
  run_script "compile_trap_cartridge.sh"
  run_script "compile_toaster.sh"
}

compile_target() {
  case "$TARGET" in
    all)
      compile_all
      ;;
    env)
      return 0
      ;;
    pack)
      run_script "run_tests.sh"
      run_script "compile_pack.sh"
      ;;
    wand)
      run_script "run_tests.sh"
      run_script "compile_wand.sh"
      ;;
    attenuator)
      run_script "run_tests.sh"
      run_script "compile_attenuator.sh"
      ;;
    blaster)
      run_script "run_tests.sh"
      run_script "compile_blaster.sh"
      ;;
    gizmo)
      run_script "run_tests.sh"
      run_script "compile_gizmo.sh"
      ;;
    stream)
      run_script "run_tests.sh"
      run_script "compile_stream.sh"
      ;;
    pstt)
      run_script "run_tests.sh"
      run_script "compile_pstt.sh"
      ;;
    trap)
      run_script "run_tests.sh"
      run_script "compile_trap_base.sh"
      run_script "compile_trap_cartridge.sh"
      ;;
    toaster)
      run_script "run_tests.sh"
      run_script "compile_toaster.sh"
      ;;
  esac
}

apply_environment_overrides "$@"
resolve_target
resolve_platformio
collect_platformio_system_info
report_environment

if [ "$TARGET" = "env" ]; then
  exit 0
fi

if [ "$ENV_INFO_PAUSE_SECONDS" -gt 0 ] 2>/dev/null; then
  echo "Pausing for $ENV_INFO_PAUSE_SECONDS seconds to allow review of environment information..."
  sleep "$ENV_INFO_PAUSE_SECONDS"
fi

compile_target
