# helios::imgui::systems

Systems that integrate ImGui overlay rendering into the engine runtime update flow.

## Systems

| System | Purpose |
|--------|---------|
| `ImGuiOverlayRenderSystem` | Invokes `ImGuiOverlay::render()` once per runtime update |

---
<details>
<summary>Doxygen</summary><p>
@namespace helios::imgui::systems
@brief Runtime systems for ImGui overlay rendering.
@details This namespace currently contains the render system adapter that bridges `ImGuiOverlay` into the engine system update loop via `SystemRole`.
</p></details>

