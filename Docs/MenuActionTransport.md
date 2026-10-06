# Taskbar action transport

The supplied header sends the requested path and program name from ShellExperienceHost to an Explorer apartment window with synchronous `WM_COPYDATA`. Calling shortcut resolution or virtual-desktop COM methods inside this callback produced **0x8001010D**, `RPC_E_CANTCALLOUT_ININPUTSYNCCALL`. The recorded failure occurred while selecting a pinned Chrome shortcut; no windows were moved.

`TaskbarMenu::Detail::HostProcedure` now validates and copies the request, posts `ExecuteActionMessage` and returns to the sender. The posted handler then resolves the executable and invokes the callback. This avoids COM calls while servicing the incoming synchronous message. Closing the host clears pending requests. The sender's return value acknowledges queuing; `LastOpenSucceeded`, `LastOpenError` and `LastMovedWindows` report the executed action separately.

Keep all shell operations, including shortcut resolution, in the posted handler. Retaining only the move itself there would leave shortcut resolution in the rejected synchronous context.
