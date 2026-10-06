// ==WindhawkMod==
// @id taskbar-background
// @name Arkaplana Yolla
// @description Move an application's windows to the second virtual desktop from its taskbar menu.
// @version 1.0.1
// @author muhammetozeski
// @github https://github.com/muhammetozeski
// @include explorer.exe
// @include ShellExperienceHost.exe
// @architecture x86-64
// @compilerOptions -lshell32 -lshlwapi -luuid -lole32 -loleaut32 -lruntimeobject
// ==/WindhawkMod==

// ==WindhawkModSettings==
/*
- language: tr
  $name: Language / Dil
  $options:
  - tr: Türkçe
  - en: English
- desktopNumber: 2
  $name: Desktop number / Masaüstü numarası
  $description: One-based destination, from 1 to 20. Default is the second desktop.
*/
// ==/WindhawkModSettings==

#include "../include/TaskbarMenuButton.hpp"
#include "../include/BackgroundDesktop.hpp"

/// Registers the localized taskbar command and a native C++ move action.
BOOL Wh_ModInit() {
    const wchar_t* language = Wh_GetStringSetting(L"language");
    bool english = language && wcscmp(language, L"en") == 0;
    Wh_FreeStringSetting(language);
    return AddTaskbarButton(english ? L"Send to Background" : L"Arkaplana Yolla",
        [](const std::wstring& executable, const std::wstring& programName) {
            int number = Wh_GetIntSetting(L"desktopNumber");
            Wh_Log(L"Move requested; application=%s; executable=%s; desktop=%d",
                programName.c_str(), executable.c_str(), number);
            Wh_SetIntValue(L"LastMovedWindows", 0);
            unsigned moved = BackgroundDesktop::MoveApplication(executable, number);
            Wh_SetIntValue(L"LastOpenSucceeded", moved != 0);
        });
}

/// Refreshes the label and setting-backed callback without adding another menu button.
void Wh_ModSettingsChanged() { Wh_ModInit(); }
