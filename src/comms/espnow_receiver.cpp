// ============================================================================
// Suraksha+ Watch OS — ESP-NOW Receiver Implementation
// Receives heart rate data from the Seeed XIAO ESP32S3 sensor module
// ============================================================================
#include "espnow_receiver.h"

// Static instance for callback routing
static ESPNowReceiver* _callbackInstance = nullptr;

ESPNowReceiver& ESPNowReceiver::getInstance() {
    static ESPNowReceiver instance;
    return instance;
}

bool ESPNowReceiver::init() {
    if (_initialized) return true;

    Serial.println("[ESP-NOW] Initializing receiver...");

    // Create mutex for thread-safe data access
    _mutex = xSemaphoreCreateMutex();
    if (!_mutex) {
        Serial.println("[ESP-NOW] ❌ Failed to create mutex");
        return false;
    }

    // Store instance for static callback
    _callbackInstance = this;

    // Set WiFi to STA mode and disable auto-reconnect
    if (WiFi.getMode() == WIFI_OFF) {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
    }
    WiFi.setAutoReconnect(false);

    // Set WiFi channel to match the sender
    esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

    Serial.printf("[ESP-NOW] Watch MAC: %s\n", WiFi.macAddress().c_str());

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] ❌ esp_now_init() FAILED");
        return false;
    }

    // Register receive callback
    esp_now_register_recv_cb(onDataRecvCallback);

    _initialized = true;
    Serial.println("[ESP-NOW] ✅ Receiver ready (listening for HR sensor)");
    Serial.printf("[ESP-NOW] Channel: %d | Timeout: %dms\n",
                  ESPNOW_WIFI_CHANNEL, ESPNOW_TIMEOUT_MS);

    return true;
}

// ============================================================================
// Static callback — routes to instance method
// ============================================================================
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void ESPNowReceiver::onDataRecvCallback(const esp_now_recv_info_t *info,
                                         const uint8_t *data, int data_len) {
    if (_callbackInstance && info) {
        int8_t rssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;
        _callbackInstance->handleReceivedData(info->src_addr, data,
                                              data_len, rssi);
    }
}
#else
void ESPNowReceiver::onDataRecvCallback(const uint8_t *mac_addr,
                                         const uint8_t *data, int data_len) {
    if (_callbackInstance) {
        int8_t rssi = WiFi.RSSI();
        _callbackInstance->handleReceivedData(mac_addr, data,
                                              data_len, rssi);
    }
}
#endif

// ============================================================================
// Process received ESP-NOW packet
// ============================================================================
void ESPNowReceiver::handleReceivedData(const uint8_t *mac,
                                         const uint8_t *data,
                                         int len, int8_t rssi) {
    // Validate packet size
    if (len != sizeof(HRPacket_t)) {
        Serial.printf("[ESP-NOW] ⚠️ Wrong packet size: %d (expected %d)\n",
                      len, sizeof(HRPacket_t));
        return;
    }

    // Parse packet
    HRPacket_t packet;
    memcpy(&packet, data, sizeof(HRPacket_t));

    // Take mutex for thread-safe update
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;  // Couldn't get mutex, skip this packet
    }

    uint32_t now = millis();

    switch (packet.packetType) {
        case PACKET_TYPE_HEARTRATE:
            _data.bpm           = packet.bpm;
            _data.rawBpm        = packet.rawBpm;
            _data.spo2          = packet.spo2;
            _data.confidence    = packet.confidence;
            _data.fingerPresent = packet.fingerPresent;
            _data.lastReceived  = now;
            _data.senderBattery = packet.batteryPercent;
            _data.senderOnline  = true;
            _data.rssi          = rssi;
            _data.packetsReceived++;

            // Estimate packet loss from sender timestamps
            if (_lastSenderTimestamp > 0 && packet.timestamp > _lastSenderTimestamp) {
                uint32_t gap = packet.timestamp - _lastSenderTimestamp;
                if (gap > 2500) {  // Expected ~1000ms, >2500ms means missed packets
                    uint32_t missed = (gap / 1000) - 1;
                    _data.packetsLost += missed;
                }
            }
            _lastSenderTimestamp = packet.timestamp;
            break;

        case PACKET_TYPE_PING:
            // Keepalive — just update timestamp and status
            _data.lastReceived  = now;
            _data.senderOnline  = true;
            _data.senderBattery = packet.batteryPercent;
            _data.rssi          = rssi;
            _data.packetsReceived++;
            break;

        default:
            Serial.printf("[ESP-NOW] ⚠️ Unknown packet type: 0x%02X\n",
                          packet.packetType);
            break;
    }

    xSemaphoreGive(_mutex);

    // Debug output (throttled — only on HR data)
    if (packet.packetType == PACKET_TYPE_HEARTRATE) {
        if (packet.fingerPresent) {
            Serial.printf("[ESP-NOW] RX (LIVE): BPM=%.1f RSSI=%d\n", packet.bpm, rssi);
        } else {
            Serial.printf("[ESP-NOW] RX (MAINTAINED): BPM=%.1f RSSI=%d\n", packet.bpm, rssi);
        }
    }
}

// ============================================================================
// Thread-safe accessors
// ============================================================================

bool ESPNowReceiver::isSenderOnline() const {
    if (!_initialized) return false;
    return (millis() - _data.lastReceived) < ESPNOW_TIMEOUT_MS;
}

ESPNowHRData ESPNowReceiver::getData() const {
    ESPNowHRData copy = {};
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        copy = _data;
        // Update online status based on timeout
        copy.senderOnline = (millis() - _data.lastReceived) < ESPNOW_TIMEOUT_MS;
        xSemaphoreGive(_mutex);
    }
    return copy;
}

float ESPNowReceiver::getBPM() const {
    ESPNowHRData d = getData();
    return d.bpm; // Maintains last best reading even when finger is off
}

float ESPNowReceiver::getSpO2() const {
    return 0.0f; // SpO2 removed
}

bool ESPNowReceiver::isFingerPresent() const {
    ESPNowHRData d = getData();
    return d.senderOnline && d.fingerPresent;
}

int8_t ESPNowReceiver::getRSSI() const {
    return _data.rssi;
}

uint32_t ESPNowReceiver::getPacketsReceived() const {
    return _data.packetsReceived;
}
