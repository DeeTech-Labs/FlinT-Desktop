[English translation](../en/BUILD.md)

# Сборка Flare

Зависимости Qt и обычные команды CMake описаны в [README](../../readme). CTest проверяет переносимый Runtime и настоящее приложение QML с временным автономным симулятором.

`platform/` — подмодуль [Flint IgniteSDK](https://github.com/DeeTech-Labs/Flint-IgniteSDK), закреплённый на коммите. Клонируйте с `--recurse-submodules` или выполните `git submodule update --init --recursive` перед конфигурацией. Общие изменения вносятся в SDK и подключаются обновлением ссылки подмодуля. Копирования и экспорта исходников нет.

Локальный Ignite собирается командой `python3 platform/tools/compile_ignite.py project.json output.ignit` или через Ignite Studio. Для исходных шрифтов и изображений нужен Pillow. Компилятор проверяет пути ресурсов относительно выбранного каталога проекта.

Отступ — четыре пробела. Формат задают `.editorconfig` и `.clang-format`. Иконки, общие компоненты QML, шрифты и шейдеры сохраняются как библиотека визуальных ресурсов.
