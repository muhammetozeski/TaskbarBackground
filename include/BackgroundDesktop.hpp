#pragma once
#include <windows.h>
#include <shobjidl.h>
#include <servprov.h>
#include <winrt/base.h>
#include <string>
#include <vector>

namespace BackgroundDesktop {
using winrt::check_hresult;
using winrt::com_ptr;
inline constexpr GUID ManagerId{0x53f5ca0b,0x158f,0x4124,{0x90,0x0c,0x05,0x71,0x58,0x06,0x0b,0x27}};
inline constexpr GUID ViewCollectionId{0x1841c6d7,0x4f9d,0x42c0,{0xaf,0x41,0x87,0x47,0x53,0x8f,0x10,0xe5}};
inline constexpr GUID DesktopId{0x3f07f4be,0xb107,0x441a,{0xaf,0x0f,0x39,0xd8,0x25,0x29,0x07,0x2c}};

// These interfaces match the Windows 11 shell contracts used by RunInBackground.
struct ApplicationView : IUnknown {};
struct ViewCollection : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetViews(IObjectArray**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewsByZOrder(IObjectArray**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewsByAppUserModelId(PCWSTR, IObjectArray**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewForHwnd(HWND, ApplicationView**) = 0;
};
struct Desktop : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE IsViewVisible(ApplicationView*, BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetId(GUID*) = 0;
};
struct Manager : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetCount(UINT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveViewToDesktop(ApplicationView*, Desktop*) = 0;
    virtual HRESULT STDMETHODCALLTYPE CanViewMoveDesktops(ApplicationView*, BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentDesktop(Desktop**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesktops(IObjectArray**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdjacentDesktop(Desktop*, int, Desktop**) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktop(Desktop*) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDesktop(Desktop**) = 0;
};

/// Owns a process-query handle for one executable identity lookup.
struct ProcessHandle {
    HANDLE Value;
    explicit ProcessHandle(DWORD id) : Value(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, id)) {}
    ~ProcessHandle() { if (Value) CloseHandle(Value); }
    ProcessHandle(const ProcessHandle&) = delete;
    ProcessHandle& operator=(const ProcessHandle&) = delete;
};

/// Compares a window's process to the complete executable path supplied by the taskbar session.
inline bool MatchesExecutable(HWND window, const std::wstring& executable) {
    DWORD id{};
    GetWindowThreadProcessId(window, &id);
    ProcessHandle process(id);
    if (!process.Value) return false;
    wchar_t path[32768]{};
    DWORD size = ARRAYSIZE(path);
    return QueryFullProcessImageNameW(process.Value, 0, path, &size) &&
        _wcsicmp(path, executable.c_str()) == 0;
}

/// Includes a hosted application's child process without matching unrelated window titles.
inline bool MatchesHostedExecutable(HWND window, const std::wstring& executable) {
    if (MatchesExecutable(window, executable)) return true;
    struct Search { const std::wstring* Path; bool Found{}; } search{&executable};
    EnumChildWindows(window, [](HWND child, LPARAM value) -> BOOL {
        auto& state = *reinterpret_cast<Search*>(value);
        if (MatchesExecutable(child, *state.Path)) { state.Found = true; return FALSE; }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&search));
    return search.Found;
}

/// Moves all visible top-level windows of the selected application without launching a new process.
/// @param executable Exact executable identity resolved by TaskbarMenuButton.hpp.
/// @param desktopNumber One-based destination, normally two.
/// @return Number of windows whose destination was verified through the public desktop manager.
inline unsigned MoveApplication(const std::wstring& executable, unsigned desktopNumber = 2) {
    if (desktopNumber < 1 || desktopNumber > 20) winrt::throw_hresult(E_INVALIDARG);
    const GUID shellClass{0xc2f03a33, 0x21f5, 0x47fa, {0xb4, 0xbb, 0x15, 0x63, 0x62, 0xa2, 0xf2, 0x39}};
    const GUID managerService{0xc5e0cdca, 0x7b6e, 0x41b2, {0x9f, 0xc4, 0xd9, 0x39, 0x75, 0xcc, 0x46, 0x7b}};
    com_ptr<IServiceProvider> shell;
    check_hresult(CoCreateInstance(shellClass, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(shell.put())));
    com_ptr<Manager> manager;
    check_hresult(shell->QueryService(managerService, ManagerId, manager.put_void()));
    com_ptr<ViewCollection> views;
    check_hresult(shell->QueryService(ViewCollectionId, ViewCollectionId, views.put_void()));
    UINT count{};
    check_hresult(manager->GetCount(&count));
    while (count < desktopNumber) {
        com_ptr<Desktop> created;
        check_hresult(manager->CreateDesktop(created.put()));
        check_hresult(manager->GetCount(&count));
    }
    com_ptr<IObjectArray> desktops;
    check_hresult(manager->GetDesktops(desktops.put()));
    com_ptr<Desktop> target;
    check_hresult(desktops->GetAt(desktopNumber - 1, DesktopId, target.put_void()));
    GUID targetId{};
    check_hresult(target->GetId(&targetId));
    com_ptr<IVirtualDesktopManager> publicManager;
    check_hresult(CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(publicManager.put())));
    struct Search { const std::wstring* Path; std::vector<HWND> Windows; } search{&executable, {}};
    EnumWindows([](HWND window, LPARAM value) -> BOOL {
        auto& state = *reinterpret_cast<Search*>(value);
        if (IsWindowVisible(window) && GetWindow(window, GW_OWNER) == nullptr &&
            MatchesHostedExecutable(window, *state.Path)) state.Windows.push_back(window);
        return TRUE;
    }, reinterpret_cast<LPARAM>(&search));
    if (search.Windows.empty()) winrt::throw_hresult(HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
    unsigned moved{};
    HRESULT firstFailure = S_OK;
    for (HWND window : search.Windows) {
        com_ptr<ApplicationView> view;
        HRESULT result = views->GetViewForHwnd(window, view.put());
        BOOL movable{};
        if (SUCCEEDED(result)) result = manager->CanViewMoveDesktops(view.get(), &movable);
        if (SUCCEEDED(result) && !movable) result = E_ACCESSDENIED;
        if (SUCCEEDED(result)) {
            for (unsigned attempt = 0; attempt < 3; ++attempt) {
                result = manager->MoveViewToDesktop(view.get(), target.get());
                if (result != RPC_E_CALL_REJECTED && result != RPC_E_SERVERCALL_RETRYLATER) break;
                Wh_Log(L"Shell busy; move attempt %u failed: %08X", attempt + 1, static_cast<UINT>(result));
                if (attempt < 2) Sleep(15);
            }
        }
        GUID actual{};
        if (SUCCEEDED(result)) result = publicManager->GetWindowDesktopId(window, &actual);
        if (SUCCEEDED(result) && actual != targetId) result = E_UNEXPECTED;
        if (FAILED(result)) {
            Wh_Log(L"Window move failed; hwnd=%p; result=%08X", window, static_cast<UINT>(result));
            if (SUCCEEDED(firstFailure)) firstFailure = result;
        } else {
            ++moved;
            Wh_Log(L"Window moved; hwnd=%p; desktop=%u", window, desktopNumber);
        }
    }
    Wh_SetIntValue(L"LastMovedWindows", moved);
    check_hresult(firstFailure);
    return moved;
}
} // namespace BackgroundDesktop
