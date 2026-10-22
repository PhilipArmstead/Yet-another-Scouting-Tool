"""Exercise real per-user paths/import without touching the developer's configuration."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix="yast-options-") as temporary:
    root = Path(temporary)
    binary = root / "bin" / Path(sys.argv[1]).name
    binary.parent.mkdir()
    shutil.copy2(sys.argv[1], binary)
    legacy = binary.parent / "options.yaml"
    home = root / "home-\u00e9"
    home.mkdir()
    env = dict(os.environ, HOME=str(home), XDG_CONFIG_HOME=str(home / "config"),
               XDG_STATE_HOME=str(home / "state"))
    config = (home / "Library/Application Support/YaST/options.yaml" if sys.platform == "darwin"
              else home / "config/yast/options.yaml")
    cwd = root / "unrelated-working-directory"
    cwd.mkdir()

    def probe(success=True, dark=False, reload=False):
        result = subprocess.run([str(binary), "--probe-options"] + (["reload"] if reload else []), cwd=cwd, env=env,
                                capture_output=True, text=True)
        assert (result.returncode == 0) == success, result.stdout + result.stderr
        if success:
            assert f"options_loaded=1 dark={int(dark)}" in result.stdout, result.stdout

    probe()
    defaults = config.read_bytes()
    assert b"InjuryProneness: -11.03" in defaults
    probe(reload=True)
    config.unlink()
    legacy.write_text("dark-mode: true\n", encoding="utf-8")
    probe(dark=True)
    assert config.read_bytes() == legacy.read_bytes()
    config.write_bytes(defaults)
    probe()
    assert config.read_bytes() == defaults
    assert legacy.read_text() == "dark-mode: true\n"
    config.unlink()
    config.mkdir()
    probe(success=False)
    config.rmdir()
    if os.geteuid() != 0:
        legacy.chmod(0)
        try:
            probe(success=False)
            assert not config.exists()
        finally:
            legacy.chmod(0o600)
        config.parent.chmod(0o500)
        try:
            probe(success=False)
            assert not config.exists()
        finally:
            config.parent.chmod(0o700)
    assert legacy.read_text() == "dark-mode: true\n"
print("Per-user defaults, import precedence, Unicode paths and I/O failures passed")
