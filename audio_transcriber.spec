# -*- mode: python ; coding: utf-8 -*-


a = Analysis(
    ['audio_transcriber.py'],
    pathex=[],
    binaries=[],
    datas=[
        ('.venv\\Lib\\site-packages\\whisper\\assets\\mel_filters.npz', 'whisper/assets'),
        ('.venv\\Lib\\site-packages\\whisper\\assets\\multilingual.tiktoken', 'whisper/assets'),
        ('.venv\\Lib\\site-packages\\whisper\\assets\\gpt2.tiktoken', 'whisper/assets'),
    ],
    hiddenimports=['webrtcvad'],  # Added webrtcvad for PyInstaller
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name='audio_transcriber',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    onefile=True,  # Always build as a single-file executable
)
