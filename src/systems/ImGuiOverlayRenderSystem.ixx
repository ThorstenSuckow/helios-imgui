/**
 * @file ImGuiOverlayRenderSystem.ixx
 * @brief Runtime system that triggers rendering of an ImGui overlay each update.
 */
module;

#include <cassert>

export module helios.imgui.systems.ImGuiOverlayRenderSystem;

import helios.engine.runtime.world.tags.SystemRole;
import helios.engine.runtime.world.UpdateContext;

import helios.imgui.ImGuiOverlay;

using namespace helios::engine::runtime::world::tags;
using namespace helios::engine::runtime::world;
using namespace helios::imgui;
export namespace helios::imgui::systems {

    /**
     * @class ImGuiOverlayRenderSystem
     * @brief Adapts an `ImGuiOverlay` to the engine runtime system update step.
     *
     * The system keeps a reference to an overlay instance and invokes
     * `ImGuiOverlay::render()` on each update.
     */
    class ImGuiOverlayRenderSystem {

        /**
         * @brief Referenced overlay rendered by this system.
         */
        ImGuiOverlay& overlay_;

    public:

        /**
         * @brief Runtime role tag used by the engine system registry.
         */
        using EngineRoleTag = SystemRole;


        /**
         * @brief Creates a render system bound to a specific ImGui overlay.
         * @param overlay Overlay instance to render on update.
         */
        explicit ImGuiOverlayRenderSystem(ImGuiOverlay& overlay) : overlay_(overlay) {}

        /**
         * @brief Executes one system update and renders the bound overlay.
         * @param updateContext Per-frame update context provided by the runtime.
         */
        void update(UpdateContext& updateContext) noexcept {
            overlay_.render();
        };

    };

}