# ImGuiOverlaySuite - Build & Setup Guide

This project contains:
- **audio_transcriber**: A Python-based real-time audio transcriber with Gemini AI Q&A.
- **SimpleGoodMorning**: A C++ ImGui-based desktop application.

## Prerequisites

### For Python (audio_transcriber)
- Python 3.8+ (recommended: 3.12)
- pip (Python package manager)
- [PyInstaller](https://pyinstaller.org/) (for building the standalone .exe)

### For C++ ImGui App
- CMake (3.15+ recommended)
- Visual Studio 2019/2022 (or another C++17+ compiler)

---

## 1. Build the Python Audio Transcriber

### a. Install Python dependencies
Open a terminal in the project root and run:
```sh
python -m venv .venv
.venv\Scripts\activate  # On Windows
pip install -r requirements.txt
```

If you don't have a `requirements.txt`, install manually:
```sh
pip install sounddevice numpy whisper webrtcvad soundfile google-generativeai
```

### b. Build the executable with PyInstaller
```sh
pyinstaller audio_transcriber.spec
```
- The output will be in the `dist/` folder as `audio_transcriber.exe`.
- Make sure your `.spec` file includes all required data files and hidden imports (see project for details).

---

## 2. Build the C++ ImGui App

### a. Configure and build with CMake
Open a terminal in the project root and run:
```sh
cmake -S . -B build
cmake --build build --config Debug
```
- The output executable (e.g., `SimpleGoodMorning.exe`) will be in `build/Debug/`.

### b. Run the app
```sh
build\Debug\SimpleGoodMorning.exe
```

---

## 3. Packaging for Distribution
- Use [PyInstaller](https://pyinstaller.org/) to bundle the Python app.
- Use [Inno Setup](https://jrsoftware.org/isinfo.php) or [NSIS](https://nsis.sourceforge.io/) to create a Windows installer that includes both executables and any required data files.

---

## 4. Troubleshooting
- If you see missing DLL errors for the C++ app, install the [Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist).
- If the Python app fails to run, ensure all dependencies are included in the PyInstaller build and that you have an internet connection for the first Whisper model download.

---

## 5. Running
- After building, you can run both apps directly from their output folders, or use the installer to set up desktop/start menu shortcuts.

---

For further help, see the comments in the code or contact the project maintainer.
