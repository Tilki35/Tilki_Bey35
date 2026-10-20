#!/usr/bin/env bash
# Unprivileged Debian 13 setup. APT verifies signed indexes and package hashes.
set -euo pipefail
task_tools=/workspace/toolchain
mkdir -p "$task_tools/apt/lists/partial" "$task_tools/apt/cache/archives/partial" "$task_tools/root"
printf '%s\n' 'deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian trixie main' > "$task_tools/apt/sources.list"
apt_options=(-o Debug::NoLocking=1 -o Dir::Etc::parts=- -o Dir::Etc::main=-
    -o "Dir::Etc::sourcelist=$task_tools/apt/sources.list" -o Dir::Etc::sourceparts=-
    -o "Dir::State::lists=$task_tools/apt/lists" -o "Dir::Cache=$task_tools/apt/cache")
apt-get "${apt_options[@]}" update
apt-get "${apt_options[@]}" --download-only -y install \
    libvulkan-dev mesa-vulkan-drivers glslang-tools vulkan-tools \
    g++-mingw-w64-x86-64-posix vulkan-validationlayers xvfb xauth
for archive in "$task_tools"/apt/cache/archives/*.deb; do
    dpkg-deb -x "$archive" "$task_tools/root"
done
python3 - <<'PY'
import json
from pathlib import Path
root = Path('/workspace/toolchain/root')
icd = json.loads((root/'usr/share/vulkan/icd.d/lvp_icd.json').read_text())
icd['ICD']['library_path'] = str(root/'usr/lib/x86_64-linux-gnu/libvulkan_lvp.so')
Path('/workspace/toolchain/lvp-local.json').write_text(json.dumps(icd))
layers = Path('/workspace/toolchain/layers')
layers.mkdir(exist_ok=True)
layer = json.loads((root/'usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json').read_text())
layer['layer']['library_path'] = str(root/'usr/lib/x86_64-linux-gnu/libVkLayer_khronos_validation.so')
(layers/'VkLayer_khronos_validation.json').write_text(json.dumps(layer))
PY
cat > "$task_tools/env.sh" <<'SH'
export PATH=/workspace/toolchain/root/usr/bin:$PATH
export VULKAN_HEADERS=/workspace/toolchain/root/usr/include
export CXX_WINDOWS=x86_64-w64-mingw32-g++-posix
export MESA_SHADER_CACHE_DIR=/workspace/toolchain/mesa-cache
export VK_LAYER_PATH=/workspace/toolchain/layers
export PYTHONPATH=/workspace/toolchain/desktop-python:${PYTHONPATH:-}
# Headless cloud machines use software Vulkan only for correctness tests.
if [ ! -d /dev/dri ] && [ -z "${VK_DRIVER_FILES:-}" ]; then
    export VK_DRIVER_FILES=/workspace/toolchain/lvp-local.json
fi
SH
python3 -c 'import tkinter'
if ! python3 -c 'import PIL,psutil; assert PIL.__version__=="12.3.0"; assert psutil.__version__=="7.2.2"' >/dev/null 2>&1; then
    PIP_CACHE_DIR="$task_tools/pip-cache" python3 -m pip install --upgrade --target "$task_tools/desktop-python" -r /workspace/Tilki_Bey35/requirements-desktop.txt
fi
printf '%s\n' 'Setup complete. Run: source /workspace/toolchain/env.sh'
