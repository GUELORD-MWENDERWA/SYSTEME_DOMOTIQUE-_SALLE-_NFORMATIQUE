#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// PINS DÉFINITION
#define PZEM_RX_PIN 32
#define PZEM_TX_PIN 33
#define PZEM_BAUDRATE 9600

#define RFID_CLK_PIN 18
#define RFID_MISO_PIN 19
#define RFID_MOSI_PIN 23
#define RFID_RST_PIN 16
#define RFID_SS1_PIN 17
#define RFID_SS2_PIN 5

#define HC595_LATCH_PIN 27
#define HC595_CLOCK_PIN 14
#define HC595_DATA_PIN 13

#define SERVO_PIN 25

#define BUTTON_MODE_PIN 4
#define BUTTON_LAMP_PIN 15

#define LDR_PIN 35
#define MOTION_SENSOR_PIN 12  // PIR / motion detector input

#define LCD_SDA_PIN 21
#define LCD_SCL_PIN 22
#define LCD_I2C_ADDR 0x27

#define SERIAL_BAUDRATE 115200

// SORTIES 74HC595 (Q0-Q7)
#define Q0_LAMP_INSIDE   7
#define Q1_LAMP_OUTSIDE  6
#define Q2_PRISE1 5
#define Q3_PRISE2 4
#define Q4_FAN 3
#define Q5_BUZZER 2
#define Q6_LED_RED 1
#define Q7_LED_GREEN 0

// TIMEOUTS
#define BUTTON_LONG_PRESS_MS 2000
#define BUTTON_DEBOUNCE_MS 50
#define SERVO_OPEN_DURATION_MS 3000
#define LED_ALERT_DURATION_MS 3000

// EEPROM ADDRESSES FOR WIFI CONFIG
#define EEPROM_ADDR_SSID 1024      // 32 bytes for SSID
#define EEPROM_ADDR_PASSWORD 1056  // 64 bytes for password
#define EEPROM_ADDR_WIFI_VALID 1120 // 1 byte flag

// WiFi DEFAULT CREDENTIALS
#define WIFI_DEFAULT_SSID "702SH_AP"
#define WIFI_DEFAULT_PASSWORD "12345678___1"
#define WIFI_CONNECT_TIMEOUT_MS 8000   // 8 seconds timeout
#define WIFI_POLLING_INTERVAL_MS 50    // Poll WiFi every 50ms (fast)
#define WIFI_AUTO_RECONNECT_ENABLED 1  // Auto-reconnect on disconnect
#define LCD_REFRESH_MS 1000
#define PZEM_READ_INTERVAL_MS 2000
#define LDR_DAY_THRESHOLD 1500
#define LDR_DAY_THRESHOLD_LOW 1400
#define LDR_DAY_THRESHOLD_HIGH 1600
#define LDR_READ_INTERVAL_MS 1000
#define LDR_STABLE_MS 2000
#define MOTION_DEBOUNCE_MS 200
#define MOTION_COOLDOWN_MS 5000

#define MAX_REGISTERED_CARDS 10
#define EEPROM_SIZE 1024

// EEPROM storage layout
#define EEPROM_MAGIC 0x444F4D4F  // 'DOMO'
#define EEPROM_VERSION 2
#define EEPROM_STATE_ADDR 0
#define EEPROM_CARD_BASE_ADDR 256
#define EEPROM_ENERGY_ADDR 512
#define EEPROM_SAVE_INTERVAL_MS 60000

// ENUMS
enum SystemMode {
    MODE_ACCESS = 0,
    MODE_REGISTRATION = 1
};

enum SystemState_Enum {
    STATE_IDLE = 0,
    STATE_CARD_SCANNED = 1,
    STATE_ACCESS_GRANTED = 2,
    STATE_ACCESS_DENIED = 3,
    STATE_NIGHT_MODE = 4,
    STATE_INTRUSION_ALERT = 5
};

enum DayNightStatus {
    STATUS_DAY = 0,
    STATUS_NIGHT = 1
};

enum AccessStatus {
    ACCESS_GRANTED = 0,
    ACCESS_DENIED = 1,
    CARD_UNKNOWN = 2
};

enum RFIDScanType {
    RFID_ENTRY = 0,
    RFID_EXIT = 1
};

// STRUCTURES
struct CardData {
    uint32_t uid;
    uint32_t timestamp;
    bool authorized;
    bool isInside; // Tracks whether this card is currently 'inside' (entry/exit)
    char name[16]; // Optional name/label for the card
} __attribute__((packed));

struct SystemCounters {
    uint32_t entries_count;
    uint32_t exits_count;
    uint32_t presence_count;
    uint32_t total_access_attempts;
    uint32_t denied_access_count;
};

struct EnergyData {
    float voltage;
    float current;
    float power;
    float reactive_power;
    float power_factor;
    float frequency;
    float energy_total;
    uint32_t timestamp;
};

struct RelayState {
    bool q0_lamp_inside;
    bool q1_lamp_outside;
    bool q2_prise1;
    bool q3_prise2;
    bool q4_fan;
    bool q5_buzzer;
    bool q6_led_red;
    bool q7_led_green;
}; 

struct SystemStatus {
    SystemMode current_mode;
    SystemState_Enum state;
    DayNightStatus daynight;
    SystemCounters counters;
    RelayState relays;
    bool intrusion_detected;
    uint32_t last_rfid_scan;
    uint32_t state_transition_time;
};

// FORWARD DECLARATIONS
void debugLog(const char* format, ...);
String uint32ToHex(uint32_t value);
String getFormattedTime();
void performSystemTest();
void updateLCDDisplay();

#endif