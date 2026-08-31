/**
 * @file ImGuiGlfwOpenGLBackend.ixx
 * @brief GLFW+OpenGL backend implementation for ImGui rendering.
 */
module;

#include <cassert>
#include <stdexcept>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

export module helios.imgui.ImGuiGlfwOpenGLBackend;

import helios.imgui.ImGuiBackend;

import helios.ecs.entity.EntityWorld;
import helios.glfw.components.GLFWWindowHandleComponent;

using namespace helios::glfw::components;
export namespace helios::imgui {

    /**
     * @class ImGuiGlfwOpenGLBackend
     * @brief ImGui backend for GLFW windowing and OpenGL 4.1 rendering.
     *
     * Initializes ImGui context, GLFW platform layer, and OpenGL renderer.
     * This backend is non-copyable and non-movable due to resource ownership semantics.
     *
     * @note Only one instance should exist per application. Creating multiple instances
     * will throw a `std::runtime_error`.
     */
    template<typename TWindowHandle>
    class ImGuiGlfwOpenGLBackend : public ImGuiBackend {

        using WindowHandle = TWindowHandle;

    private:

        /**
         * @brief Indicates whether the backend has been successfully initialized.
         *
         * Tracks the initialization state of the ImGui backend to prevent multiple
         * redundant initialization attempts and manage proper shutdown procedures.
         *
         * @note Modified during initialization and shutdown processes.
         */
        bool initialized_ = false;

        /**
         * @brief Shuts down the ImGui GLFW and OpenGL backend.
         *
         * Ensures proper cleanup of the ImGui context, OpenGL, and GLFW resources.
         * After calling this method, the backend will no longer be initialized, and
         * associated resources will be released.
         *
         * This function is safe to call multiple times but has no effect if the backend
         * is not currently initialized.
         *
         * @note This method is noexcept and will not throw exceptions.
         */
        void shutdown() noexcept {
            if (initialized_) {
                ImGui_ImplOpenGL3_Shutdown();
                ImGui_ImplGlfw_Shutdown();
                ImGui::DestroyContext();
                initialized_ = false;
            }
        }

        /**
         * @brief Window entity handle used to resolve the native GLFW window.
         */
        WindowHandle windowHandle_;

        /**
         * @brief Platform world that stores the GLFW window-handle component.
         */
        ecs::entity::EntityWorld& ecsWorld_;

        /**
         * @brief Performs one-time backend initialization.
         *
         * Resolves the GLFW window entity, initializes the ImGui GLFW binding,
         * and initializes the OpenGL renderer backend.
         *
         * @return `true` if the backend is initialized and ready; otherwise `false`.
         */
        bool initialize() noexcept {

            if (initialized_) {
                return true;
            }
            auto glfwWindow = ecsWorld_.find(windowHandle_);

            if (!glfwWindow) {
                assert(false && "Expected a valid GLFW window entity");
                return false;
            }

            auto glfwComp = glfwWindow->template get<GLFWWindowHandleComponent<WindowHandle>>();

            if (!glfwComp) {
                return false;
            }

            if (!ImGui_ImplGlfw_InitForOpenGL(glfwComp->handle, true)) {
                ImGui::DestroyContext();
                assert(false && "Failed to initialize GLFW backend for ImGui");
                return false;
            }

            if (!ImGui_ImplOpenGL3_Init("#version 410")) {
                ImGui_ImplGlfw_Shutdown();
                ImGui::DestroyContext();
                assert(false && "Failed to initialize OpenGL 4.1 backend for ImGui");
                return false;
            }
            initialized_ = true;
            return true;
        }

    public:

        // No copy, no move (manages global ImGui context).
        ImGuiGlfwOpenGLBackend(const ImGuiGlfwOpenGLBackend&) = delete;
        ImGuiGlfwOpenGLBackend& operator=(const ImGuiGlfwOpenGLBackend&) = delete;
        ImGuiGlfwOpenGLBackend(ImGuiGlfwOpenGLBackend&& other) noexcept = delete;
        ImGuiGlfwOpenGLBackend& operator=(ImGuiGlfwOpenGLBackend&& other) noexcept = delete;

        /**
         * @brief Constructs the ImGui backend for GLFW+OpenGL.
         *
         * @param window GLFW window handle. Must be valid for the lifetime of this backend.
         * @param ecsWorld Entity space used to resolve the native GLFW window handle.
         *
         * @throws std::runtime_error if an ImGui context already exists.
         */
        explicit ImGuiGlfwOpenGLBackend(WindowHandle window, ecs::entity::EntityWorld& ecsWorld)
            : windowHandle_(window), ecsWorld_(ecsWorld) {

            if (ImGui::GetCurrentContext()) {
                throw std::runtime_error("ImGui context already initialized");
            }

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            ImGui::StyleColorsDark();
        }

        /**
         * @brief Renders ImGui draw data using OpenGL.
         *
         * @param drawData Pointer to ImGui draw data.
         *
         * @note If the backend is not initialized yet, this call is a no-op.
         */
        void renderDrawData(ImDrawData* drawData) override {
            if (initialized_) {
                ImGui_ImplOpenGL3_RenderDrawData(drawData);
            }
        }

        /**
         * @brief Starts a new ImGui frame.
         *
         * If the windowHandle_ cannot be found, this method returns false.
         *
         * @return `true` if frame setup succeeded; otherwise `false`.
         */
        bool newFrame() override {

            if (!initialize()) {
                return false;
            }

            if (!ecsWorld_.find(windowHandle_)) {
                return false;
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            return true;
        }

        /**
         * @brief Destructor; shuts down ImGui backend and releases resources.
         *
         * @see shutdown()
         */
        ~ImGuiGlfwOpenGLBackend() override {
            shutdown();
        }

    };

}
