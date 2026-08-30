/**
 * @file ImGuiOverlayRenderSystem.ixx
 * @brief Runtime system that triggers rendering of an ImGui overlay each update.
 */
module;

export module helios.imgui.systems.ImGuiOverlayRenderSystem;


import helios.ecs.EcsWorld;


import helios.imgui.ImGuiOverlay;



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

        using EcsWorld = ecs::EcsWorld;

        /**
         * @brief Referenced overlay rendered by this system.
         */
        ImGuiOverlay& overlay_;

    public:


        /**
         * @brief Creates a render system bound to a specific ImGui overlay.
         * @param overlay Overlay instance to render on update.
         */
        explicit ImGuiOverlayRenderSystem(ImGuiOverlay& overlay) : overlay_(overlay) {}

        /**
         * @brief Executes one system update and renders the bound overlay.
         * @param ecsWorld The ECS world (unused in this system).
         * @return true if the update was successful.
         */
        void update(EcsWorld& ecsWorld) noexcept {
            (void)ecsWorld;
            overlay_.render();
        };

    };

}