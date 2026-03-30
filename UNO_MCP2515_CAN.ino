/*
    UNO-compatible variant of the Tesla FSD CAN mod sketch.
    Keeps the original message logic while avoiding RP2040-specific APIs.
*/

#include <SPI.h>
#include <mcp2515.h>

#define LEGACY LegacyHandler
#define HW3 HW3Handler
#define HW4 HW4Handler // For pre-2026.2.3 HW4 firmware, compile as HW3.

#define HW HW3 // Change to LEGACY, HW3, or HW4

bool enablePrint = true;

// Arduino UNO + MCP2515 defaults.
#define LED_PIN 13
#define CAN_CS 10
#define CAN_INT_PIN 2 // Reserved for optional interrupt use.

// HW4 FSD V14 options
#define ENABLE_APPROACHING_EMERGENCY_VEHICLE_DETECTION true

MCP2515 mcp(CAN_CS);

struct CarManagerBase {
  int speedProfile = 1;
  bool FSDEnabled = false;
  virtual void handleMessage(can_frame &frame) = 0;
};

inline uint8_t readMuxID(const can_frame &frame) {
  return frame.data[0] & 0x07;
}

inline bool isFSDSelectedInUI(const can_frame &frame) {
  return ((frame.data[4] >> 6) & 0x01) != 0;
}

inline void setSpeedProfileV12V13(can_frame &frame, int profile) {
  frame.data[6] &= ~0x06;
  frame.data[6] |= (profile << 1);
}

inline void setBit(can_frame &frame, int bit, bool value) {
  int byteIndex = bit / 8;
  int bitIndex = bit % 8;
  uint8_t mask = static_cast<uint8_t>(1U << bitIndex);

  if (value) {
    frame.data[byteIndex] |= mask;
  } else {
    frame.data[byteIndex] &= static_cast<uint8_t>(~mask);
  }
}

void printHandlerStatus(const char *handlerName, bool fsdEnabled, int profile, int offset) {
  if (!enablePrint) {
    return;
  }

  Serial.print(handlerName);
  Serial.print(" FSD: ");
  Serial.print(fsdEnabled ? 1 : 0);
  Serial.print(", Profile: ");
  Serial.print(profile);

  if (offset >= 0) {
    Serial.print(", Offset: ");
    Serial.print(offset);
  }

  Serial.println();
}

struct LegacyHandler : public CarManagerBase {
  void handleMessage(can_frame &frame) override {
    if (frame.can_id != 1006) {
      return;
    }

    uint8_t index = readMuxID(frame);
    bool selected = isFSDSelectedInUI(frame);

    if (index == 0 && selected) {
      uint8_t off = static_cast<uint8_t>((frame.data[3] >> 1) & 0x3F) - 30;
      switch (off) {
        case 2:
          speedProfile = 2;
          break;
        case 1:
          speedProfile = 1;
          break;
        case 0:
          speedProfile = 0;
          break;
        default:
          break;
      }

      setBit(frame, 46, true);
      setSpeedProfileV12V13(frame, speedProfile);
      mcp.sendMessage(&frame);
    }

    if (index == 1) {
      setBit(frame, 19, false);
      mcp.sendMessage(&frame);
    }

    if (index == 0) {
      printHandlerStatus("LegacyHandler", selected, speedProfile, -1);
    }
  }
};

struct HW3Handler : public CarManagerBase {
  int speedOffset = 0;

  void handleMessage(can_frame &frame) override {
    if (frame.can_id == 1016) {
      uint8_t followDistance = static_cast<uint8_t>((frame.data[5] & 0b11100000) >> 5);
      switch (followDistance) {
        case 1:
          speedProfile = 2;
          break;
        case 2:
          speedProfile = 1;
          break;
        case 3:
          speedProfile = 0;
          break;
        default:
          break;
      }
      return;
    }

    if (frame.can_id != 1021) {
      return;
    }

    uint8_t index = readMuxID(frame);
    bool selected = isFSDSelectedInUI(frame);

    if (index == 0 && selected) {
      int rawOff = static_cast<uint8_t>((frame.data[3] >> 1) & 0x3F) - 30;
      speedOffset = constrain(rawOff * 5, 0, 100);

      switch (rawOff) {
        case 2:
          speedProfile = 2;
          break;
        case 1:
          speedProfile = 1;
          break;
        case 0:
          speedProfile = 0;
          break;
        default:
          break;
      }

      setBit(frame, 46, true);
      setSpeedProfileV12V13(frame, speedProfile);
      mcp.sendMessage(&frame);
    }

    if (index == 1) {
      setBit(frame, 19, false);
      mcp.sendMessage(&frame);
    }

    if (index == 2 && selected) {
      frame.data[0] &= ~(0b11000000);
      frame.data[1] &= ~(0b00111111);
      frame.data[0] |= static_cast<uint8_t>((speedOffset & 0x03) << 6);
      frame.data[1] |= static_cast<uint8_t>(speedOffset >> 2);
      mcp.sendMessage(&frame);
    }

    if (index == 0) {
      printHandlerStatus("HW3Handler", selected, speedProfile, speedOffset);
    }
  }
};

struct HW4Handler : public CarManagerBase {
  void handleMessage(can_frame &frame) override {
    if (frame.can_id == 1016) {
      uint8_t fd = static_cast<uint8_t>((frame.data[5] & 0b11100000) >> 5);
      switch (fd) {
        case 1:
          speedProfile = 3;
          break;
        case 2:
          speedProfile = 2;
          break;
        case 3:
          speedProfile = 1;
          break;
        case 4:
          speedProfile = 0;
          break;
        case 5:
          speedProfile = 4;
          break;
        default:
          break;
      }
    }

    if (frame.can_id != 1021) {
      return;
    }

    uint8_t index = readMuxID(frame);
    bool selected = isFSDSelectedInUI(frame);

    if (index == 0 && selected) {
      setBit(frame, 46, true);
      setBit(frame, 60, true);
      if (ENABLE_APPROACHING_EMERGENCY_VEHICLE_DETECTION) {
        setBit(frame, 59, true);
      }
      mcp.sendMessage(&frame);
    }

    if (index == 1) {
      setBit(frame, 19, false);
      setBit(frame, 47, true);
      mcp.sendMessage(&frame);
    }

    if (index == 2) {
      frame.data[7] &= ~(0x07 << 4);
      frame.data[7] |= static_cast<uint8_t>((speedProfile & 0x07) << 4);
      mcp.sendMessage(&frame);
    }

    if (index == 0) {
      printHandlerStatus("HW4Handler", selected, speedProfile, -1);
    }
  }
};

LegacyHandler legacyHandler;
HW3Handler hw3Handler;
HW4Handler hw4Handler;
CarManagerBase *handler = nullptr;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(CAN_INT_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  delay(1200);

#if HW == LEGACY
  handler = &legacyHandler;
#elif HW == HW3
  handler = &hw3Handler;
#elif HW == HW4
  handler = &hw4Handler;
#else
#error Invalid HW selection. Use LEGACY, HW3, or HW4.
#endif

  mcp.reset();
  MCP2515::ERROR e = mcp.setBitrate(CAN_500KBPS, MCP_16MHZ);
  if (e != MCP2515::ERROR_OK) {
    Serial.println("setBitrate failed");
  }

  mcp.setNormalMode();
  Serial.println("MCP2515 ready @ 500k");
}

void loop() {
  can_frame frame;
  int r = mcp.readMessage(&frame);

  if (r != MCP2515::ERROR_OK) {
    digitalWrite(LED_PIN, HIGH);
    return;
  }

  digitalWrite(LED_PIN, LOW);
  if (handler != nullptr) {
    handler->handleMessage(frame);
  }
}
