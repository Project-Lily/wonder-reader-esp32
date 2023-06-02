#include "bluetooth.h"
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "NimBLEDevice.h"
#include "control/braillecontrol.h"
#include "sound.h"

static const char* TAG = "bluetooth";
static const char* GATTS_TAG = "GATT Server";
static const char* GAP_TAG = "GAP";

static NimBLEServer* pServer;

class BluetoothReadChunked: virtual public NimBLECharacteristicCallbacks {
  String _value;
  size_t _len;
  size_t _offset;
  size_t _chunkSize = 64;

  public:
  void setChunkedValue(char* value, size_t len) {
    _value = String(value);
    _len = len;
    _offset = 0;
  }

  void onRead(NimBLECharacteristic* pCharacteristic) {
    // Length to read
    size_t lenToRead = _chunkSize;
    if (_len < _offset + _chunkSize) {
      lenToRead = _len - _offset;
    }

    // Slice array
    String substr = _value.substring(_offset, _offset + lenToRead);
    if (lenToRead == _chunkSize) {
      pCharacteristic->setValue((uint8_t*)substr.c_str(), lenToRead);
    } else {
      const char* str_char = substr.c_str();
      char buf[substr.length() + 1];
      strcpy(buf, str_char);
      buf[substr.length()] = 0;
      pCharacteristic->setValue((uint8_t*)buf, substr.length() + 1);
    }

    _offset += _chunkSize;
    if (_offset > _len) {
      // Reset offset
      _offset = 0;
    }
  }
};

class BluetoothWriteChunked: virtual public NimBLECharacteristicCallbacks {
  String _builder;

  virtual void onChunkedWrite(const char* data) = 0;

  void onWrite(NimBLECharacteristic* pCharacteristic) {
    const char* data = pCharacteristic->getValue().c_str();
    ESP_LOGI(TAG, "Received |%s| len=%d lastchar=%c", data, strlen(data), data[strlen(data) - 1]);
    // Look for a terminator
    if (data[strlen(data) - 1] == 1) {
      _builder.concat(data, strlen(data) - 1);
      onChunkedWrite(_builder.c_str());
      _builder.clear();
    } else {
      _builder.concat(data);
    }
  }
};

class CharacteristicTest: public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic* pCharacteristic) {
    // Set characteristic as current time
    char buf[30];
    size_t len = sprintf(buf, "Cur time: %ld", esp_timer_get_time() / 1000);
    pCharacteristic->setValue((uint8_t*)buf, len);
    ESP_LOGI(TAG, "Characteristic read: %s", pCharacteristic->getUUID().toString().c_str());
    ESP_LOGI(TAG, "Sent: %s", buf);
  }
};

class CharacteristicAnswer: public BluetoothWriteChunked, public BluetoothReadChunked {
  void onChunkedWrite(const char* data) {
    ESP_LOGI(TAG, "Chunked write request: %s", data);
    wonder::display_text(data);
  }

  void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
    ESP_LOGI(TAG, "Subscribe event: %d", subValue);
    if (subValue == 2) {
      wonder::play_file("/bluetooth.wav");
    }
  }
};

static CharacteristicTest testCallback;
static CharacteristicAnswer answerCallback;
static NimBLECharacteristic* cAnswer;

void wonder::send_answer(char* answer, size_t len) {
  cAnswer->indicate((uint8_t*)"a", 1);
  answerCallback.setChunkedValue(answer, len);
  ESP_LOGI(TAG, "Indicate %s", answer);
}

void wonder::init_bt() {
  NimBLEDevice::init("Wonder Reader 2");
  pServer = NimBLEDevice::createServer();

  // Service for answers
  NimBLEService* sAnswer = pServer->createService("ABAB");
  NimBLECharacteristic* cTest = sAnswer->createCharacteristic(
    "BBBB",
    NIMBLE_PROPERTY::READ
  );

  cTest->setCallbacks(&testCallback);

  // Characteristic for all things questions and answers
  cAnswer = sAnswer->createCharacteristic(
    "CCCC",
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::INDICATE |
    NIMBLE_PROPERTY::WRITE,
    2048
  );

  cAnswer->setCallbacks(&answerCallback);

  sAnswer->start();

  // Advertising time
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID("ABAB");
  pAdvertising->start();
}