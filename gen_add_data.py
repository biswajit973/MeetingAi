import os
assets_dir = ".venv\\Lib\\site-packages\\whisper\\assets"
for fname in os.listdir(assets_dir):
    print(f'--add-data "{assets_dir}\\{fname};whisper/assets" \\')