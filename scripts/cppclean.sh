#!/bin/sh

cd ../source/Erwin
cppclean --include-path=. --include-path=../vendor --include-path=../vendor/glm --include-path=../vendor/imgui --include-path=../vendor/imgui_filedialog --include-path=../vendor/taskflow --include-path=../vendor/glad/include --include-path=../vendor/ctti/include --include-path=../vendor/freetype/include --include-path=../vendor/entt/src .
