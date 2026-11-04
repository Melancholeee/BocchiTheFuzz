# Bocchi the Fuzz

Вокальный фуз/скример + тон + хорус. VST3 и Standalone. Окно 640x480 (можно растягивать).

Ручки: Drive, Tone, Wide, Out. Двойной клик по ручке = сброс. При наведении подпись меняется на значение.

## Сборка на GitHub (ничего ставить не нужно)
1. Создай репозиторий на github.com, залей ВСЁ содержимое папки (включая .github).
   Если папка .github не загрузилась: Add file > Create new file, имя `.github/workflows/build.yml`, вставь содержимое.
2. Вкладка Actions > Build plugin. Через 5-15 минут внизу страницы в Artifacts будет BocchiTheFuzz-Windows.
3. Внутри: папка VST3 (скопируй `Bocchi the Fuzz.vst3` в `C:\Program Files\Common Files\VST3`) и Standalone (.exe для проверки без DAW).

## Локальная сборка (CMake + Visual Studio Build Tools)
    cmake -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release

## Где что крутить
- Звук: `Source/Params.h` (все числа с комментариями).
- Раскладка и смена картинки по Drive: `Source/PluginEditor.cpp`, namespace Layout.
- Вид ручек: `Source/GrungeLookAndFeel.h`.
- Картинки: `Assets/` (замена файлов с теми же именами не требует правок кода).
