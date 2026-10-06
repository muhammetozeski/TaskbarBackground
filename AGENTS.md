# TaskbarBackground

Read README.md before changing code. This is a standalone native C++ Windhawk extension, independent of the C# launcher. Preserve existing taskbar menu commands. The default button is Arkaplana Yolla and the default target is desktop two.

Code, comments, technical logs and documentation are English. User-facing labels support Turkish and English. Keep generated distributions under publish and disposable probes in NewTemp. Preserve existing Windhawk settings when installing updates.

Read [the menu transport lesson](Docs/MenuActionTransport.md) before changing request dispatch. Do not perform shell COM calls inside the synchronous WM_COPYDATA handler; dispatch the copied request through the host message loop.
