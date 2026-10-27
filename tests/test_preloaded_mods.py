#!/usr/bin/env python3
from __future__ import annotations

import sys
import tomllib
from pathlib import Path


EXPECTED = {
    "mmx5.enhancement.skip-alia": {
        "feature": "skip-alia", "plugin": "mmx5.skip-alia", "version": "1.0.0",
    },
    "mmx5.coop": {
        "feature": "coop", "plugin": "mmx5.coop", "version": "0.0.1",
        "default": False, "format": 7,
    },
    "mmx5.cheat.ultimate-armor": {
        "feature": "ultimate-armor", "plugin": "mmx5.cheats.ultimate-armor",
        "version": "1.0.0", "default": False, "group": "Cheats",
        "save_compatibility": "shared",
    },
    "mmx5.cheat.black-zero": {
        "feature": "black-zero", "plugin": "mmx5.cheats.black-zero",
        "version": "1.0.0", "default": False, "group": "Cheats",
        "save_compatibility": "shared",
    },
    "mmx5.diagnostics": {
        "feature": "inputs", "plugin": "mmx5.diagnostics", "version": "1.0.0",
    },
    "mmx5.enhancement.widescreen": {
        "feature": "widescreen",
        "plugin": "mmx5.widescreen",
        "version": "1.1.0",
    },
    "mmx5.enhancement.resident-loading": {
        "feature": "resident-loading",
        "plugin": "mmx5.resident-loading",
        "version": "1.0.0",
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
        require([path.name for path in versions] == [expected["version"]],
                f"{package_id}: expected only version {expected['version']}")
        manifest_path = versions[0] / "manifest.toml"
        with manifest_path.open("rb") as stream:
            manifest = tomllib.load(stream)

        require(manifest["format_version"] == expected.get("format",5), f"{package_id}: format")
        require(manifest["id"] == package_id, f"{package_id}: id")
        require(manifest["version"] == expected["version"], f"{package_id}: version")
        require(manifest["resolver"] == "declarative", f"{package_id}: resolver")
        if "save_compatibility" in expected:
            require(manifest["save_compatibility"] == expected["save_compatibility"],
                    f"{package_id}: native save compatibility")

        targets = manifest.get("target", [])
        require(len(targets) == 1, f"{package_id}: target count")
        require(targets[0]["game_id"] == "SLUS-01334", f"{package_id}: game id")
        require(targets[0]["disc_sha256"] == DISC_SHA256,
                f"{package_id}: disc hash")

        features = manifest.get("feature", [])
        require(len(features) == 1, f"{package_id}: feature count")
        require(features[0]["id"] == expected["feature"],
                f"{package_id}: feature id")
        require(features[0]["default_enabled"] is expected.get("default", True),
                f"{package_id}: incorrect feature default")
        if "group" in expected:
            require(features[0]["group"] == expected["group"],
                    f"{package_id}: feature group")

        plugins = manifest.get("plugin", [])
        require(len(plugins) == 1, f"{package_id}: plugin count")
        require(plugins[0] == {
            "feature": expected["feature"],
            "id": expected["plugin"],
        }, f"{package_id}: plugin binding")

        if expected["feature"] == "widescreen":
            aspect = next(o for o in manifest["option"] if o["id"] == "aspect")
            require(aspect["default"] == "adaptive", "custom renderer must default to adaptive")
            require({c["value"] for c in aspect["choice"]} == {"adaptive", "16:9", "21:9", "32:9"},
                    "custom renderer aspect choices")
        if package_id == "mmx5.coop":
            require(manifest.get('requirement') == [dict(feature='coop',
                    package='mmx5.enhancement.widescreen',requires_feature='widescreen',
                    when={'local_cameras':'split'})],
                    'local split must derive the authored renderer')
            for option_id in ("ultimate_armor_x", "black_zero"):
                options = [o for o in manifest["option"] if o["id"] == option_id]
                require(len(options) == 1, f"co-op: unique {option_id} option")
                require(options[0]["feature"] == "coop" and
                        options[0]["type"] == "boolean" and
                        options[0]["default"] == "false",
                        f"co-op: {option_id} must be an opt-in boolean")

    print("validated X5 renderer/loading/diagnostics/Alia defaults, opt-in co-op and native forms; interpolation absent")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
