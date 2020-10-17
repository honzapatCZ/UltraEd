#include <ImGui/imconfig.h>
#include <thread>
#include "Debug.h"
#include "Gui.h"
#include "FileIO.h"
#include "Scene.h"
#include "Settings.h"
#include "View.h"

namespace UltraEd
{
    Gui::Gui(HWND hWnd) :
        m_scene(std::make_unique<Scene>(hWnd, this)),
        m_hWnd(hWnd),
        m_renderDevice(hWnd),
        m_sceneTexture(),
        m_selectedActor(),
        m_textEditor(),
        m_fileBrowser(ImGuiFileBrowserFlags_EnterNewFilename),
        m_folderBrowser(ImGuiFileBrowserFlags_SelectDirectory),
        m_consoleText(),
        m_moveConsoleToBottom(),
        m_openContextMenu(),
        m_textEditorOpen(),
        m_optionsModalOpen(),
        m_sceneSettingsModalOpen(),
        m_newProjectModalOpen(),
        m_loadProjectModalOpen(),
        m_addTextureModalOpen(),
        m_addModelModalOpen(),
        m_loadSceneModalOpen(),
        m_saveSceneModalOpen(),
        m_openConfirmSceneModal()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        LoadFonts();
        LoadColorTheme();

        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

        ImGui_ImplWin32_Init(hWnd);
        ImGui_ImplDX9_Init(m_renderDevice.GetDevice());

        m_textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::C());

        Debug::Instance().Connect([&](std::string text) {
            m_consoleText.append(text);
            m_moveConsoleToBottom = true;
        });
    }

    Gui::~Gui()
    {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void Gui::PrepareFrame()
    {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", 0, window_flags);
        {
            ImGui::PopStyleVar(3);

            ImGuiID dockspace_id = ImGui::GetID("RootDockspace");
            ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

            if (ImGui::BeginMainMenuBar())
            {
                FileMenu();
                EditMenu();
                ActorMenu();
                ViewMenu();
                GizmoMenu();
                ImGui::EndMainMenuBar();
            }

            SceneGraph();
            SceneView();
            Console();
            Properties();
            OptionsModal();
            SceneSettingsModal();
            ContextMenu();
            ScriptEditor();
            NewProjectModal();
            LoadProjectModal();
            AddTextureModal();
            AddModelModal();
            ConfirmSceneModal();
            SaveSceneModal();
            LoadSceneModal();

            //ThemeEditor();
            //ImGui::ShowDemoWindow();
        }
        ImGui::End();
        ImGui::EndFrame();
    }

    void Gui::Render()
    {
        PrepareFrame();
        auto device = m_renderDevice.GetDevice();

        if (SUCCEEDED(device->BeginScene()))
        {
            device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(100, 100, 100), 1.0f, 0);

            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            device->EndScene();
            device->Present(NULL, NULL, NULL, NULL);
        }
    }

    void Gui::Resize(UINT width, UINT height)
    {
        ImGui_ImplDX9_InvalidateDeviceObjects();

        Project::Refresh();
        ReleaseSceneTexture();
        m_renderDevice.Resize(width, height);

        ImGui_ImplDX9_CreateDeviceObjects();
    }

    ImGuiIO &Gui::IO()
    {
        return ImGui::GetIO();
    }

    void Gui::OpenContextMenu(Actor *selectedActor)
    {
        m_openContextMenu = true;
        m_selectedActor = selectedActor;
    }

    void Gui::ConfirmScene(std::function<void()> block)
    {
        m_openConfirmSceneModal = std::make_tuple(true, block);
    }

    void Gui::RefreshScene(const std::vector<boost::uuids::uuid> &changedAssetIds)
    {
        m_scene->Refresh(changedAssetIds);
    }

    void Gui::LoadFonts()
    {
        ImGuiIO& io = ImGui::GetIO();
        //io.Fonts->AddFontDefault();
        io.Fonts->AddFontFromMemoryCompressedTTF(roboto_compressed_data, roboto_compressed_size, 13.0f);
        ImFontConfig config;
        config.MergeMode = true;
        const ImWchar icon_ranges[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
        io.Fonts->AddFontFromMemoryCompressedTTF(fk_compressed_data, fk_compressed_size, 13.0f, &config, icon_ranges);
        //io.Fonts->AddFontFromFileTTF("D:/Dev/CPPaCS/tst/kenney-icon-font.ttf", 13.0f, &config, icon_ranges);
        io.Fonts->Build();
    }
    void Gui::LightColors() {
        ImGuiStyle* style = &ImGui::GetStyle();
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.73f, 0.73f, 0.73f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.98f, 0.09f, 1.00f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.73f, 0.08f, 0.47f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.71f, 0.71f, 0.71f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.71f, 0.71f, 0.71f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.62f, 0.62f, 0.62f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.71f, 0.71f, 0.71f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.62f, 0.62f, 0.62f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.71f, 0.71f, 0.71f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.71f, 0.71f, 0.71f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.62f, 0.62f, 0.62f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.71f, 0.71f, 0.71f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.69f, 0.69f, 0.69f, 0.16f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.21f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.21f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.21f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.59f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.59f);

        CustomStyle(style);
    }
    void Gui::DarkColors() {
        ImGuiStyle* style = &ImGui::GetStyle();
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.28f, 0.28f, 0.28f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
        colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.39f);
        colors[ImGuiCol_Header] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.22f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.21f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.21f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.21f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.59f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.59f);

        CustomStyle(style);
    }
    void Gui::CustomStyle(ImGuiStyle* style) {
        style->WindowPadding = ImVec2(4, 4);
        style->FramePadding = ImVec2(4, 4);
        style->ItemSpacing = ImVec2(4, 4);
        style->ItemInnerSpacing = ImVec2(4, 4);
        style->TouchExtraPadding = ImVec2(0, 0);
        style->IndentSpacing = 12.0f;
        style->ScrollbarSize = 10.0f;
        style->GrabMinSize = 10.0f;

        style->WindowBorderSize = 0.0f;
        style->ChildBorderSize = 0.0f;
        style->PopupBorderSize = 0.0f;
        style->FrameBorderSize = 0.0f;
        style->TabBorderSize = 0.0f;

        style->WindowRounding = 0.0f;
        style->ChildRounding = 0.0f;
        style->FrameRounding = 12.0f;
        style->PopupRounding = 0.0f;
        style->ScrollbarRounding = 12.0f;
        style->GrabRounding = 12.0f;
        style->LogSliderDeadzone = 0.0f;
        style->TabRounding = 0.0f;

        style->WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style->WindowMenuButtonPosition = ImGuiDir_::ImGuiDir_Right;
        style->ColorButtonPosition = ImGuiDir_::ImGuiDir_Right;
        style->ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style->SelectableTextAlign = ImVec2(0.0f, 0.0f);

        style->DisplaySafeAreaPadding = ImVec2(3.0f, 3.0f);
    }
    void Gui::LoadColorTheme()
    {
        switch (Settings::GetColorTheme())
        {
            case ColorTheme::Dark: {
                DarkColors();
                break;
            }                
            case ColorTheme::Light:
            {
                LightColors();
                break;
            }
        }

    }

    void Gui::ThemeEditor() {
        if (ImGui::Begin("Theme Editor", 0, ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGuiStyle* style = &ImGui::GetStyle();
            ImVec4* colors = style->Colors;

            ImGui::Text("Main");
            ImGui::SliderFloat2("WindowPadding", (float*)&style->WindowPadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("FramePadding", (float*)&style->FramePadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("ItemSpacing", (float*)&style->ItemSpacing, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("ItemInnerSpacing", (float*)&style->ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("TouchExtraPadding", (float*)&style->TouchExtraPadding, 0.0f, 10.0f, "%.0f");
            ImGui::SliderFloat("IndentSpacing", &style->IndentSpacing, 0.0f, 30.0f, "%.0f");
            ImGui::SliderFloat("ScrollbarSize", &style->ScrollbarSize, 1.0f, 20.0f, "%.0f");
            ImGui::SliderFloat("GrabMinSize", &style->GrabMinSize, 1.0f, 20.0f, "%.0f");
            ImGui::Text("Borders");
            ImGui::SliderFloat("WindowBorderSize", &style->WindowBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat("ChildBorderSize", &style->ChildBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat("PopupBorderSize", &style->PopupBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat("FrameBorderSize", &style->FrameBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat("TabBorderSize", &style->TabBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::Text("Rounding");
            ImGui::SliderFloat("WindowRounding", &style->WindowRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("ChildRounding", &style->ChildRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("FrameRounding", &style->FrameRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("PopupRounding", &style->PopupRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("ScrollbarRounding", &style->ScrollbarRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("GrabRounding", &style->GrabRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("LogSliderDeadzone", &style->LogSliderDeadzone, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat("TabRounding", &style->TabRounding, 0.0f, 12.0f, "%.0f");
            ImGui::Text("Alignment");
            ImGui::SliderFloat2("WindowTitleAlign", (float*)&style->WindowTitleAlign, 0.0f, 1.0f, "%.2f");
            int window_menu_button_position = style->WindowMenuButtonPosition + 1;
            if (ImGui::Combo("WindowMenuButtonPosition", (int*)&window_menu_button_position, "None\0Left\0Right\0"))
                style->WindowMenuButtonPosition = window_menu_button_position - 1;
            ImGui::Combo("ColorButtonPosition", (int*)&style->ColorButtonPosition, "Left\0Right\0");
            ImGui::SliderFloat2("ButtonTextAlign", (float*)&style->ButtonTextAlign, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat2("SelectableTextAlign", (float*)&style->SelectableTextAlign, 0.0f, 1.0f, "%.2f");
            ImGui::Text("Safe Area Padding");
            ImGui::SliderFloat2("DisplaySafeAreaPadding", (float*)&style->DisplaySafeAreaPadding, 0.0f, 30.0f, "%.0f");

            ImGui::Text("Colors");
            for (int i = 0; i < ImGuiCol_COUNT; i++)
            {
                const char* name = ImGui::GetStyleColorName(i);
                ImGui::PushID(i);
                ImGui::ColorEdit4("##color", (float*)&style->Colors[i], ImGuiColorEditFlags_AlphaBar);
                ImGui::SameLine(0.0f, style->ItemInnerSpacing.x);
                ImGui::TextUnformatted(name);
                ImGui::PopID();
            }
            if (ImGui::Button("Export"))
            {
                ImGui::LogToClipboard();
                ImGui::LogText("ImVec4* colors = ImGui::GetStyle().Colors;" "\n");
                for (int i = 0; i < ImGuiCol_COUNT; i++)
                {
                    const ImVec4& col = style->Colors[i];
                    const char* name = ImGui::GetStyleColorName(i);
                    ImGui::LogText("colors[ImGuiCol_%s]%*s= ImVec4(%.2ff, %.2ff, %.2ff, %.2ff);" "\n", name, 23 - (int)strlen(name), "", col.x, col.y, col.z, col.w);
                }
                ImGui::LogFinish();
            }
        }

        ImGui::End();
    }

    void Gui::ReleaseSceneTexture()
    {
        if (m_sceneTexture)
        {
            m_sceneTexture->Release();
            m_sceneTexture = nullptr;
        }
    }

    void Gui::FileMenu()
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem(ICON_FK_FILE" New Project"))
            {
                ConfirmScene([&]() {
                    m_newProjectModalOpen = true;
                });
            }

            if (Project::IsLoaded() && ImGui::MenuItem(ICON_FK_FLOPPY_O" Save Project"))
            {
                Project::Save();
            }

            if (ImGui::MenuItem(ICON_FK_FOLDER" Load Project"))
            {
                ConfirmScene([&]() {
                    m_loadProjectModalOpen = true;
                    m_folderBrowser.SetTitle("Load Project");
                    m_folderBrowser.Open();
                });
            }

            if (Project::IsLoaded())
            {
                ImGui::Separator();

                if (ImGui::MenuItem(ICON_FK_FILE" New Scene"))
                {
                    ConfirmScene([&]() { m_scene->New(); });
                }

                if (m_scene->HasPath() && ImGui::MenuItem(ICON_FK_FLOPPY_O" Save Scene", "Ctrl+S"))
                {
                    m_scene->SaveAs();
                }

                if (ImGui::MenuItem(ICON_FK_FLOPPY_O" Save Scene As...", m_scene->HasPath() ? 0 : "Ctrl+S"))
                {
                    m_saveSceneModalOpen = std::make_tuple(true, []() {});
                    m_fileBrowser.SetTitle("Save Scene As...");
                    m_fileBrowser.Open();
                }

                if (ImGui::MenuItem(ICON_FK_FOLDER" Load Scene"))
                {
                    ConfirmScene([&]() {
                        m_loadSceneModalOpen = true;
                        m_fileBrowser.SetTitle("Load Scene");
                        m_fileBrowser.SetTypeFilters({ APP_SCENE_FILE_EXT });
                        m_fileBrowser.Open();
                    });
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Build ROM"))
                {
                    m_scene->BuildROM(BuildFlag::Build);
                }

                if (ImGui::MenuItem("Build ROM & Load"))
                {
                    m_scene->BuildROM(BuildFlag::Load);
                }

                if (ImGui::MenuItem("Build ROM & Run"))
                {
                    m_scene->BuildROM(BuildFlag::Run);
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Install Build Tools"))
            {
                char pathBuffer[128];
                GetFullPathName("..\\Engine\\tools.bin", 128, pathBuffer, NULL);
                Debug::Instance().Info("Installing build tools...");

                std::thread run([pathBuffer]() {
                    if (FileIO::Unpack(pathBuffer))
                    {
                        Debug::Instance().Info("Build tools successfully installed.");
                    }
                    else
                    {
                        Debug::Instance().Error("Could not find build tools.");
                    }
                });
                run.detach();
            }

            if (ImGui::MenuItem(ICON_FK_COGS" Options"))
            {
                m_optionsModalOpen = true;
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit"))
            {
                SendMessage(m_hWnd, WM_CLOSE, 0, 0);
            }

            ImGui::EndMenu();
        }
    }

    void Gui::EditMenu()
    {
        if (Project::IsLoaded() && ImGui::BeginMenu("Edit"))
        {
            auto auditorTitles = m_scene->m_auditor.Titles();

            if (ImGui::MenuItem(auditorTitles[0].c_str(), "Ctrl+Z"))
            {
                m_scene->m_auditor.Undo();
            }

            if (ImGui::MenuItem(auditorTitles[1].c_str(), "Ctrl+Y"))
            {
                m_scene->m_auditor.Redo();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
            {
                m_scene->Duplicate();
            }

            if (ImGui::MenuItem("Delete", "Delete"))
            {
                m_scene->Delete();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Select All", "Ctrl+A"))
            {
                m_scene->SelectAll();
            }

            if (ImGui::MenuItem("Scene Settings"))
            {
                m_sceneSettingsModalOpen = true;
            }

            ImGui::EndMenu();
        }
    }

    void Gui::ActorMenu()
    {
        if (Project::IsLoaded() && ImGui::BeginMenu("Actor"))
        {
            if (ImGui::MenuItem("Camera"))
            {
                m_scene->AddCamera();
            }

            if (ImGui::MenuItem("Model"))
            {
                m_addModelModalOpen = true;
            }

            if (ImGui::BeginMenu("Texture"))
            {
                if (ImGui::MenuItem("Add"))
                {
                    m_addTextureModalOpen = true;
                }

                if (ImGui::MenuItem("Delete"))
                {
                    m_scene->DeleteTexture();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Collider"))
            {
                if (ImGui::MenuItem("Box"))
                {
                    m_scene->AddCollider(ColliderType::Box);
                }

                if (ImGui::MenuItem("Sphere"))
                {
                    m_scene->AddCollider(ColliderType::Sphere);
                }

                if (ImGui::MenuItem("Delete"))
                {
                    m_scene->DeleteCollider();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    }

    void Gui::ViewMenu()
    {
        if (Project::IsLoaded() && ImGui::BeginMenu("View"))
        {
            auto viewType = m_scene->GetActiveView()->GetType();

            if (ImGui::MenuItem("Perspective", 0, viewType == ViewType::Perspective))
            {
                m_scene->SetViewType(ViewType::Perspective);
            }

            if (ImGui::MenuItem("Top", 0, viewType == ViewType::Top))
            {
                m_scene->SetViewType(ViewType::Top);
            }

            if (ImGui::MenuItem("Left", 0, viewType == ViewType::Left))
            {
                m_scene->SetViewType(ViewType::Left);
            }

            if (ImGui::MenuItem("Front", 0, viewType == ViewType::Front))
            {
                m_scene->SetViewType(ViewType::Front);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Go To", "F"))
            {
                m_scene->FocusSelected();
            }

            if (ImGui::MenuItem("Home", "H"))
            {
                m_scene->ResetViews();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Solid", 0, m_scene->m_fillMode == D3DFILL_SOLID))
            {
                m_scene->ToggleFillMode();
            }

            ImGui::EndMenu();
        }
    }

    void Gui::GizmoMenu()
    {
        if (Project::IsLoaded() && ImGui::BeginMenu("Gizmo"))
        {
            if (ImGui::MenuItem("Translate", "1", m_scene->m_gizmo.m_modifierState == GizmoModifierState::Translate))
            {
                m_scene->m_gizmo.SetModifier(GizmoModifierState::Translate);
            }

            if (ImGui::MenuItem("Rotate", "2", m_scene->m_gizmo.m_modifierState == GizmoModifierState::Rotate))
            {
                m_scene->m_gizmo.SetModifier(GizmoModifierState::Rotate);
            }

            if (ImGui::MenuItem("Scale", "3", m_scene->m_gizmo.m_modifierState == GizmoModifierState::Scale))
            {
                m_scene->m_gizmo.SetModifier(GizmoModifierState::Scale);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("World Space", 0, m_scene->m_gizmo.m_worldSpaceToggled))
            {
                m_scene->ToggleMovementSpace();
            }

            if (ImGui::MenuItem("Snap to Grid", 0, m_scene->m_gizmo.m_snapToGridToggled))
            {
                m_scene->m_gizmo.ToggleSnapping();
            }

            ImGui::EndMenu();
        }
    }

    void Gui::SceneGraph()
    {
        if (ImGui::Begin(ICON_FK_TH_LIST" Scene Graph", 0, ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
                | ImGuiTreeNodeFlags_SpanAvailWidth;

            auto actors = m_scene->GetActors();
            for (int i = 0; i < actors.size(); i++)
            {
                ImGuiTreeNodeFlags leafFlags = baseFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (m_scene->IsActorSelected(actors[i]->GetId()))
                    leafFlags |= ImGuiTreeNodeFlags_Selected;

                ImGui::TreeNodeEx((void *)(intptr_t)i, leafFlags, actors[i]->GetName().c_str());
                if (ImGui::IsItemClicked())
                    m_scene->SelectActorById(actors[i]->GetId(), !IO().KeyShift);
            }
        }

        ImGui::End();
    }

    void Gui::SceneView()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin(ICON_FK_TH" Scene View", 0, ImGuiWindowFlags_NoScrollbar))
        {
            if (ImGui::IsWindowHovered())
            {
                const auto mousePos = ImGui::GetMousePos();
                const auto windowPos = ImGui::GetWindowPos();
                const ImVec2 windowMousePos { mousePos.x - windowPos.x, mousePos.y - windowPos.y - ImGui::GetFrameHeight() };

                m_scene->UpdateInput(windowMousePos);
            }

            const auto width = ImGui::GetWindowWidth();
            const auto height = ImGui::GetWindowHeight() - ImGui::GetFrameHeight();

            ReleaseSceneTexture();
            m_scene->Resize(static_cast<UINT>(width), static_cast<UINT>(height));
            m_scene->Render(m_renderDevice.GetDevice(), &m_sceneTexture);

            if (m_sceneTexture != nullptr)
            {
                ImGui::Image(m_sceneTexture, { width, height });
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void Gui::Console()
    {
        if (ImGui::Begin("Console", 0, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::SmallButton("Clear"))
                {
                    m_consoleText.clear();
                }

                ImGui::EndMenuBar();
            }

            ImGui::TextUnformatted(m_consoleText.c_str());
            if (m_moveConsoleToBottom)
            {
                ImGui::SetScrollHereY(1);
                m_moveConsoleToBottom = false;
            }
        }

        ImGui::End();
    }

    void Gui::Properties()
    {
        if (ImGui::Begin(ICON_FK_COG" Properties", 0, ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (m_scene->GetActors(true).size() > 0)
            {
                ActorProperties();
            }
        }

        ImGui::End();
    }

    void Gui::ActorProperties()
    {
        char name[100] { 0 };
        float position[3] { 0 };
        float rotation[3] { 0 };
        float scale[3] { 0 };
        auto actors = m_scene->GetActors(true);
        Actor *targetActor = NULL;

        if (actors.size() > 0)
        {
            // Show the properties of the last selected actor.
            targetActor = actors[actors.size() - 1];
            sprintf(name, targetActor->GetName().c_str());
            Util::ToFloat3(targetActor->GetPosition(), position);
            Util::ToFloat3(targetActor->GetEulerAngles(), rotation);
            Util::ToFloat3(targetActor->GetScale(), scale);
        }

        char tempName[100];
        sprintf(tempName, name);
        ImGui::InputText("Name", name, 100);

        D3DXVECTOR3 tempPos = D3DXVECTOR3(position);
        ImGui::InputFloat3("Position", position, "%g");

        D3DXVECTOR3 tempRot = D3DXVECTOR3(rotation);
        ImGui::InputFloat3("Rotation", rotation, "%g");

        D3DXVECTOR3 tempScale = D3DXVECTOR3(scale);
        ImGui::InputFloat3("Scale", scale, "%g");

        const auto groupId = Util::NewUuid();

        // Only apply changes to selected actors when one if it's properties has changed
        // to make all actors mutate as expected.
        for (int i = 0; i < actors.size(); i++)
        {
            if (strcmp(tempName, name) != 0)
            {
                m_scene->m_auditor.ChangeActor("Name Set", actors[i]->GetId(), groupId);
                actors[i]->SetName(std::string(name));
            }

            if (tempPos != D3DXVECTOR3(position))
            {
                auto curPos = actors[i]->GetPosition();
                m_scene->m_auditor.ChangeActor("Position Set", actors[i]->GetId(), groupId);
                actors[i]->SetPosition(D3DXVECTOR3(
                    tempPos.x != position[0] ? position[0] : curPos.x,
                    tempPos.y != position[1] ? position[1] : curPos.y,
                    tempPos.z != position[2] ? position[2] : curPos.z
                ));

                // Make sure the gizmo follows the target actor.
                if (actors[i] == targetActor)
                    m_scene->m_gizmo.Update(targetActor);
            }

            if (tempRot != D3DXVECTOR3(rotation))
            {
                auto curRot = actors[i]->GetEulerAngles();
                m_scene->m_auditor.ChangeActor("Rotation Set", actors[i]->GetId(), groupId);
                actors[i]->SetRotation(D3DXVECTOR3(
                    tempRot.x != rotation[0] ? rotation[0] : curRot.x,
                    tempRot.y != rotation[1] ? rotation[1] : curRot.y,
                    tempRot.z != rotation[2] ? rotation[2] : curRot.z
                ));

                // Make sure the gizmo follows the target actor.
                if (actors[i] == targetActor && !m_scene->m_gizmo.m_worldSpaceToggled)
                    m_scene->m_gizmo.Update(targetActor);
            }

            if (tempScale != D3DXVECTOR3(scale))
            {
                auto curScale = actors[i]->GetScale();
                m_scene->m_auditor.ChangeActor("Scale Set", actors[i]->GetId(), groupId);
                actors[i]->SetScale(D3DXVECTOR3(
                    tempScale.x != scale[0] ? scale[0] : curScale.x,
                    tempScale.y != scale[1] ? scale[1] : curScale.y,
                    tempScale.z != scale[2] ? scale[2] : curScale.z
                ));
            }
        }
    }

    void Gui::OptionsModal()
    {
        static int videoMode;
        static int buildCart;
        static int colorTheme;

        if (m_optionsModalOpen)
        {
            ImGui::OpenPopup("Options");

            videoMode = static_cast<int>(Settings::GetVideoMode());
            buildCart = static_cast<int>(Settings::GetBuildCart());
            colorTheme = static_cast<int>(Settings::GetColorTheme());

            m_optionsModalOpen = false;
        }

        if (ImGui::BeginPopupModal("Options", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::Combo("Color Theme", &colorTheme, "Dark\0Light\0\0");
            ImGui::Combo("Video Mode", &videoMode, "NTSC\0PAL\0\0");
            ImGui::Combo("Build Cart", &buildCart, "64drive\0EverDrive-64 X7\0\0");

            if (ImGui::Button("Save"))
            {
                Settings::SetColorTheme(static_cast<ColorTheme>(colorTheme));
                LoadColorTheme();

                Settings::SetVideoMode(static_cast<VideoMode>(videoMode));
                Settings::SetBuildCart(static_cast<BuildCart>(buildCart));

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Close"))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void Gui::SceneSettingsModal()
    {
        static float backgroundColor[3];
        static float gridSnapSize;

        if (m_sceneSettingsModalOpen)
        {
            ImGui::OpenPopup("Scene Settings");

            backgroundColor[0] = m_scene->m_backgroundColorRGB[0] / 255.0f;
            backgroundColor[1] = m_scene->m_backgroundColorRGB[1] / 255.0f;
            backgroundColor[2] = m_scene->m_backgroundColorRGB[2] / 255.0f;
            gridSnapSize = m_scene->m_gizmo.GetSnapSize();

            m_sceneSettingsModalOpen = false;
        }

        if (ImGui::BeginPopupModal("Scene Settings", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::ColorEdit3("Background Color", backgroundColor);
            ImGui::InputFloat("Grid Snap Size", &gridSnapSize);

            if (ImGui::Button("Save"))
            {
                m_scene->SetBackgroundColor(RGB(backgroundColor[0] * 255, backgroundColor[1] * 255,
                    backgroundColor[2] * 255));
                m_scene->SetGizmoSnapSize(gridSnapSize);

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Close"))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void Gui::ContextMenu()
    {
        if (m_openContextMenu)
        {
            ImGui::OpenPopup("Context Menu");
            m_openContextMenu = false;
        }

        if (ImGui::BeginPopup("Context Menu"))
        {
            if (ImGui::MenuItem("Edit Script"))
            {
                m_textEditorOpen = true;
                m_textEditor.SetText(m_scene->GetScript());
            }

            if (ImGui::BeginMenu("Texture"))
            {
                if (ImGui::MenuItem("Add"))
                {
                    m_addTextureModalOpen = true;
                }

                if (m_selectedActor != NULL && reinterpret_cast<Model *>(m_selectedActor)->HasTexture())
                {
                    ImGui::Separator();

                    if (ImGui::MenuItem("Delete"))
                    {
                        m_scene->DeleteTexture();
                    }
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Collider"))
            {
                if (ImGui::MenuItem("Box"))
                {
                    m_scene->AddCollider(ColliderType::Box);
                }

                if (ImGui::MenuItem("Sphere"))
                {
                    m_scene->AddCollider(ColliderType::Sphere);
                }

                if (m_selectedActor != NULL && m_selectedActor->HasCollider())
                {
                    ImGui::Separator();

                    if (ImGui::MenuItem("Delete"))
                    {
                        m_scene->DeleteCollider();
                    }
                }

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Delete"))
            {
                m_scene->Delete();
            }

            if (ImGui::MenuItem("Duplicate"))
            {
                m_scene->Duplicate();
            }

            ImGui::EndPopup();
        }
    }

    void Gui::ScriptEditor()
    {
        if (!m_textEditorOpen)
            return;

        ImGui::Begin(ICON_FK_CODE" Script Editor", &m_textEditorOpen, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem(ICON_FK_FLOPPY_O" Save Changes"))
                {
                    m_scene->SetScript(m_textEditor.GetText());
                }

                if (ImGui::MenuItem("Close"))
                {
                    m_textEditorOpen = false;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        m_textEditor.Render("Edit Script");

        ImGui::End();
    }

    void Gui::NewProjectModal()
    {
        static char projectName[64] { '\0' };
        static char projectPath[MAX_PATH] { '\0' };
        static bool createDirectory = true;

        if (m_newProjectModalOpen)
        {
            ImGui::OpenPopup("New Project");
            memset(projectName, 0, strlen(projectName));
            memset(projectPath, 0, strlen(projectPath));
            createDirectory = true;
            m_newProjectModalOpen = false;
        }

        if (ImGui::BeginPopupModal("New Project", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
        {
            if (m_folderBrowser.HasSelected())
            {
                strcpy(projectPath, m_folderBrowser.GetSelected().string().c_str());
                m_folderBrowser.ClearSelected();
            }

            ImGui::Text("Name");
            ImGui::SameLine();
            ImGui::InputTextWithHint("##projectName", "required", projectName, 64);

            ImGui::Text("Path");
            ImGui::SameLine();
            ImGui::InputTextWithHint("##projectPath", "required", projectPath, MAX_PATH);
            ImGui::SameLine();

            if (ImGui::Button("Choose..."))
            {
                m_folderBrowser.SetTitle("New Project");
                m_folderBrowser.Open();
            }

            ImGui::Checkbox("Create directory?", &createDirectory);

            if (ImGui::Button("Create") && strlen(projectName) > 0 && strlen(projectPath) > 0)
            {
                try
                {
                    Project::New(projectName, projectPath, createDirectory);
                    m_scene->New();
                }
                catch (const std::exception &e)
                {
                    Debug::Instance().Error(e.what());
                }

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }

            m_folderBrowser.Display();

            ImGui::EndPopup();
        }
    }

    void Gui::LoadProjectModal()
    {
        if (m_loadProjectModalOpen)
        {
            m_folderBrowser.Display();

            if (m_folderBrowser.HasSelected())
            {
                try
                {
                    Project::Load(m_folderBrowser.GetSelected());
                    m_scene->New();
                    m_folderBrowser.Close();
                }
                catch (const std::exception &e)
                {
                    Debug::Instance().Error(e.what());
                }
            }

            m_loadProjectModalOpen = m_folderBrowser.IsOpened();
        }
    }

    void Gui::AddTextureModal()
    {
        static bool modalOpen;

        if (m_addTextureModalOpen)
        {
            ImGui::OpenPopup("Add Texture");
            m_addTextureModalOpen = false;
            modalOpen = true;
        }

        if (ImGui::BeginPopupModal("Add Texture", &modalOpen))
        {
            int i = 0;
            auto textures = Project::Previews(AssetType::Texture, m_renderDevice.GetDevice());
            int rowLimit = static_cast<int>(std::max(1.0f, ImGui::GetWindowContentRegionWidth() / ModelPreviewer::PreviewWidth));

            for (const auto &texture : textures)
            {
                if (texture.second == NULL) continue;

                ImGui::PushID(i++);
                if (ImGui::ImageButton(texture.second, ImVec2(ModelPreviewer::PreviewWidth, ModelPreviewer::PreviewWidth)))
                {
                    m_scene->AddTexture(texture.first);
                    ImGui::CloseCurrentPopup();
                }

                if ((i % rowLimit) != 0) ImGui::SameLine();
                ImGui::PopID();
            }

            ImGui::EndPopup();
        }
    }

    void Gui::AddModelModal()
    {
        static bool modalOpen;

        if (m_addModelModalOpen)
        {
            ImGui::OpenPopup("Add Model");
            m_addModelModalOpen = false;
            modalOpen = true;
        }

        if (ImGui::BeginPopupModal("Add Model", &modalOpen))
        {
            int i = 0;
            auto models = Project::Previews(AssetType::Model, m_renderDevice.GetDevice());
            int rowLimit = static_cast<int>(std::max(1.0f, ImGui::GetWindowContentRegionWidth() / ModelPreviewer::PreviewWidth));

            for (const auto &model : models)
            {
                if (model.second == NULL) continue;

                ImGui::PushID(i++);
                if (ImGui::ImageButton(model.second, ImVec2(ModelPreviewer::PreviewWidth, ModelPreviewer::PreviewWidth)))
                {
                    m_scene->AddModel(model.first);
                    ImGui::CloseCurrentPopup();
                }

                if ((i % rowLimit) != 0) ImGui::SameLine();
                ImGui::PopID();
            }

            ImGui::EndPopup();
        }
    }

    void Gui::LoadSceneModal()
    {
        if (m_loadSceneModalOpen)
        {
            m_fileBrowser.Display();

            if (m_fileBrowser.HasSelected())
            {
                try
                {
                    m_scene->Load(m_fileBrowser.GetSelected());
                    m_fileBrowser.Close();
                }
                catch (const std::exception &e)
                {
                    Debug::Instance().Error(e.what());
                }
            }

            m_loadSceneModalOpen = m_fileBrowser.IsOpened();
        }
    }

    void Gui::SaveSceneModal()
    {
        if (std::get<0>(m_saveSceneModalOpen))
        {
            m_fileBrowser.Display();

            if (m_fileBrowser.HasSelected())
            {
                try
                {
                    m_scene->SaveAs(m_fileBrowser.GetSelected());
                    m_fileBrowser.Close();
                    std::get<1>(m_saveSceneModalOpen)();
                }
                catch (const std::exception &e)
                {
                    Debug::Instance().Error(e.what());
                }
            }

            std::get<0>(m_saveSceneModalOpen) = m_fileBrowser.IsOpened();
        }
    }

    void Gui::ConfirmSceneModal()
    {
        if (std::get<0>(m_openConfirmSceneModal))
        {
            ImGui::OpenPopup("Are you sure?");
            std::get<0>(m_openConfirmSceneModal) = false;
        }

        if (ImGui::BeginPopupModal("Are you sure?", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::Text("Would you like to save your changes?");

            if (ImGui::Button("Yes"))
            {
                if (m_scene->HasPath())
                {
                    // Has already been saved so just save and run callback.
                    m_scene->SaveAs();
                    std::get<1>(m_openConfirmSceneModal)();
                }
                else
                {
                    // Open save scene modal and forward defined callback.
                    m_saveSceneModalOpen = std::make_tuple(true, std::get<1>(m_openConfirmSceneModal));
                    m_fileBrowser.SetTitle("Save Scene As...");
                    m_fileBrowser.Open();
                }

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("No") || !m_scene->IsDirty())
            {
                std::get<1>(m_openConfirmSceneModal)();
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}
