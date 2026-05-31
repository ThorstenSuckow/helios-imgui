/**
 * @file ImGuiBackend.ixx
 * @brief Abstract backend interface for ImGui integration with rendering/windowing systems.
 */
module;

#include "imgui.h"


export module helios.imgui.ImGuiBackend;


export namespace helios::imgui {

    /**
     * @class ImGuiBackend
     * @brief Platform-agnostic interface for ImGui backend implementations.
     *
     * Concrete backends (e.g., GLFW+OpenGL, SDL+Vulkan) implement this interface
     * to provide platform-specific initialization, frame setup, and rendering.
     */
    class ImGuiBackend {

    public:

        /**
         * @brief Default constructor.
         */
        ImGuiBackend() = default;

        /**
         * @brief Virtual destructor for polymorphic backend cleanup.
         */
        virtual ~ImGuiBackend() = default;

        /**
         * @brief Prepares ImGui for a new frame.
         *
         * Must be called at the beginning of each frame before any ImGui calls.
         * Typically invokes platform-specific `NewFrame()` implementations.
         *
         * @return `true` if frame setup succeeded; otherwise `false`.
         */
        virtual bool newFrame() = 0;

        /**
         * @brief Renders ImGui draw data to the screen.
         *
         * Called after `ImGui::Render()` to submit draw commands to the GPU.
         *
         * @param drawData Pointer to ImGui draw data (typically from `ImGui::GetDrawData()`).
         *                 Implementations may treat `nullptr` as a no-op.
         */
        virtual void renderDrawData(ImDrawData* drawData) = 0;

    };
}
