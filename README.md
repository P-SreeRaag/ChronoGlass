# ChronoGlass

A tiny, native Windows desktop time-clock widget inspired by minimalist lock-screen typography.

![Concept](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcSOb7oA5t9FQK_cMraSxx8kI_IS0QoJGio1-40CMZQ-Tw&s=10)

## Design goals

- **Very low overhead:** native C++ Win32, no Electron, no Chromium, no web runtime.
- **Minimal UI:** day, date and time with transparent background.
- **Desktop wallpaper layer:** attempts to attach to Explorer's WorkerW so it behaves like part of the desktop.
- **Click-through:** it does not steal mouse/keyboard input.
- **1-second update:** only repaints once per second.
- **Tray menu:** right-click the tray icon for seconds / 12-or-24-hour settings and exit.
- **Ctrl+Alt+click:** temporarily enables dragging so you can reposition the widget.
- **Windows 10/11.**

## Lock-screen limitation

A normal Win32 desktop window cannot reliably draw over the secure Windows sign-in/lock screen. Windows isolates the secure desktop for security. Windows 11 does support a limited, OS-controlled set of lock-screen widgets, but a custom always-visible third-party clock cannot simply be placed there like a normal desktop overlay.

ChronoGlass therefore focuses on the **desktop wallpaper layer**, where it can stay visible without consuming an application window.

## Build

Install Visual Studio 2022 with **Desktop development with C++**, plus CMake.

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The executable will be:

```text
build\Release\ChronoGlass.exe
```

## Auto-start

Create a shortcut to `ChronoGlass.exe` and put it in:

```text
shell:startup
```

## Why not Electron?

Electron bundles a browser engine and multiple helper processes. This project intentionally uses Win32/GDI so there is no Chromium runtime and no JavaScript engine.

## Notes

Explorer updates and Windows 11 builds can change the WorkerW desktop-layer behavior. If the clock ever appears behind or disappears, restart ChronoGlass or Explorer.

License: MIT
