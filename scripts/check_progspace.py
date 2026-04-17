"""
PROGMEM Budget Check -- Fails build if INDEX_HTML exceeds threshold.
Threshold: 40 KB (PROGMEM_BUDGET_THRESHOLD)

This script runs as a PlatformIO post-build action for the ESP8266 target.
It checks the compiled size of the INDEX_HTML PROGMEM symbol against a
defined budget. If the HTML/CSS/JS embedded in pages.h grows beyond the
threshold, the build fails with a clear error message.

Size detection strategy (in order of reliability):
  1. Parse the linker map file for the INDEX_HTML symbol
  2. Fallback: use nm --size-sort on the ELF binary
  3. Fallback: use readelf -s on the ELF binary
  4. If none work, warn but do not fail (no false-negative build breakage)
"""
import os
import sys

PROGMEM_BUDGET_THRESHOLD = 40960  # 40 KB


def check_progspace(source, target, env):
    """Post-build action: check INDEX_HTML size against budget."""
    build_dir = env.get("BUILD_DIR")
    progname = env.get("PROGNAME", "esp12e")

    # Strategy 1: Parse the linker map file for INDEX_HTML
    map_file = os.path.join(build_dir, progname + ".map")
    size = _size_from_mapfile(map_file)

    # Strategy 2: Use nm --size-sort on the ELF
    if size is None:
        elf_file = str(target[0])
        size = _size_from_nm(elf_file)

    # Strategy 3: Use readelf -s on the ELF
    if size is None:
        elf_file = str(target[0])
        size = _size_from_readelf(elf_file)

    # Report results
    if size is not None:
        size_kb = size / 1024.0
        threshold_kb = PROGMEM_BUDGET_THRESHOLD / 1024.0
        print(
            "[PROGMEM Check] INDEX_HTML: {:.0f} bytes ({:.1f} KB) / {:.0f} KB threshold".format(
                size, size_kb, threshold_kb
            )
        )

        if size > PROGMEM_BUDGET_THRESHOLD:
            print(
                "[PROGMEM Check] FAILED: INDEX_HTML exceeds {:.0f} KB budget!".format(
                    threshold_kb
                )
            )
            print(
                "[PROGMEM Check] Reduce HTML/CSS/JS in pages.h or increase threshold."
            )
            env.Exit(1)
        else:
            headroom = (1.0 - size / PROGMEM_BUDGET_THRESHOLD) * 100
            print(
                "[PROGMEM Check] OK: {:.0f}% headroom remaining".format(headroom)
            )
    else:
        print(
            "[PROGMEM Check] WARNING: Could not determine INDEX_HTML size from build artifacts"
        )
        print("[PROGMEM Check] Verify manually via boot serial log")


def _size_from_mapfile(map_file):
    """Parse linker map file for INDEX_HTML symbol size."""
    if not os.path.exists(map_file):
        return None

    try:
        with open(map_file, "r") as f:
            for line in f:
                if "INDEX_HTML" in line:
                    parts = line.strip().split()
                    for i, p in enumerate(parts):
                        if p == "INDEX_HTML" and i > 0:
                            try:
                                return int(parts[i - 1], 16)
                            except ValueError:
                                continue
    except OSError:
        pass

    return None


def _size_from_nm(elf_file):
    """Use nm --size-sort to find INDEX_HTML symbol size."""
    try:
        import subprocess

        result = subprocess.run(
            ["nm", "--size-sort", elf_file],
            capture_output=True,
            text=True,
        )
        for line in result.stdout.strip().split("\n"):
            if "INDEX_HTML" in line:
                parts = line.strip().split()
                if len(parts) >= 2:
                    try:
                        return int(parts[1], 16)
                    except ValueError:
                        pass
                break
    except (FileNotFoundError, OSError):
        pass

    return None


def _size_from_readelf(elf_file):
    """Use readelf -s to find INDEX_HTML symbol size."""
    try:
        import subprocess

        result = subprocess.run(
            ["readelf", "-s", elf_file],
            capture_output=True,
            text=True,
        )
        for line in result.stdout.strip().split("\n"):
            if "INDEX_HTML" in line:
                parts = line.strip().split()
                if len(parts) >= 3:
                    try:
                        return int(parts[2])
                    except ValueError:
                        pass
                break
    except (FileNotFoundError, OSError):
        pass

    return None


# Register as post-build action on the ELF output
Import("env")
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", check_progspace)
