#include <Arduino.h>
#include <ArduinoJson.h>
#include <LoRa.h>
#include <SPI.h>

// Pin LoRa harus sama dengan wiring modul pada perangkat gateway.
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_CS    16
#define LORA_RST   17
#define LORA_DIO0  34
#define TRANSFER_LED_PIN 25
#define TRANSFER_LED_ON_MS 250

const long LORA_FREQUENCY_HZ = 923E6;
const int LORA_TX_POWER_DBM = 14;

bool loraReady = false;
unsigned long transferLedStartedAt = 0;

void tandaiTransferBerhasil() {
    digitalWrite(TRANSFER_LED_PIN, HIGH);
    transferLedStartedAt = millis();
}

void perbaruiLampuTransfer() {
    if (transferLedStartedAt != 0 &&
        millis() - transferLedStartedAt >= TRANSFER_LED_ON_MS) {
        digitalWrite(TRANSFER_LED_PIN, LOW);
        transferLedStartedAt = 0;
    }
}

void tampilkanPesanSensor(const String& payload, int packetSize) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.printf("[LoRa] Payload JSON tidak valid: %s\n", error.c_str());
        return;
    }

    const char* type = doc["type"] | "";
    if (strcmp(type, "raw_sensor") != 0) {
        Serial.printf("[LoRa] Tipe pesan tidak dikenal: %s\n", type);
        return;
    }

    Serial.println("\n[LoRa] Pesan sensor diterima");
    Serial.printf("  Sequence       : %lu\n", doc["seq"] | 0UL);
    Serial.printf("  Uptime sensor  : %lu ms\n", doc["uptime_ms"] | 0UL);
    Serial.printf("  Echo ultrasonik: %lu us\n", doc["sonar_echo_us"] | 0UL);
    Serial.printf("  Jarak sensor   : %.1f cm\n",
                  doc["sonar_distance_cm"] | 0.0f);
    Serial.printf("  Tip hujan      : %d\n", doc["rain_tip_count"] | 0);
    Serial.printf("  Ukuran paket   : %d byte | RSSI: %d dBm | SNR: %.1f dB\n",
                  packetSize, LoRa.packetRssi(), LoRa.packetSnr());
    tandaiTransferBerhasil();
    Serial.println("[LED] Transfer berhasil, indikator GPIO25 menyala.");
}

void terimaPesanLoRa() {
    int packetSize = LoRa.parsePacket();
    if (packetSize == 0) {
        return;
    }

    String payload;
    payload.reserve(packetSize);
    while (LoRa.available()) {
        payload += static_cast<char>(LoRa.read());
    }

    tampilkanPesanSensor(payload, packetSize);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    pinMode(TRANSFER_LED_PIN, OUTPUT);
    digitalWrite(TRANSFER_LED_PIN, LOW);
    Serial.println("\n==========================================");
    Serial.println("       EWS IoT GATEWAY - LoRa RECEIVER");
    Serial.println("==========================================");

    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(LORA_FREQUENCY_HZ)) {
        Serial.println("[LoRa] Gagal memulai radio. Gateway berhenti menerima.");
        return;
    }

    LoRa.setTxPower(LORA_TX_POWER_DBM);
    LoRa.setSpreadingFactor(7);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setCodingRate4(5);
    LoRa.receive();
    loraReady = true;

    Serial.printf("[LoRa] Siap menerima pada %ld Hz\n", LORA_FREQUENCY_HZ);
    Serial.println("[LoRa] Mode: SF7 | BW 125 kHz | CR 4/5");
}

void loop() {
    perbaruiLampuTransfer();
    if (loraReady) {
        terimaPesanLoRa();
    }
    delay(10);
}
