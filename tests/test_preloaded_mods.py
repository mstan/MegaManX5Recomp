#!/usr/bin/env python3
from __future__ import annotations

import sys
import tomllib
from pathlib import Path


EXPECTED = {
    "mmx5.enhancement.widescreen": {
        "feature": "widescreen",
        "plugin": "mmx5.widescreen",
    },
    "mmx5.enhancement.frame-interpolation": {
        "feature": "frame-interpolation",
        "plugin": "mmx5.frame-interpolation",
    },
}
DISC_SHA256 = "be731bc4b9d3211b9267a34b8a68c769199a15479b14004ff25b67cdfebe8af4"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    require(len(sys.argv) == 2, "usage: test_preloaded_mods.py <catalog>")
    packages = Path(sys.argv[1]) / "packages"
    actual = {path.name for path in packages.iterdir() if path.is_dir()}
    require(actual == set(EXPECTED), f"package inventory mismatch: {sorted(actual)}")

    for package_id, expected in EXPECTED.items():
        versions = [path for path in (packages / package_id).iterdir() if path.is_dir()]
        require([path.name for path in versions] == ["1.0.0"],
                f"{package_id}: expected only version 1.0.0")
        manifest_path = versions[0] / "manifest.toml"
        with manifest_path.open("rb") as stream:
            manifest = tomllib.load(stream)

        require(manifest["format_version"] == 5, f"{package_id}: format")
        require(manifest["id"] == package_id, f"{package_id}: id")
        require(manifest["version"] == "1.0.0", f"{package_id}: version")
        require(manifest["resolver"] == "declarative", f"{package_id}: resolver")

        targets = manifest.get("target", [])
        require(len(targets) == 1, f"{package_id}: target count")
        require(targets[0]["game_id"] == "SLUS-01334", f"{package_id}: game id")
        require(targets[0]["disc_sha256"] == DISC_SHA256,
                f"{package_id}: disc hash")

        features = manifest.get("feature", [])
        require(len(features) == 1, f"{package_id}: feature count")
        require(features[0]["id"] == expected["feature"],
                f"{package_id}: feature id")
        require(features[0]["default_enabled"] is False,
                f"{package_id}: feature must default off")

        plugins = manifest.get("plugin", [])
        require(len(plugins) == 1, f"{package_id}: plugin count")
        require(plugins[0] == {
            "feature": expected["feature"],
            "id": expected["plugin"],
        }, f"{package_id}: plugin binding")

    interpolation = packages / "mmx5.enhancement.frame-interpolation" / "1.0.0"
    with (interpolation / "manifest.toml").open("rb") as stream:
        frame_manifest = tomllib.load(stream)
    options = frame_manifest.get("option", [])
    require(len(options) == 1, "frame interpolation: option count")
    require(options[0]["default"] == "display", "frame interpolation: default rate")
    require([choice["value"] for choice in options[0]["choice"]] ==
            ["display", "90", "120", "144", "165", "240"],
            "frame interpolation: rate choices")

    print("validated 2 default-disabled MMX5 presentation mods")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
