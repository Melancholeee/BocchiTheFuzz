#pragma once
// ============================================================================
//  ВСЕ ЧИСЛА ЗВУКА ЗДЕСЬ. Подкручивай и пересобирай.
//  Подсказки: "грязнее" -> больше kMaxDriveDb / kBias; "мягче" -> меньше.
// ============================================================================
namespace Cfg
{
    // ---- ID параметров (так они называются в автоматизации DAW) ----
    inline constexpr const char* kIdDrive = "drive";
    inline constexpr const char* kIdTone  = "tone";
    inline constexpr const char* kIdWide  = "wide";
    inline constexpr const char* kIdOut   = "out";

    // ---- Значения по умолчанию ----
    inline constexpr float kDefDrive = 0.30f;   // 0..1  (на ручке 0..10)
    inline constexpr float kDefTone  = 0.50f;   // 0..1  (0.5 = нейтрально)
    inline constexpr float kDefWide  = 0.35f;   // 0..1
    inline constexpr float kDefOutDb = 0.0f;    // дБ

    // ---- DRIVE ----
    inline constexpr int   kOversampleLog2 = 2;       // 2 => 4x оверсэмплинг
    inline constexpr float kMaxDriveDb  = 40.0f;      // максимальный гейн перед клиппером
    inline constexpr float kBias        = 0.15f;      // асимметрия (чётные гармоники, "лампа")
    inline constexpr float kMixSpeed    = 3.0f;       // как быстро искажение подмешивается (3 = за первую треть ручки)
    inline constexpr float kCompExp     = 0.30f;      // компенсация громкости: 0 = нет, 0.5 = сильная

    // Предфильтр только на "грязной" ветке (как у Tube Screamer)
    inline constexpr float kPreHpHz     = 120.0f;     // срез низа перед искажением
    inline constexpr float kPreMidHz    = 900.0f;     // центр подъёма середины
    inline constexpr float kPreMidQ     = 0.7f;
    inline constexpr float kPreMidGain  = 2.0f;       // подъём середины (линейный множитель, 2.0 = +6 дБ)

    // ---- TONE (наклон спектра) ----
    inline constexpr float kTonePivotHz = 900.0f;
    inline constexpr float kToneRangeDb = 14.0f;      // полный размах: влево -7 дБ верха / +7 дБ низа, вправо наоборот

    // ---- WIDE (хорус) ----
    inline constexpr float kChorusBaseAms = 14.0f;    // задержка голоса A
    inline constexpr float kChorusBaseBms = 22.0f;    // задержка голоса B
    inline constexpr float kChorusDepthMs = 3.5f;     // глубина модуляции
    inline constexpr float kChorusRateA   = 0.45f;    // Гц
    inline constexpr float kChorusRateB   = 0.31f;    // Гц
    inline constexpr float kChorusMaxWet  = 0.60f;    // сколько "мокрого" при Wide = 1

    // ---- OUT ----
    inline constexpr float kOutMinDb = -24.0f;
    inline constexpr float kOutMaxDb =  12.0f;

    // Сглаживание ручек (секунды) против щелчков
    inline constexpr double kSmoothSec = 0.03;
}
