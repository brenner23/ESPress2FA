#pragma once

// ╔══════════════════════════════════════════════════════════╗
// ║  FUNC_BATTERY.h  –  battery level (since 1.2.0)          ║
// ║  Pins per Meshtastic variants/esp32s3/                   ║
// ║  heltec_vision_master_t190/variant.h:                    ║
// ║   ADC_CTRL 46 (HIGH = measurement on), BATTERY_PIN 6,    ║
// ║   ADC_MULTIPLIER 4.9 * 1.03, attenuation 2.5 dB          ║
// ╚══════════════════════════════════════════════════════════╝

// Pins + divider per board in BOARD.h (BAT_ADC_CTRL -1 = no enable pin)

volatile int batMv  = 0;     // battery voltage in mV (0 = not measured yet)
volatile int batPct = -1;    // 0..100, -1 = unknown / no battery

// Li-ion curve (resting voltage -> percent), linear in between
static int batPercentFromMv(int mv) {
    static const int T[][2] = { {4200,100}, {4100,90}, {4000,80}, {3900,65}, {3800,50},
                                {3700,30},  {3600,15}, {3500,5},  {3300,0} };
    if (mv >= T[0][0]) return 100;
    for (int i = 1; i < 9; i++) {
        if (mv >= T[i][0]) {
            const int v0 = T[i][0], v1 = T[i - 1][0], p0 = T[i][1], p1 = T[i - 1][1];
            return p0 + (mv - v0) * (p1 - p0) / (v1 - v0);
        }
    }
    return 0;
}

void batteryInit() {
    if (BAT_ADC_PIN < 0) return;              // board without battery (Dongle)
    if (BAT_ADC_CTRL >= 0) { pinMode(BAT_ADC_CTRL, OUTPUT); digitalWrite(BAT_ADC_CTRL, LOW); }
    analogSetPinAttenuation(BAT_ADC_PIN, BAT_ATTEN);
}

// Measure every 10 s (divider is only switched on briefly to save battery)
void batteryUpdate(bool force = false) {
    static unsigned long last = 0;
    if (BAT_ADC_PIN < 0) { batPct = -1; batMv = 0; return; }
    if (!force && last && millis() - last < 10000) return;
    last = millis();
    if (BAT_ADC_CTRL >= 0) { digitalWrite(BAT_ADC_CTRL, HIGH); delay(3); }
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(BAT_ADC_PIN);
    if (BAT_ADC_CTRL >= 0) digitalWrite(BAT_ADC_CTRL, LOW);
    const int mv = (int)((sum / 16) * BAT_MULT);
    batMv = batMv ? (batMv * 3 + mv) / 4 : mv;     // light smoothing
    batPct = (batMv < 2500) ? -1 : batPercentFromMv(batMv);   // < 2.5 V = no battery detected
}
