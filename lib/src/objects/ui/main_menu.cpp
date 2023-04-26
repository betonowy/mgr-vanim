#include "main_menu.hpp"

#include <imgui.h>

#include "../vdb/nano_vdb_resource.hpp"

#include "animation_controller.hpp"
#include "convert_vdb_nvdb.hpp"
#include "debug_window.hpp"
#include "file_dialog.hpp"
#include "popup.hpp"

#include <scene/object_context.hpp>
#include <utils/utf8_exception.hpp>

namespace objects::ui
{
main_menu::~main_menu() = default;

void main_menu::init(scene::object_context &ctx)
{
    {
        auto found_debug_windows = ctx.find_objects<debug_window>();

        if (!found_debug_windows.empty())
        {
            if (found_debug_windows.size() == 1)
            {
                _debug_window = std::move(found_debug_windows[0]);
            }
            else
            {
                throw std::runtime_error(std::string(__func__) + ": Unexpected more than one instance of debug "
                                                                 "window.");
            }
        }
    }
}

void main_menu::update(scene::object_context &ctx, float)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::BeginMenu("Convert OpenVDB to NanoVDB..."))
            {
                if (ImGui::MenuItem("...to 32-bit float grids"))
                {
                    ctx.add_object(std::make_shared<file_dialog>([ctx = &ctx](std::filesystem::path path) -> bool {
                        ctx->add_object(std::make_shared<convert_vdb_nvdb>(std::move(path), converter::nvdb_format::F32, converter::nvdb_error_method::relative, 0.01));
                        return true;
                    }));
                }

                if (ImGui::MenuItem("...to 16-bit float grids"))
                {
                    ctx.add_object(std::make_shared<file_dialog>([ctx = &ctx](std::filesystem::path path) -> bool {
                        ctx->add_object(std::make_shared<convert_vdb_nvdb>(std::move(path), converter::nvdb_format::F16, converter::nvdb_error_method::relative, 0.01));
                        return true;
                    }));
                }

                if (ImGui::MenuItem("...to 8-bit float grids"))
                {
                    ctx.add_object(std::make_shared<file_dialog>([ctx = &ctx](std::filesystem::path path) -> bool {
                        ctx->add_object(std::make_shared<convert_vdb_nvdb>(std::move(path), converter::nvdb_format::F8, converter::nvdb_error_method::relative, 0.01));
                        return true;
                    }));
                }

                if (ImGui::MenuItem("...to 4-bit float grids"))
                {
                    ctx.add_object(std::make_shared<file_dialog>([ctx = &ctx](std::filesystem::path path) -> bool {
                        ctx->add_object(std::make_shared<convert_vdb_nvdb>(std::move(path), converter::nvdb_format::F4, converter::nvdb_error_method::relative, 0.01));
                        return true;
                    }));
                }

                if (ImGui::MenuItem("...to variable-bit float grids"))
                {
                    ctx.add_object(std::make_shared<file_dialog>([ctx = &ctx](std::filesystem::path path) -> bool {
                        ctx->add_object(std::make_shared<convert_vdb_nvdb>(std::move(path), converter::nvdb_format::FN, converter::nvdb_error_method::relative, 0.01));
                        return true;
                    }));
                }

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Convert OpenVDB to custom format"))
            {
            }

            if (ImGui::MenuItem("Open NanoVDB animation"))
            {
                for (const auto &object : ctx.find_objects<animation_controller>())
                {
                    object->destroy();
                }

                ctx.share_object(std::make_shared<file_dialog>([ctx = &ctx](std::filesystem::path path) -> bool {
                    try
                    {
                        auto resource = ctx->share_object(std::make_shared<vdb::nano_vdb_resource>(std::move(path)));
                        ctx->add_object(std::make_shared<animation_controller>(std::move(resource)));
                        return true;
                    }
                    catch (utils::utf8_exception &e)
                    {
                        ctx->add_object(std::make_shared<popup>(u8"Can't load NanoVDB animation", e.utf8_what()));
                        return false;
                    }
                    catch (std::exception &e)
                    {
                        ctx->add_object(std::make_shared<popup>(u8"Can't load NanoVDB animation", reinterpret_cast<const char8_t*>(e.what())));
                        return false;
                    }
                }));
            }

            if (ImGui::MenuItem("Open custom animation"))
            {
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Recompile shaders"))
            {
                ctx.broadcast_signal(scene::signal_e::RELOAD_SHADERS);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Quit application", "Alt+F4"))
            {
                ctx.broadcast_signal(scene::signal_e::EXIT_APPLICATION);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            {
                bool active = _debug_window && !(_debug_window.use_count() == 1);

                if (_debug_window && (_debug_window.use_count() == 1))
                {
                    _debug_window->destroy(), _debug_window.reset();
                }

                if (ImGui::MenuItem("Debug window", nullptr, active))
                {
                    if (!active)
                    {
                        _debug_window = ctx.share_object(std::make_shared<debug_window>());
                    }
                    else
                    {
                        _debug_window->destroy(), _debug_window.reset();
                    }
                }
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}
} // namespace objects::ui
