# helios::imgui

Dear ImGui integration for the helios engine modules.

## Overview

`helios::imgui` provides a debug/developer UI layer on top of Dear ImGui. It
combines a platform-agnostic backend interface, a GLFW+OpenGL backend,
overlay/widget orchestration, log forwarding, and ready-to-use widgets for
runtime diagnostics.

## Features

- Platform-agnostic `ImGuiBackend` abstraction
- GLFW+OpenGL backend for Dear ImGui frame setup and draw-data submission
- Central `ImGuiOverlay` for rendering multiple widgets per frame
- Engine render-system adapter for invoking the overlay from the runtime update loop
- Docking-enabled overlay support
- Base `ImGuiWidget` interface for custom developer tools
- Log sink bridge from the helios logging system to an ImGui console
- Built-in FPS, log-console, main-menu, and camera widgets

## Module surface

| Area | Public modules / APIs |
|------|------------------------|
| Backend abstraction | `ImGuiBackend` |
| GLFW/OpenGL backend | `ImGuiGlfwOpenGLBackend` |
| Overlay orchestration | `ImGuiOverlay` |
| Runtime systems | `helios.imgui.systems`, `systems::ImGuiOverlayRenderSystem` |
| Widget interface | `ImGuiWidget` |
| Logging integration | `ImGuiLogSink`, `widgets::LogWidget` |
| Widgets | `helios.imgui.widgets`, `FpsWidget`, `LogWidget`, `MainMenuWidget` |
| Aggregator | `helios.imgui` |

## Usage

### C++ module

```cpp
import helios.imgui;
```

### Overlay architecture

`ImGuiBackend` defines the frame lifecycle used by the overlay. Concrete
implementations call Dear ImGui platform/renderer backends in `newFrame()` and
submit `ImDrawData` in `renderDrawData()`.

`ImGuiGlfwOpenGLBackend` initializes a Dear ImGui context for a helios GLFW
window entity and enables keyboard navigation and docking. It resolves the
native GLFW window through `GLFWWindowHandleComponent` in the platform world.

`ImGuiOverlay::forBackend()` returns the singleton overlay for a backend.
Widgets are registered with `addWidget()` and are drawn in registration order;
the overlay does not take ownership, so widget instances must outlive their
registration.

`systems::ImGuiOverlayRenderSystem` adapts an `ImGuiOverlay` to the engine runtime
system interface and calls `ImGuiOverlay::render()` during its update.

Built-in widgets include:

- `widgets::FpsWidget` for FPS metrics, frame-time history, and frame-pacer control
- `widgets::LogWidget` for scope-aware, filterable log output
- `widgets::MainMenuWidget` for docking, transparency, style, and debug-menu controls

`ImGuiLogSink` forwards helios log messages into `widgets::LogWidget`.

### CMake

Use as a local sibling/subdirectory module:

```cmake
add_subdirectory(path/to/helios-imgui)
target_link_libraries(your_target PRIVATE helios::imgui)
```

Build locally:

```bash
cmake -S . -B build
cmake --build build
```

When installed/package-export support is enabled, consume from another project:

```cmake
find_package(helios-ecs CONFIG REQUIRED)
find_package(helios-engine CONFIG REQUIRED)
find_package(helios-glfw CONFIG REQUIRED)
find_package(helios-imgui CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE helios::imgui)
```

Configure a consumer against an installed prefix:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="/path/to/helios-prefix"
cmake --build build
```

Package export generation is controlled by `HELIOS_IMGUI_ENABLE_PACKAGE` and is
disabled by default for local sibling-repository development.

## Development

Run the regular CMake build from the repository root:

```bash
cmake -S . -B build
cmake --build build
```

All built-in widgets, including `widgets::CameraWidget`, are compiled in the
regular default build.

## Related repositories

- [`helios-ecs`](https://github.com/thorstensuckow/helios-ecs)
- [`helios-engine`](https://github.com/thorstensuckow/helios-engine)
- [`helios-math`](https://github.com/thorstensuckow/helios-math)
- [`helios-opengl`](https://github.com/thorstensuckow/helios-opengl)
- [`helios-glfw`](https://github.com/thorstensuckow/helios-glfw)

