#ifndef FREAKY_CONFIG_H
#define FREAKY_CONFIG_H

// Pin definitions using WiringPi pin numbering
#define ESTOP_PIN      0   // WiringPi pin 0 (GPIO 17)
#define STATUS_LED_PIN 1   // WiringPi pin 1 (GPIO 18)

// Network settings
#define SERVER_PORT    8080
#define MAX_CLIENTS    5

// Timing settings (milliseconds)
#define HEARTBEAT_INTERVAL  1000
#define WATCHDOG_TIMEOUT    3000

#endif // FREAKY_CONFIG_H
