/**
 * @file CameraWidget.ixx
 * @brief ImGui widget for editing ECS camera and look-at components.
 */
module;

#include <string>
#include <vector>

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
import helios.engine.spatial.components.TargetPosition3DComponent;
import helios.engine.spatial.components.UpVector3DComponent;

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
     * and edits its camera/look-at ECS components at runtime.
     */
    class CameraWidget : public ImGuiWidget {

        using GameWorld = helios::engine::runtime::world::GameWorld;
        using GameObjectHandle = helios::engine::runtime::world::types::GameObjectHandle;
        using ViewportHandle = helios::engine::rendering::viewport::types::ViewportHandle;

        using ViewportCameraBindingComponent = helios::engine::scene::components::CameraBindingComponent<ViewportHandle>;
        using PerspectiveCameraComponent = helios::engine::scene::components::PerspectiveCameraComponent<GameObjectHandle>;
        using Position3DComponent = helios::engine::spatial::components::Position3DComponent<GameObjectHandle, Local>;
        using TargetPosition3DComponent = helios::engine::spatial::components::TargetPosition3DComponent<GameObjectHandle, World>;
        using UpVector3DComponent = helios::engine::spatial::components::UpVector3DComponent<GameObjectHandle>;

        struct ViewportCameraEntry {
            ViewportHandle viewportHandle{};
            GameObjectHandle cameraHandle{};
            std::string label;
        };

        struct CameraSnapshot {
            bool valid = false;
            bool hasPosition = false;
            bool hasTarget = false;
            bool hasUp = false;
            bool hasPerspective = false;

            helios::math::vec3f position{0.0f, 0.0f, 1.0f};
            helios::math::vec3f target{0.0f, 0.0f, 0.0f};
            helios::math::vec3f up{0.0f, 1.0f, 0.0f};

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
        helios::math::vec3f tempTarget_{0.0f, 0.0f, 0.0f};
        helios::math::vec3f tempUp_{0.0f, 1.0f, 0.0f};

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

            auto* viewportDebugNameCmp = viewportEntity ? viewportEntity->template get<DebugNameComponent<ViewportHandle>>() : nullptr;
            auto* cameraDebugNameCmp = cameraEntity ? cameraEntity->template get<DebugNameComponent<GameObjectHandle>>() : nullptr;

            return (viewportDebugNameCmp ? viewportDebugNameCmp->value : "Viewport " + std::to_string(viewportHandle.entityId))
                + " -> " + (cameraDebugNameCmp ? cameraDebugNameCmp->value : "Camera " + std::to_string(cameraHandle.entityId));
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

            for (auto [viewportEntity, cameraBinding] : gameWorld_->template view<ViewportHandle, ViewportCameraBindingComponent>()) {
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

            if (const auto* target = entity.template get<TargetPosition3DComponent>()) {
                snapshot.hasTarget = true;
                snapshot.target = target->value();
            }

            if (const auto* up = entity.template get<UpVector3DComponent>()) {
                snapshot.hasUp = true;
                snapshot.up = up->value();
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

            if (auto* target = entity.template get<TargetPosition3DComponent>()) {
                tempTarget_ = target->value();
            }

            if (auto* up = entity.template get<UpVector3DComponent>()) {
                tempUp_ = up->value();
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

    public:
        /**
         * @brief Creates the widget bound to a runtime `GameWorld`.
         *
         * @param gameWorld World used for viewport/camera discovery and component edits.
         */
        explicit CameraWidget(GameWorld& gameWorld) noexcept : gameWorld_(&gameWorld) {}

        /**
         * @brief Renders the camera editor UI and writes changes into ECS components.
         *
         * @details The UI provides:
         * - viewport/camera selection via `CameraBindingComponent<ViewportHandle>`
         * - editing for `Position3DComponent`, `TargetPosition3DComponent`, `UpVector3DComponent`
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
            if (selectedViewportCameraIndex_ >= 0 && selectedViewportCameraIndex_ < static_cast<int>(viewportCameraEntries_.size())) {
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
            bool targetChanged = false;
            bool upChanged = false;
            bool projectionChanged = false;
            bool missingPosition = false;
            bool missingTarget = false;
            bool missingUp = false;
            bool missingCamera = false;

            ImGui::SeparatorText("LookAt Components");

            ImGui::Text("Position3DComponent");
            positionChanged |= ImGui::DragFloat("X##Pos", &tempPosition_[0], 0.1f, -10000.0f, 10000.0f, "%.3f");
            positionChanged |= ImGui::DragFloat("Y##Pos", &tempPosition_[1], 0.1f, -10000.0f, 10000.0f, "%.3f");
            positionChanged |= ImGui::DragFloat("Z##Pos", &tempPosition_[2], 0.1f, -10000.0f, 10000.0f, "%.3f");

            ImGui::Spacing();
            ImGui::Text("TargetPosition3DComponent");
            targetChanged |= ImGui::DragFloat("X##Target", &tempTarget_[0], 0.1f, -10000.0f, 10000.0f, "%.3f");
            targetChanged |= ImGui::DragFloat("Y##Target", &tempTarget_[1], 0.1f, -10000.0f, 10000.0f, "%.3f");
            targetChanged |= ImGui::DragFloat("Z##Target", &tempTarget_[2], 0.1f, -10000.0f, 10000.0f, "%.3f");

            ImGui::Spacing();
            ImGui::Text("UpVector3DComponent");
            upChanged |= ImGui::DragFloat("X##Up", &tempUp_[0], 0.01f, -1.0f, 1.0f, "%.3f");
            upChanged |= ImGui::DragFloat("Y##Up", &tempUp_[1], 0.01f, -1.0f, 1.0f, "%.3f");
            upChanged |= ImGui::DragFloat("Z##Up", &tempUp_[2], 0.01f, -1.0f, 1.0f, "%.3f");

            ImGui::SameLine();
            if (ImGui::SmallButton("Normalize##Up")) {
                const float len = tempUp_.length();
                if (len > 0.0001f) {
                    tempUp_ = tempUp_.normalize();
                    upChanged = true;
                }
            }

            ImGui::SeparatorText("PerspectiveCameraComponent");
            projectionChanged |= ImGui::SliderFloat("FOV Y", &tempFovDegrees_, 10.0f, 170.0f, "%.1f deg");
            projectionChanged |= ImGui::DragFloat("Aspect", &tempAspectRatio_, 0.01f, 0.1f, 8.0f, "%.3f");
            projectionChanged |= ImGui::DragFloat("Near", &tempZNear_, 0.001f, 0.001f, 1000.0f, "%.4f");
            projectionChanged |= ImGui::DragFloat("Far", &tempZFar_, 1.0f, 0.01f, 1000000.0f, "%.2f");

            if (positionChanged) {
                auto* position = cameraEntity.template get<Position3DComponent>();
                if (position) {
                    position->setValue(tempPosition_);
                } else {
                    missingPosition = true;
                }
            }

            if (targetChanged) {
                auto* target = cameraEntity.template get<TargetPosition3DComponent>();
                if (target) {
                    target->setValue(tempTarget_);
                } else {
                    missingTarget = true;
                }
            }

            if (upChanged) {
                auto* up = cameraEntity.template get<UpVector3DComponent>();
                if (up) {
                    up->setValue(tempUp_);
                } else {
                    missingUp = true;
                }
            }

            if (projectionChanged) {
                missingCamera = !writeProjectionToEntity(cameraEntity);
            }

            ImGui::Spacing();
            ImGui::BeginDisabled(!resetSnapshot_.valid);
            if (ImGui::Button("Reset selected camera")) {
                if (resetSnapshot_.hasPosition) {
                    auto* position = cameraEntity.template get<Position3DComponent>();
                    if (position) {
                        position->setValue(resetSnapshot_.position);
                        tempPosition_ = resetSnapshot_.position;
                    } else {
                        missingPosition = true;
                    }
                }

                if (resetSnapshot_.hasTarget) {
                    auto* target = cameraEntity.template get<TargetPosition3DComponent>();
                    if (target) {
                        target->setValue(resetSnapshot_.target);
                        tempTarget_ = resetSnapshot_.target;
                    } else {
                        missingTarget = true;
                    }
                }

                if (resetSnapshot_.hasUp) {
                    auto* up = cameraEntity.template get<UpVector3DComponent>();
                    if (up) {
                        up->setValue(resetSnapshot_.up);
                        tempUp_ = resetSnapshot_.up;
                    } else {
                        missingUp = true;
                    }
                }

                if (resetSnapshot_.hasPerspective) {
                    auto* perspective = cameraEntity.template get<PerspectiveCameraComponent>();
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

            if (missingPosition || missingTarget || missingUp || missingCamera) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Missing component(s): values could not be written.");
                if (missingPosition) {
                    ImGui::TextDisabled("- Position3DComponent");
                }
                if (missingTarget) {
                    ImGui::TextDisabled("- TargetPosition3DComponent");
                }
                if (missingUp) {
                    ImGui::TextDisabled("- UpVector3DComponent");
                }
                if (missingCamera) {
                    ImGui::TextDisabled("- PerspectiveCameraComponent");
                }
            }

            ImGui::Separator();
            ImGui::TextDisabled("Handle: entityId=%u versionId=%u", cameraHandle_.entityId, cameraHandle_.versionId);

            ImGui::End();
        }
    };

}
