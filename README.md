# LABRYS

Suite d'instruments virtuels (VST3 / AU / Standalone) de Raw Powa Studio.

| Instrument | Description | État |
|---|---|---|
| DÉDALE | synthé génératif à deux chemins parallèles (wavefolder + filtre) | en développement |
| TALOS | synthé de percussion | prévu |
| ICARE | synthé polyrythmique à sous-harmoniques | prévu |
| EUROPE | synthé semi-modulaire au clavier | prévu |
| PHÈDRE | petit synthé semi-modulaire mono | prévu |

## Build (macOS)
Prérequis : Command Line Tools Xcode, CMake ≥ 3.25, Ninja.

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Presets disponibles : `debug`, `tests` (warnings = erreurs), `asan`, `release` (universal arm64 + x86_64).

Documentation : [docs/](docs/), à commencer par [docs/SPEC_DEDALE.md](docs/SPEC_DEDALE.md).
