/**
 * @file ImGuiOverlayRenderSystem.ixx
 * @brief Runtime system that triggers rendering of an ImGui overlay each update.
 */
module;

export module helios.imgui.systems.ImGuiOverlayRenderSystem;

import helios.ecs.system.tags;
import helios.engine.runtime.world.UpdateContext;
import helios.engine.runtime.world.types;
import helios.engine.runtime.concepts;

import helios.imgui.ImGuiOverlay;


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
        using EcsRoleTag = ecs::system::tags::TypedSystemRole;

        /**
         * @brief Creates a render system bound to a specific ImGui overlay.
         * @param overlay Overlay instance to render on update.
         */
        explicit ImGuiOverlayRenderSystem(ImGuiOverlay& overlay) : overlay_(overlay) {}

        /**
         * @brief Executes one system update and renders the bound overlay.
         * @param updateContext Per-frame update context provided by the runtime.
         * @return true if the update was successful.
         */
        void update(UpdateContext& updateContext) noexcept {
            (void)updateContext;
            overlay_.render();
        };

    };

}