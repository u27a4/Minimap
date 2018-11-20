#include "AEConfig.h"

#include "entry.h"

#ifdef AE_OS_WIN
    #include <windows.h>
#endif

#include <string>
#include <vector>

#include "AE_Macros.h"
#include "AE_EffectCBSuites.h"
#include "AE_GeneralPlug.h"
#include "AE_GeneralPlugPanels.h"
#include "AEGP_SuiteHandler.h"
#include "AE_EffectSuites.h"
#include "SuiteHelper.h"

#define MINIMAP_MATCH_NAME      "Minimap"
#define MINIMAP_PREFS_SECTION   "Minimap Preference Section"
#define MINIMAP_LINE_WIDTH      2
#define MINIMAP_MARGIN          3
#define MINIMAP_SNAP_SIZE_X     206

template <>
const A_char* SuiteTraits<AEGP_PanelSuite1>::i_name = kAEGPPanelSuite;
template <>
const int32_t SuiteTraits<AEGP_PanelSuite1>::i_version = kAEGPPanelSuiteVersion1;

struct ColorPalette
{
    A_u_long                labels[AEGP_Label_NUMTYPES];
    A_u_long                bg;
    A_u_long                needle;
};

struct LayerInfo
{
    A_Time                  durationT;
    A_Time                  in_pointT;
    AEGP_LabelID            lable_id;
    A_Boolean               shy;
};

struct CompInfo
{
    A_Time                  durationT;
    A_Time                  current_timeT;
    A_Boolean               show_shy;
    std::vector<LayerInfo>  layers;
};

// This entry point is exported through the PiPL (.r file)
extern "C" DllExport AEGP_PluginInitFuncPrototype EntryPointFunc;

void
ShowLastError(HWND hWnd)
{
    HLOCAL lpBuffer;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    NULL, GetLastError(), LANG_USER_DEFAULT, (LPTSTR)&lpBuffer, 0, NULL);
    MessageBox(hWnd, (LPTSTR)lpBuffer, "An Error Occurred", MB_OK | MB_ICONHAND);

    LocalFree(lpBuffer);
}