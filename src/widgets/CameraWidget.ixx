/**
 * @file CameraWidget.ixx
 * @brief ImGui widget for editing ECS camera transform and projection components.
 */
module;

#include <string>
#include <vector>
#include <numbers>

#include "imgui.h"

export module helios.imgui.widgets.CameraWidget;

import helios.imgui.ImGuiWidget;

import helios.engine.core.components.DebugNameComponent;

import helios.engine.runtime.world.GameWorld;
import helios.engine.runtime.world.types.GameObjectHandle;
import helios.engine.rendering.viewport.types.ViewportHandle;

import helios.engine.scene.components.PerspectiveCameraComponent;
import helios.engine.scene.components.CameraBindingComponent;
import helios.engine.spatial.components.Position3DComponent;
import helios.engine.spatial.components.YawPitchRollComponent;

import helios.math.types;
import helios.math.utils;
import helios.engine.core.types.ComponentTypeTags;

using namespace helios::engine::core::types;
using namespace helios::engine::core::components;

export namespace helios::imgui::widgets {

    /**
     * @brief ECS-driven ImGui camera editor for viewport-bound cameras.
     *
     * @details The widget discovers all viewport entities that expose
     * `CameraBindingComponent<ViewportHandle>`, lets the user pick one camera,
     * and edits its transform/projection ECS components at runtime.
     */
    class CameraWidget : public ImGuiWidget {

        using GameWorld = helios::engine::runtime::world::GameWorld;
        using GameObjectHandle = helios::engine::runtime::world::types::GameObjectHandle;
        using ViewportHandle = helios::engine::rendering::viewport::types::ViewportHandle;

        using ViewportCameraBindingComponent =
            helios::engine::scene::components::CameraBindingComponent<ViewportHandle>;

        using PerspectiveCameraComponent =
            helios::engine::scene::components::PerspectiveCameraComponent<GameObjectHandle>;

        using Position3DComponent =
            helios::engine::spatial::components::Position3DComponent<GameObjectHandle, Local>;

        using YawPitchRollComponent =
            helios::engine::spatial::components::YawPitchRollComponent<GameObjectHandle>;

        struct ViewportCameraEntry {
            ViewportHandle viewportHandle{};
            GameObjectHandle cameraHandle{};
            std::string label;
        };

        struct CameraSnapshot {
            bool valid = false;
            bool hasPosition = false;
            bool hasRotation = false;
            bool hasPerspective = false;

            helios::math::vec3f position{0.0f, 0.0f, 1.0f};

            /**
             * @brief First-person control state in radians.
             *
             * @details Stored as:
             * - x: yaw
             * - y: pitch
             * - z: roll
             */
            helios::math::vec3f rotation{0.0f, 0.0f, 0.0f};

            float fovYDegrees = 90.0f;
            float aspectRatio = 16.0f / 9.0f;
            float zNear = 0.1f;
            float zFar = 1000.0f;
        };

        GameWorld* gameWorld_ = nullptr;
        GameObjectHandle cameraHandle_{};
        std::vector<ViewportCameraEntry> viewportCameraEntries_{};
        int selectedViewportCameraIndex_ = -1;

        helios::math::vec3f tempPosition_{0.0f, 0.0f, 1.0f};

        /**
         * @brief First-person control state in radians.
         *
         * @details Stored as:
         * - x: yaw
         * - y: pitch
         * - z: roll
         */
        helios::math::vec3f tempRotation_{0.0f, 0.0f, 0.0f};

        float tempFovDegrees_ = 90.0f;
        float tempAspectRatio_ = 16.0f / 9.0f;
        float tempZNear_ = 0.1f;
        float tempZFar_ = 1000.0f;

        bool syncedFromEntity_ = false;
        CameraSnapshot resetSnapshot_{};

        [[nodiscard]] std::string makeViewportCameraLabel(
            const ViewportHandle viewportHandle,
            const GameObjectHandle cameraHandle
        ) {
            auto viewportEntity = gameWorld_->find(viewportHandle);
            auto cameraEntity = gameWorld_->find(cameraHandle);

            auto* viewportDebugNameCmp = viewportEntity
                ? viewportEntity->get<DebugNameComponent<ViewportHandle>>()
                : nullptr;

            auto* cameraDebugNameCmp = cameraEntity
                ? cameraEntity->get<DebugNameComponent<GameObjectHandle>>()
                : nullptr;

            return (viewportDebugNameCmp
                    ? viewportDebugNameCmp->value
                    : "Viewport " + std::to_string(viewportHandle.entityId))
                + " -> "
                + (cameraDebugNameCmp
                    ? cameraDebugNameCmp->value
                    : "Camera " + std::to_string(cameraHandle.entityId));
        }

        void refreshViewportCameraEntries() {
            const auto previousCameraHandle = cameraHandle_;
            ViewportHandle previousViewportHandle{};
            bool hasPreviousViewport = false;

            if (selectedViewportCameraIndex_ >= 0
                && selectedViewportCameraIndex_ < static_cast<int>(viewportCameraEntries_.size())) {
                previousViewportHandle = viewportCameraEntries_[selectedViewportCameraIndex_].viewportHandle;
                hasPreviousViewport = true;
            }

            viewportCameraEntries_.clear();

            if (!gameWorld_) {
                selectedViewportCameraIndex_ = -1;
                cameraHandle_ = {};
                syncedFromEntity_ = false;
                return;
            }

            for (auto [viewportEntity, cameraBinding] :
                gameWorld_->view<ViewportHandle, ViewportCameraBindingComponent>()) {

                const auto viewportHandle = viewportEntity.handle();
                const auto boundCameraHandle = cameraBinding->targetHandle();

                viewportCameraEntries_.push_back(ViewportCameraEntry{
                    viewportHandle,
                    boundCameraHandle,
                    makeViewportCameraLabel(viewportHandle, boundCameraHandle)
                });
            }

            if (viewportCameraEntries_.empty()) {
                selectedViewportCameraIndex_ = -1;
                cameraHandle_ = {};
                syncedFromEntity_ = false;
                return;
            }

            int matchingIndex = -1;

            if (hasPreviousViewport) {
                for (int i = 0; i < static_cast<int>(viewportCameraEntries_.size()); ++i) {
                    if (viewportCameraEntries_[i].viewportHandle == previousViewportHandle) {
                        matchingIndex = i;
                        break;
                    }
                }
            }

            if (matchingIndex < 0) {
                for (int i = 0; i < static_cast<int>(viewportCameraEntries_.size()); ++i) {
                    if (viewportCameraEntries_[i].cameraHandle == previousCameraHandle) {
                        matchingIndex = i;
                        break;
                    }
                }
            }

            if (matchingIndex < 0) {
                matchingIndex = 0;
            }

            if (selectedViewportCameraIndex_ != matchingIndex) {
                selectedViewportCameraIndex_ = matchingIndex;
                cameraHandle_ = viewportCameraEntries_[selectedViewportCameraIndex_].cameraHandle;

                /**
                 * The selected camera changed. We keep tempRotation_ as the widget's
                 * first-person control state and re-sync position/projection from ECS.
                 */
                syncedFromEntity_ = false;
            }
        }

        void selectViewportCameraIndex(const int index) {
            if (index < 0 || index >= static_cast<int>(viewportCameraEntries_.size())) {
                return;
            }

            if (selectedViewportCameraIndex_ == index) {
                return;
            }

            selectedViewportCameraIndex_ = index;
            cameraHandle_ = viewportCameraEntries_[selectedViewportCameraIndex_].cameraHandle;

            /**
             * Do not decompose the camera quaternion back into Euler angles here.
             * tempRotation_ is the widget's first-person control state.
             */
            syncedFromEntity_ = false;
        }

        template<typename TEntity>
        [[nodiscard]] CameraSnapshot captureSnapshot(const TEntity& entity) const noexcept {
            CameraSnapshot snapshot{};
            snapshot.valid = true;

            if (const auto* position = entity.template get<Position3DComponent>()) {
                snapshot.hasPosition = true;
                snapshot.position = position->value();
            }

            if (const auto* yawPitchRoll = entity.template get<YawPitchRollComponent>()) {
                snapshot.hasRotation = true;
                snapshot.rotation = helios::math::vec3f{
                    yawPitchRoll->yaw,
                    yawPitchRoll->pitch,
                    yawPitchRoll->roll
                };
            }

            if (const auto* camera = entity.template get<PerspectiveCameraComponent>()) {
                snapshot.hasPerspective = true;
                snapshot.fovYDegrees = helios::math::degrees(camera->fovY());
                snapshot.aspectRatio = camera->aspectRatio();
                snapshot.zNear = camera->zNear();
                snapshot.zFar = camera->zFar();
            }

            return snapshot;
        }

        template<typename TEntity>
        void syncFromEntity(TEntity& entity) noexcept {
            if (auto* position = entity.template get<Position3DComponent>()) {
                tempPosition_ = position->value();
            }

            if (auto* yawPitchRoll = entity.template get<YawPitchRollComponent>()) {
                tempRotation_ = helios::math::vec3f{
                    yawPitchRoll->yaw,
                    yawPitchRoll->pitch,
                    yawPitchRoll->roll
                };
            }

            if (auto* camera = entity.template get<PerspectiveCameraComponent>()) {
                tempFovDegrees_ = helios::math::degrees(camera->fovY());
                tempAspectRatio_ = camera->aspectRatio();
                tempZNear_ = camera->zNear();
                tempZFar_ = camera->zFar();
            }
        }

        template<typename TEntity>
        bool writeProjectionToEntity(TEntity& entity) {
            if (tempZNear_ <= 0.0f) {
                tempZNear_ = 0.001f;
            }

            if (tempZFar_ <= tempZNear_) {
                tempZFar_ = tempZNear_ + 0.01f;
            }

            if (tempAspectRatio_ <= 0.0f) {
                tempAspectRatio_ = 1.0f;
            }

            auto* camera = entity.template get<PerspectiveCameraComponent>();
            if (!camera) {
                return false;
            }

            camera->setPerspective(
                helios::math::radians(tempFovDegrees_),
                tempAspectRatio_,
                tempZNear_,
                tempZFar_
            );

            return true;
        }

        template<typename TEntity>
        bool writeYawPitchRollToEntity(TEntity& entity) {
            auto* yawPitchRoll = entity.template get<YawPitchRollComponent>();
            if (!yawPitchRoll) {
                return false;
            }

            yawPitchRoll->yaw = tempRotation_[0];
            yawPitchRoll->pitch = tempRotation_[1];
            yawPitchRoll->roll = tempRotation_[2];

            return true;
        }

    public:

        /**
         * @brief Creates the widget bound to a runtime `GameWorld`.
         *
         * @param gameWorld World used for viewport/camera discovery and component edits.
         */
        explicit CameraWidget(GameWorld& gameWorld) noexcept
            : gameWorld_(&gameWorld) {}

        /**
         * @brief Renders the camera editor UI and writes changes into ECS components.
         *
         * @details The UI provides:
         * - viewport/camera selection via `CameraBindingComponent<ViewportHandle>`
         * - editing for `Position3DComponent`
         * - first-person-style yaw/pitch/roll editing for `YawPitchRollComponent`
         * - editing for `PerspectiveCameraComponent`
         * - reset to the snapshot captured when the current selection became active
         */
        void draw() override {
            ImGui::SetNextWindowSize(ImVec2(380, 520), ImGuiCond_FirstUseEver);

            if (!ImGui::Begin("Camera ECS", nullptr, ImGuiWindowFlags_NoCollapse)) {
                ImGui::End();
                return;
            }

            if (!gameWorld_) {
                ImGui::TextDisabled("GameWorld missing.");
                ImGui::End();
                return;
            }

            refreshViewportCameraEntries();

            if (viewportCameraEntries_.empty()) {
                ImGui::TextDisabled("No viewport has CameraBindingComponent.");
                ImGui::End();
                return;
            }

            const char* selectedLabel = "<none>";
            if (selectedViewportCameraIndex_ >= 0
                && selectedViewportCameraIndex_ < static_cast<int>(viewportCameraEntries_.size())) {
                selectedLabel = viewportCameraEntries_[selectedViewportCameraIndex_].label.c_str();
            }

            if (ImGui::BeginCombo("Viewport / Camera", selectedLabel)) {
                for (int i = 0; i < static_cast<int>(viewportCameraEntries_.size()); ++i) {
                    const bool isSelected = selectedViewportCameraIndex_ == i;

                    if (ImGui::Selectable(viewportCameraEntries_[i].label.c_str(), isSelected)) {
                        selectViewportCameraIndex(i);
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }

            auto cameraEntityOptional = gameWorld_->find(cameraHandle_);
            if (!cameraEntityOptional) {
                ImGui::TextDisabled("Camera entity not found or stale handle.");
                ImGui::End();
                return;
            }

            auto& cameraEntity = *cameraEntityOptional;

            if (!syncedFromEntity_) {
                syncFromEntity(cameraEntity);
                resetSnapshot_ = captureSnapshot(cameraEntity);
                syncedFromEntity_ = true;
            }

            bool positionChanged = false;
            bool rotationChanged = false;
            bool projectionChanged = false;

            bool missingPosition = false;
            bool missingRotation = false;
            bool missingCamera = false;

            ImGui::SeparatorText("Spatial Components");

            ImGui::Text("Position3DComponent");
            positionChanged |= ImGui::DragFloat(
                "X##Pos",
                &tempPosition_[0],
                0.1f,
                -10000.0f,
                10000.0f,
                "%.3f"
            );

            positionChanged |= ImGui::DragFloat(
                "Y##Pos",
                &tempPosition_[1],
                0.1f,
                -10000.0f,
                10000.0f,
                "%.3f"
            );

            positionChanged |= ImGui::DragFloat(
                "Z##Pos",
                &tempPosition_[2],
                0.1f,
                -10000.0f,
                10000.0f,
                "%.3f"
            );

            ImGui::Spacing();
            ImGui::Text("YawPitchRollComponent (first-person yaw/pitch/roll)");

            const auto pi = static_cast<float>(std::numbers::pi);
            const auto halfPi = pi * 0.5f;

            rotationChanged |= ImGui::SliderFloat(
                "Pitch##Rot",
                &tempRotation_[1],
                -halfPi + 0.001f,
                halfPi - 0.001f,
                "%.3f rad"
            );

            rotationChanged |= ImGui::SliderFloat(
                "Yaw##Rot",
                &tempRotation_[0],
                -pi,
                pi,
                "%.3f rad"
            );

            /**
             * Keep roll available for debugging/editor use.
             * For a strict FPS camera, remove this slider and keep tempRotation_[2] = 0.
             */
            rotationChanged |= ImGui::SliderFloat(
                "Roll##Rot",
                &tempRotation_[2],
                -pi,
                pi,
                "%.3f rad"
            );

            ImGui::SeparatorText("PerspectiveCameraComponent");

            projectionChanged |= ImGui::SliderFloat(
                "FOV Y",
                &tempFovDegrees_,
                10.0f,
                170.0f,
                "%.1f deg"
            );

            projectionChanged |= ImGui::DragFloat(
                "Aspect",
                &tempAspectRatio_,
                0.01f,
                0.1f,
                8.0f,
                "%.3f"
            );

            projectionChanged |= ImGui::DragFloat(
                "Near",
                &tempZNear_,
                0.001f,
                0.001f,
                1000.0f,
                "%.4f"
            );

            projectionChanged |= ImGui::DragFloat(
                "Far",
                &tempZFar_,
                1.0f,
                0.01f,
                1000000.0f,
                "%.2f"
            );

            if (positionChanged) {
                auto* position = cameraEntity.get<Position3DComponent>();
                if (position) {
                    position->setValue(tempPosition_);
                } else {
                    missingPosition = true;
                }
            }

            if (rotationChanged) {
                missingRotation = !writeYawPitchRollToEntity(cameraEntity);
            }

            if (projectionChanged) {
                missingCamera = !writeProjectionToEntity(cameraEntity);
            }

            ImGui::Spacing();
            ImGui::BeginDisabled(!resetSnapshot_.valid);

            if (ImGui::Button("Reset selected camera")) {
                if (resetSnapshot_.hasPosition) {
                    auto* position = cameraEntity.get<Position3DComponent>();
                    if (position) {
                        position->setValue(resetSnapshot_.position);
                        tempPosition_ = resetSnapshot_.position;
                    } else {
                        missingPosition = true;
                    }
                }

                if (resetSnapshot_.hasRotation) {
                    tempRotation_ = resetSnapshot_.rotation;
                    missingRotation = !writeYawPitchRollToEntity(cameraEntity);
                }

                if (resetSnapshot_.hasPerspective) {
                    auto* perspective = cameraEntity.get<PerspectiveCameraComponent>();
                    if (perspective) {
                        perspective->setPerspective(
                            helios::math::radians(resetSnapshot_.fovYDegrees),
                            resetSnapshot_.aspectRatio,
                            resetSnapshot_.zNear,
                            resetSnapshot_.zFar
                        );

                        tempFovDegrees_ = resetSnapshot_.fovYDegrees;
                        tempAspectRatio_ = resetSnapshot_.aspectRatio;
                        tempZNear_ = resetSnapshot_.zNear;
                        tempZFar_ = resetSnapshot_.zFar;
                    } else {
                        missingCamera = true;
                    }
                }
            }

            ImGui::EndDisabled();

            if (missingPosition || missingRotation || missingCamera) {
                ImGui::Separator();
                ImGui::TextColored(
                    ImVec4(1.0f, 0.75f, 0.2f, 1.0f),
                    "Missing component(s): values could not be written."
                );

                if (missingPosition) {
                    ImGui::TextDisabled("- Position3DComponent");
                }

                if (missingRotation) {
                    ImGui::TextDisabled("- YawPitchRollComponent");
                }

                if (missingCamera) {
                    ImGui::TextDisabled("- PerspectiveCameraComponent");
                }
            }

            ImGui::Separator();
            ImGui::TextDisabled(
                "Handle: entityId=%u versionId=%u",
                cameraHandle_.entityId,
                cameraHandle_.versionId
            );

            ImGui::End();
        }
    };

}