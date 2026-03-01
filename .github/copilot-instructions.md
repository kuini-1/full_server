# Copilot Instructions

## Project Guidelines
- Project is Linux-only: prefer CMakeLists.txt cleaned of Windows/MSVC-specific branches and use only CMakePresets.json for configuration.
- For quick testing without committing to Git, use Windows Visual Studio with WSL. Mirror the Windows workspace to WSL and run CMake there.