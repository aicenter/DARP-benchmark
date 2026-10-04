# Dependencies

- The development version of future-config (see [README](README.md), section "Development version of future-config") must be installed from the existing Future-Config repository on the host (`Workspaces/Fido/Future-Config`, a sibling of the `AIC` workspace directory), so that host changes are propagated. Never clone the Future-Config repository to install it.
- vcpkg does not detect changes in the Future-Config repository. To pick up host changes, run `vcpkg remove future-config` and install it again with binary caching disabled (`--binarysource=clear`).
