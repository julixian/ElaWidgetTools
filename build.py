"""Configure, build and install ElaWidgetTools with the local Qt SDK."""

import argparse
import shutil
import subprocess
import tomllib
from pathlib import Path


ROOT = Path(__file__).resolve().parent
BUILD_DIR = ROOT / "out" / "build"
INSTALL_DIR = ROOT / "Install"


def qt_root() -> Path:
    with (ROOT / "build-local.toml").open("rb") as config:
        qt = Path(tomllib.load(config)["windows"]["qt_root"])
    if not any((qt / "lib" / "cmake" / version).is_dir()
               for version in ("Qt6", "Qt5")):
        raise RuntimeError(f"Qt SDK is missing or incomplete: {qt}")
    return qt


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--no-example", action="store_true", help="skip ElaWidgetToolsExample"
    )
    args = parser.parse_args()

    if shutil.which("cmake") is None:
        raise RuntimeError("cmake was not found in PATH")
    qt = qt_root()
    subprocess.run(
        [
            "cmake", "-S", str(ROOT), "-B", str(BUILD_DIR),
            f"-DQT_SDK_DIR={qt}",
            f"-DCMAKE_INSTALL_PREFIX={INSTALL_DIR}",
            f"-DELAWIDGETTOOLS_BUILD_EXAMPLE={'OFF' if args.no_example else 'ON'}",
        ],
        check=True,
    )
    subprocess.run(
        ["cmake", "--build", str(BUILD_DIR), "--config", "Release",
         "--target", "install", "--parallel"],
        check=True,
    )


if __name__ == "__main__":
    main()
