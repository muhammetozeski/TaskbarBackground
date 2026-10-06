# TaskbarBackground

![Windows](https://img.shields.io/badge/Windows-11_x64-0078D4)
![C++](https://img.shields.io/badge/C%2B%2B-23-00599C)

Move an application's open windows to another virtual desktop directly from its taskbar menu.

Right-click a running application on the taskbar and select **Arkaplana Yolla**. The English label is **Send to Background**. The default destination is the second virtual desktop. Windows remain available there; the action preserves their visible/minimized state and does not launch another copy of the application.

This native C++ Windhawk extension works independently of [RunInBackground](https://github.com/muhammetozeski/RunInBackground). It uses its own Windows shell interfaces and requires no .NET runtime.

## Requirements

- Windows 11 x64.
- Portable [Windhawk](https://windhawk.net/) already installed and running.
- `JumpViewUI.dll` version **10.0.26100.9549**, symbol identity **ADF87E5879A2571AC6566850798F0A6B1**. The menu header checks the identity before attaching its hooks.

## Installation

Download and extract [the release ZIP](https://github.com/muhammetozeski/TaskbarBackground/releases). From the extracted `TaskbarBackground` directory:

```powershell
pwsh -File .\Install.ps1
```

If Windhawk is outside PATH, supply its directory:

```powershell
pwsh -File .\Install.ps1 -WindhawkDirectory "C:\Path\To\Windhawk"
```

The installer registers the compiled module and embeds both headers into Windhawk's source copy. Existing language and desktop settings are retained. Disable the module in Windhawk to remove its menu command.

The release also includes `TaskbarBackground.wh.cpp`, a combined source file that can be used directly as a local Windhawk mod.

## Settings and diagnostics

Windhawk stores the extension's settings and logs in its portable data directory. Select `language=tr` or `en`; `desktopNumber=2` means the second desktop and valid numbers are 1 through 20. Enable Windhawk logging to inspect shell call results. The extension records the resolved executable, number of moved windows and last action error in its own local storage.

Executable matching uses the full process path, including child processes hosting packaged application windows. All visible top-level windows for the selected executable are moved. Unrelated titles and similarly named programs are not used as matches.

## Build

From this repository directory, run `pwsh -File .\Build.ps1`. Windhawk's bundled compiler produces `publish\AppData\TaskbarBackground.dll`; no additional packages are downloaded.

The project reuses the supplied `TaskbarMenuButton.hpp`. The shell action is queued after the menu's synchronous message returns, so desktop COM calls run from the host apartment's normal message loop. See [the transport lesson](Docs/MenuActionTransport.md) when changing the callback.

## Verified behavior

The command appeared in the native taskbar menu. After queuing the shell action outside the synchronous message, an ImageGlass window moved successfully to desktop two. The user confirmed the result; local action storage recorded one moved window and error zero.
