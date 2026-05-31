/**
 * @file ImGuiLogSink.ixx
 * @brief Log sink that forwards messages to a LogWidget.
 */
module;

#include <string>

export module helios.imgui.ImGuiLogSink;

import helios.engine.util.log.LogSink;
import helios.imgui.widgets.LogWidget;

export namespace helios::imgui {

    /**
     * @class ImGuiLogSink
     * @brief LogSink implementation that forwards messages to a LogWidget.
     *
     * This sink bridges the helios logging system with the ImGui-based LogWidget,
     * allowing log output to be displayed in the debug overlay instead of (or in
     * addition to) the console.
     *
     * ```cpp
     * auto logWidget = std::make_shared<widgets::LogWidget>();
     * overlay.addWidget(logWidget.get());
     *
     * auto sink = std::make_shared<ImGuiLogSink>(logWidget.get());
     * LogManager::getInstance().registerSink(sink);
     * ```
     */
    class ImGuiLogSink : public helios::engine::util::log::LogSink {

    private:
        /**
         * @brief Pointer to the LogWidget to forward messages to.
         */
        widgets::LogWidget* widget_ = nullptr;

    public:
        /**
         * @brief Unique type identifier for this sink.
         */
        static constexpr helios::engine::util::log::SinkTypeId TYPE_ID = "imgui";

        /**
         * @brief Constructs an ImGuiLogSink attached to a LogWidget.
         *
         * @param widget Pointer to the LogWidget to receive messages.
         *               Must remain valid for the lifetime of this sink.
         */
        explicit ImGuiLogSink(widgets::LogWidget* widget)
            : widget_(widget) {}

        /**
         * @brief Returns the unique type identifier for this sink.
         *
         * @return "imgui".
         */
        [[nodiscard]] helios::engine::util::log::SinkTypeId typeId() const noexcept override {
            return TYPE_ID;
        }

        /**
         * @brief Forwards a log message to the LogWidget.
         *
         * @param level   The severity level of the message.
         * @param scope   The source scope/module name.
         * @param message The log message text.
         */
        void write(helios::engine::util::log::LogLevel level,
                   const std::string& scope,
                   const std::string& message) override {
            if (!widget_) return;

            // Map LogLevel to LogWidget's LogLevel
            widgets::LogLevel widgetLevel;
            switch (level) {
                case helios::engine::util::log::LogLevel::Debug:
                    widgetLevel = widgets::LogLevel::Debug; break;
                case helios::engine::util::log::LogLevel::Info:
                    widgetLevel = widgets::LogLevel::Info;  break;
                case helios::engine::util::log::LogLevel::Warn:
                    widgetLevel = widgets::LogLevel::Warn;  break;
                case helios::engine::util::log::LogLevel::Error:
                    widgetLevel = widgets::LogLevel::Error; break;
                default:
                    widgetLevel = widgets::LogLevel::Info;
            }

            widget_->addLog(widgetLevel, scope, message);
        }
    };

}

