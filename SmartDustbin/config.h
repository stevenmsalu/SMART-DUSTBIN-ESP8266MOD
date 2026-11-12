#ifndef SMART_DUSTBIN_CONFIG_H
#define SMART_DUSTBIN_CONFIG_H

// Firmware defaults — physical calibration remains TBD (docs/CALIBRATION_LOG.md).

#define CONFIG_SERIAL_BAUD 115200

#define CONFIG_HEARTBEAT_MS 5000UL

// --- Approach sensor (WP-2) — provisional development defaults ---
#define CONFIG_APPROACH_DETECT_CM 50
#define CONFIG_APPROACH_CLEAR_CM 55
#define CONFIG_APPROACH_SAMPLE_MS 100UL
#define CONFIG_APPROACH_PULSE_TIMEOUT_US 30000UL
#define CONFIG_APPROACH_MIN_VALID_CM 2
#define CONFIG_APPROACH_MAX_VALID_CM 400
// round-trip time to cm: speed of sound ~343 m/s => ~58 µs per cm (round trip)
#define CONFIG_APPROACH_US_PER_CM 58UL

// --- Not yet implemented (later work packages) ---

// Waste level (WP-3+)
// #define CONFIG_WASTE_EMPTY_CM          TBD
// #define CONFIG_WASTE_FULL_CM           TBD
// #define CONFIG_WARN_FILL_PERCENT       TBD

// Servo (WP-5+)
// #define CONFIG_SERVO_ANGLE_OPEN        TBD
// #define CONFIG_SERVO_ANGLE_CLOSED      TBD

// Buzzer (later WP; GPIO2 not driven in WP-2)
// #define CONFIG_BUZZER_ALERT_MS         TBD
// #define CONFIG_BUZZER_COOLDOWN_MS      TBD

#endif
