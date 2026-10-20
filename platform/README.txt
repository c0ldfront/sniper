Sniper 87: reconstructed Win32 client

Build from the repository root (assets first; see ../README.md):
    make assets SNIPER_ZIP=/path/to/sniper87.zip
    make app
Output: platform/build/sniper87-recreated.exe

Run with platform/build as the working directory so LICENSE.TXT, LOCAL.LVL and
the other data files are found. The Win32 build requires Windows or Wine. The
native Linux client under ../linux uses the portable session code directly; it
does not load this executable.

Implemented application paths include settings, ship/spectator selection,
offline gameplay, DirectDraw presentation, chat/radar/statistics/help panels,
DirectSound audio, macro editing, Help Contents and the hidden level editor.
Online paths include connection/login/download, gameplay, scores/squad/news/
update commands, received files and recorded voice bindings.
Enter the editor with Ctrl+Shift+Play Game (command 40009). It edits the
working LOCAL.LVL and saves on exit using atomic replacement.

The original resources (bitmaps, dialogs, icons, menu, sounds) are compiled in
from the generated build/assets/sniper_assets.rc with their original type, ID
and language; none are stored in this repository. Portable game code lives in
../src; the DirectDraw/DirectSound and Win32 adapters live here. External
bitmap overrides use the original filenames; malformed overrides do not
silently fall back to the built-in resources.

Known limits: after a real focus loss DirectDraw may report WRONGMODE when
restoring the display; the client keeps the session and retries each frame,
as the original does. Help image display depends on the WinHelp viewer.
Windows needs the executable's imported runtime libraries (UCRT), legacy
DirectDraw/DirectSound and a compatible WinHelp viewer.
