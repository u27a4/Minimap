#include "Minimap.h"
#include "AEUT.hpp"

static AEGP_Command         s_command = NULL;
static HWND                 s_panel = NULL;
static LONG_PTR             s_def_proc;
static SPBasicSuite         *s_pica_basicP;
static ColorPalette         s_color_palette;
static CompInfo             s_comp_info;

void
DrawMinimap(HWND hWnd)
{
    PAINTSTRUCT             paint;
    RECT                    rect;
    HBRUSH                  label_brush[AEGP_Label_NUMTYPES];

    auto hdc = BeginPaint(hWnd, &paint);

    auto bg_brush = CreateSolidBrush(s_color_palette.bg);
    auto needle_pen = CreatePen(PS_SOLID, 1, s_color_palette.needle);
    for (int i = 0; i < AEGP_Label_NUMTYPES; i++)
    {
        label_brush[i] = CreateSolidBrush(s_color_palette.labels[i]);
    }

    GetClientRect(hWnd, &rect);
    FillRect(hdc, &rect, bg_brush);

    SelectObject(hdc, GetStockObject(NULL_PEN));
    if (s_comp_info.durationT.scale != 0 && rect.right != 0)
    {
        auto ux = (float)rect.right / DECIMAL(s_comp_info.durationT);

        int i = 0;
        for (auto li : s_comp_info.layers)
        {
            if (li.in_pointT.scale > 0 && li.durationT.scale > 0 && (s_comp_info.show_shy || !li.shy))
            {
                int x1 = (int)std::floor(DECIMAL(li.in_pointT) * ux);
                int x2 = (int)std::floor((DECIMAL(li.in_pointT) + DECIMAL(li.durationT)) * ux);

                SelectObject(hdc, label_brush[li.lable_id]);
                Rectangle(hdc, x1, i * MINIMAP_LINE_WIDTH, x2, (i + 1) * MINIMAP_LINE_WIDTH);

                i++;
            }
        }

        SelectObject(hdc, needle_pen);
        MoveToEx(hdc, (int)std::floor(DECIMAL(s_comp_info.current_timeT) * ux), 0, NULL);
        LineTo(hdc, (int)std::floor(DECIMAL(s_comp_info.current_timeT) * ux), rect.bottom);
    }

    for (int i = 0; i < AEGP_Label_NUMTYPES; i++)
    {
        DeleteObject(label_brush[i]);
    }
    DeleteObject(bg_brush);
    DeleteObject(needle_pen);

    EndPaint(hWnd, &paint);
}

LRESULT CALLBACK MinimapProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    bool                    handledB = false;

    switch (msg)
    {
    case WM_PAINT:
        DrawMinimap(hWnd);
        handledB = true;
        return 0;

    case WM_SIZING:
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }

    if (s_def_proc && !handledB)
    {
        return CallWindowProc((WNDPROC)s_def_proc, hWnd, msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static A_Err
IdleHook(
    AEGP_GlobalRefcon plugin_refconP,               /* >> */
    AEGP_IdleRefcon refconP,                        /* >> */
    A_long *max_sleepPL)                            /* <> in 1/60 of a second*/
{
    A_Err                   err = A_Err_NONE;
    AEGP_SuiteHandler       suites(AEUT_GetSPBasicSuitePtr());
    SuiteHelper<AEGP_PanelSuite1> panel_suites(s_pica_basicP);
    std::list<AEGP_LayerH>  layers;
    AEGP_CompH              active_compH = NULL;
    AEGP_ItemH              itemH;
    AEGP_LayerFlags         layer_flags;
    AEGP_CompFlags          comp_flags;
    A_Boolean               out_tab, out_panel;
    PF_App_Color            color;
    LayerInfo               li;

    ERR(panel_suites->AEGP_IsShown((A_u_char*)MINIMAP_MATCH_NAME, &out_tab, &out_panel));

    if (out_tab && out_panel && s_panel)
    {
        for (auto i = 0; i < AEGP_Label_NUMTYPES; i++)
        {
            AEUT_GetLabelColor(i + 1, &s_color_palette.labels[i]);
        }

        ERR(suites.AppSuite6()->PF_AppGetColor(PF_App_Color_PANEL_BACKGROUND, &color));
        s_color_palette.bg = RGB(color.red, color.green, color.blue);
        ERR(suites.AppSuite6()->PF_AppGetColor(PF_App_Color_TLW_NEEDLE_CURRENT_TIME, &color));
        s_color_palette.needle = RGB(color.red, color.green, color.blue);

        ERR(AEUT_GetActiveComp(&active_compH));
        if (active_compH != NULL)
        {
            ERR(suites.CompSuite10()->AEGP_GetItemFromComp(active_compH, &itemH));
            ERR(suites.CompSuite10()->AEGP_GetCompFlags(active_compH, &comp_flags));
            ERR(suites.ItemSuite8()->AEGP_GetItemDuration(itemH, &s_comp_info.durationT));
            ERR(suites.ItemSuite8()->AEGP_GetItemCurrentTime(itemH, &s_comp_info.current_timeT));
            ERR(AEUT_GetActiveCompLayers(layers));

            s_comp_info.show_shy = !(comp_flags & AEGP_CompFlag_SHOW_ALL_SHY);

            s_comp_info.layers.clear();
            
            for (auto layerH : layers)
            {
                ERR(suites.LayerSuite8()->AEGP_GetLayerLabel(layerH, &li.lable_id));
                ERR(suites.LayerSuite8()->AEGP_GetLayerDuration(layerH, AEGP_LTimeMode_CompTime, &li.durationT));
                ERR(suites.LayerSuite8()->AEGP_GetLayerInPoint(layerH, AEGP_LTimeMode_CompTime, &li.in_pointT));
                ERR(suites.LayerSuite8()->AEGP_GetLayerFlags(layerH, &layer_flags));
                li.shy = layer_flags & AEGP_LayerFlag_SHY;

                s_comp_info.layers.push_back(li);
            }
        }

        InvalidateRect(s_panel, NULL, TRUE);
    }

    return err;
}

static A_Err
CommandHook(
    AEGP_GlobalRefcon       plugin_refconPV,        /* >> */
    AEGP_CommandRefcon      refconPV,               /* >> */
    AEGP_Command            command,                /* >> */
    AEGP_HookPriority       hook_priority,          /* >> */
    A_Boolean               already_handledB,       /* >> */
    A_Boolean               *handledPB)             /* << */
{
    A_Err                   err = A_Err_NONE;
    SuiteHelper<AEGP_PanelSuite1> panel_suites(s_pica_basicP);

    if (command == s_command)
    {
        ERR(panel_suites->AEGP_ToggleVisibility((A_u_char*)MINIMAP_MATCH_NAME));
        // *handledPB = TRUE;
    }

    return err;
}

static A_Err
UpdateMenuHook(
    AEGP_GlobalRefcon       plugin_refconPV,        /* >> */
    AEGP_UpdateMenuRefcon   refconPV,               /* >> */
    AEGP_WindowType         active_window)          /* >> */
{
    A_Err                   err = A_Err_NONE;
    SuiteHelper<AEGP_PanelSuite1> panel_suites(s_pica_basicP);
    A_Boolean               out_tab;
    A_Boolean               out_panel;

    if (s_command != NULL)
    {
        ERR(panel_suites->AEGP_IsShown((A_u_char*)MINIMAP_MATCH_NAME, &out_tab, &out_panel));
        ERR(AEUT_EnableCommand(s_command, true));
        ERR(AEUT_CheckMarkMenuCommand(s_command, out_tab && out_panel));
    }

    return err;
}

static A_Err
GetSnapSizes(AEGP_PanelRefcon refcon, A_LPoint* snapSizes, A_long * numSizesP)
{
    auto num_layers = std::count_if(s_comp_info.layers.begin(), s_comp_info.layers.end(),
        [](auto li) { return  s_comp_info.show_shy || !li.shy; });

    snapSizes[0].x = MINIMAP_SNAP_SIZE_X;
    snapSizes[0].y = (A_long)(MINIMAP_LINE_WIDTH * num_layers + 1);
    *numSizesP = 1;

    return A_Err_NONE;
}

static A_Err
PopulateFlyout(AEGP_PanelRefcon refcon, AEGP_FlyoutMenuItem* itemsP, A_long * in_out_numItemsP)
{
    return A_Err_NONE;
}

static A_Err
DoFlyoutCommand(AEGP_PanelRefcon refcon, AEGP_FlyoutMenuCmdID commandID)
{
    return A_Err_NONE;
}

A_Err
CreatePanelHook(
    AEGP_GlobalRefcon       plugin_refconP,
    AEGP_CreatePanelRefcon  refconP,
    AEGP_PlatformViewRef    container,
    AEGP_PanelH             panelH,
    AEGP_PanelFunctions1*   outFunctionTable,
    AEGP_PanelRefcon*       outRefcon)
{
    s_panel = container;
    s_def_proc = GetWindowLongPtr(container, GWLP_WNDPROC);
    SetWindowLongPtrA(container, GWLP_WNDPROC, (LONG_PTR)MinimapProc);

    outFunctionTable->DoFlyoutCommand = DoFlyoutCommand;
    outFunctionTable->GetSnapSizes = GetSnapSizes;
    outFunctionTable->PopulateFlyout = PopulateFlyout;

    return A_Err_NONE;
}

A_Err
EntryPointFunc(
    struct SPBasicSuite     *pica_basicP,           /* >> */
    A_long                  major_versionL,         /* >> */
    A_long                  minor_versionL,         /* >> */
    AEGP_PluginID           aegp_plugin_id,         /* >> */
    AEGP_GlobalRefcon       *global_refconP)        /* << */
{
    A_Err                   err = A_Err_NONE;
    AEGP_SuiteHandler       suites(pica_basicP);
    SuiteHelper<AEGP_PanelSuite1> panel_suites(pica_basicP);

    s_pica_basicP = pica_basicP;

    ERR(AEUT_Init(aegp_plugin_id, pica_basicP, MINIMAP_PREFS_SECTION));
    ERR(suites.CommandSuite1()->AEGP_GetUniqueCommand(&s_command));
    ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(s_command, "Minimap", AEGP_Menu_WINDOW, AEGP_MENU_INSERT_SORTED));
    ERR(suites.RegisterSuite5()->AEGP_RegisterUpdateMenuHook(aegp_plugin_id, UpdateMenuHook, NULL));
    ERR(suites.RegisterSuite5()->AEGP_RegisterIdleHook(aegp_plugin_id, IdleHook, NULL));
    ERR(suites.RegisterSuite5()->AEGP_RegisterCommandHook(aegp_plugin_id, AEGP_HP_BeforeAE, AEGP_Command_ALL, CommandHook, 0));
    ERR(panel_suites->AEGP_RegisterCreatePanelHook(aegp_plugin_id, (A_u_char*)MINIMAP_MATCH_NAME, CreatePanelHook, NULL, true));

    if (err != A_Err_NONE)
    {
        AEUT_ReportInfo("An error has occurred at enabling Minimap Plug-in");
    }

    return err;
}
