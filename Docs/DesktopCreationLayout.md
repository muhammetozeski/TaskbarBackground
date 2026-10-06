# Missing desktop creation

The manager interface identity `53F5CA0B-158F-4124-900C-057158060B27` has `SwitchDesktopAndMoveForegroundView` between `SwitchDesktop` and `CreateDesktop` on the supported shell. See [Microsoft TypeAgent's interface declaration](https://github.com/microsoft/TypeAgent/blob/main/dotnet/autoShell/Services/WindowsVirtualDesktopService.cs).

The first extension version omitted that intervening entry. Existing-desktop moves worked because their methods precede the missing entry. Creating a missing desktop instead called the foreground-move method with an output-pointer argument. The user's missing-desktop attempt crashed Explorer; Windows recorded an access violation, `0xc0000005`, in `twinui.pcshell.dll` version 10.0.26100.9549.

Keep the intervening declaration even though the extension never calls it. `BackgroundDesktop::MoveApplication` first obtains the count and creates enough desktops to reach the configured one-based destination. After each creation it verifies that the count increased. Only then does it resolve the target desktop and move application windows. Diagnostic values `LastDesktopCountBefore`, `LastCreatedDesktops` and `LastDesktopCountAfter` distinguish creation from movement.

Runtime verification after the layout correction: the registry reported one desktop before the user action. The user confirmed creation of desktop two and successful movement. A later Chrome action found two desktops and reported one moved window with error zero.
