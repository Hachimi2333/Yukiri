# Yukiri development conventions

## Product and architecture

- Yukiri is a C++23 retained-mode desktop GUI static library. The current delivery is 0.1.0 on Windows 10 22H2 / Windows 11 x64 with MSVC and MinGW.
- Default windows and the demo to custom decorations; retain explicit native decoration support and test both.
- Keep public interfaces independent of SDL, Vulkan, FreeType, and HarfBuzz types. Platform details belong in platform adapters; Vulkan details belong in the renderer.
- Keep each file responsible for one coherent concern. Do not split files just to satisfy a line count, or combine unrelated subsystems.
- A parent exclusively owns its children through `std::unique_ptr`. Observers do not own widgets. Defer destruction during input, layout, and paint dispatch. UI mutation and rendering run on the application thread.
- Use RAII, the rule of zero where practical, explicit ownership, and the C++ Core Guidelines. Fallible initialization and resource operations return `std::expected<..., Error>`.
- Confirm new important public names with the user. Approved names include Yukiri, Application, Window, Widget, Canvas, Theme, ApplicationConfig, WindowConfig, WindowDecoration, ThemeMode, Signal, Connection, Error, ErrorCode, MotionSettings, OnAnimate, RequestAnimationFrame, SetSmoothScrolling, IsFocusVisible, and GetFocusBounds.

## Source style

- Use actual tabs for indentation in project code and build scripts, with a display width of four. Use Allman braces for multiline C++ definitions; trivial accessors may be inline.
- Types, functions, enum values, and constants use PascalCase. Members use `m_Name`; parameters and local variables use camelCase. Source files use PascalCase.hpp/.cpp.
- Code comments and commit messages are English. User and developer documentation is primarily Chinese.
- Prefer standard C++23 facilities when they improve clarity; do not introduce features solely for novelty. Public headers must be self-contained.
- Do not rewrite vendored source formatting. Third-party libraries are pinned Git submodules, built from source, and linked statically.

## Build and verification

- Use CMake >= 3.28 and Ninja. Keep MSVC / MinGW and Debug / Release build trees separate.
- Use the local build, GPU smoke, and package verification tools. GitHub Actions is intentionally not configured.
- Apply strict warnings only to Yukiri code, not dependencies. Do not propagate project warning flags into consumers.
- Keep presentation synchronized with FIFO and per-window refresh deadlines; never add a fixed 16ms wait after presentation. Avoid queue-idle waits in interactive uploads.
- Keep logical input focus separate from keyboard focus visibility. Pointer presses hide focus indicators; Tab navigation reveals them. Custom focus bounds use widget-local coordinates and must avoid content overlap.
- Animate visual state using elapsed time; update logical values and emit notifications immediately. Finished or hidden motion must not keep scheduling frames.
- Run meaningful Catch2 / CTest coverage for changed behavior. GPU and desktop interaction checks are separate from headless tests.
- Never claim runtime checks from compilation alone. Record compiler, OS, GPU, validation-layer and manual interaction results in docs/Validation.md.
- CMake packages target Ninja single-configuration builds; keep compiler families, architectures, and Debug / Release installations isolated.
- Validate both add_subdirectory and relocated find_package consumption before release.

## Commits and documentation

- Commit each independently completed part using Conventional Commits. Every commit needs an English body explaining the change, motivation, and validation.
- Update the relevant docs in the same commit. Keep docs/Status.md and FUTURE.md accurate; distinguish implemented, validated, and pending work.
- Do not publish or push unless requested.
