# ImGuiOverlay

A minimal DirectX11 + Win32 overlay using Dear ImGui, designed to be invisible to screen sharing and capture software (e.g., Zoom, Teams, Discord, OBS, etc.) on Windows 10/11.

## Features
- DirectX 11 rendering
- Win32 windowing
- Dear ImGui UI
- Invisible to most screen sharing/capture tools (via `SetWindowDisplayAffinity`)

## Prerequisites
- Windows 10 (version 2004+) or Windows 11
- Visual Studio (2019/2022) or MSVC Build Tools
- C++17 or newer

## Build Instructions

### 1. Clone the Repository
```
git clone https://github.com/yourusername/ImGuiOverlay.git
cd ImGuiOverlay
```

### 2. Open in VS Code (Recommended)
```
code .
```

### 3. Build with MSVC (Visual Studio Build Tools)
- Open the "x64 Native Tools Command Prompt for VS" from the Start Menu.
- Navigate to the project folder:
  ```
  cd path\to\ImGuiOverlay
  ```
- Launch VS Code from this prompt:
  ```
  code .
  ```
- Press `Ctrl+Shift+B` in VS Code to build.

### 4. Run
- Double-click `ImGuiOverlay.exe` or run from the terminal.

## Notes
- The overlay window will **not** appear in most screen sharing/capture software due to the use of `SetWindowDisplayAffinity(hwnd, 0x11)`.
- You can customize the ImGui UI in `main.cpp`.

## Troubleshooting
- If you get `cl.exe not found`, make sure you are using the Developer Command Prompt for Visual Studio.
- If you see linker errors for ImGui symbols, ensure you are using the correct `imgui/backends` source files and headers.
- For Windows 7 or old Windows 10, the capture exclusion may not work.

## Credits
- [ocornut/imgui](https://github.com/ocornut/imgui)

## License
This project is MIT licensed. See `imgui/LICENSE.txt` for Dear ImGui's license.

## System Audio to Text (Vosk)

This repository also includes a real-time system audio transcription tool using Vosk and PyInstaller.

### Prerequisites
- Windows 10/11
- Python 3.8+
- Vosk model (already included in `model/`)

### Python Usage
1. **Install dependencies:**
   ```sh
   pip install sounddevice vosk numpy
   ```
2. **Run the script:**
   ```sh
   python vosk_transcribe_to_file.py
   ```
   - To list audio devices: `python vosk_transcribe_to_file.py --list-devices`
   - To select a device: `python vosk_transcribe_to_file.py <device_index>`

### Building a Standalone EXE
1. **Ensure your `model/` directory is present in the project root.**
2. **Build with PyInstaller using the provided .spec file:**
   ```powershell
   pyinstaller vosk_transcribe_to_file.spec
   ```
   - This will bundle all Vosk dependencies and the model for offline use.
3. **Run the EXE:**
   - Find the output in the `dist/` folder: `dist/vosk_transcribe_to_file.exe`
   - The EXE will work standalone, no Python required.

### Troubleshooting
- If you see `No WASAPI loopback device found!`, your system may not support loopback or you may need to enable "Stereo Mix" in your sound settings.
- If you see `NameError: name 'exit' is not defined`, ensure you are using the latest code (uses `sys.exit`).
- If you get model not found errors, make sure the `model/` directory is present and included in the build.

### Credits
- [Vosk Speech Recognition Toolkit](https://alphacephei.com/vosk/)
- [PyInstaller](https://www.pyinstaller.org/)
