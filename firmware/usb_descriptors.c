#include "tusb.h"
#include "pico/unique_id.h"
#include <string.h>
#include <stdio.h>

// デバイス記述子
tusb_desc_device_t const desc_device = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0210,  // USB 2.1: BOS デスクリプタをサポートすることを宣言
  .bDeviceClass       = TUSB_CLASS_MISC,
  .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol    = MISC_PROTOCOL_IAD,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = 0xCafe,
  .idProduct          = 0x4002,
  .bcdDevice          = 0x0100,
  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,
  .bNumConfigurations = 1
};

uint8_t const* tud_descriptor_device_cb(void) {
  return (uint8_t const*)&desc_device;
}

// インターフェース番号
enum {
  ITF_NUM_CDC0 = 0,
  ITF_NUM_CDC0_DATA,
  ITF_NUM_CDC1,
  ITF_NUM_CDC1_DATA,
  ITF_NUM_CDC2,
  ITF_NUM_CDC2_DATA,
  ITF_NUM_CDC3,
  ITF_NUM_CDC3_DATA,
  ITF_NUM_TOTAL
};

// エンドポイント定義
#define EPNUM_CDC0_NOTIF  0x81
#define EPNUM_CDC0_OUT    0x02
#define EPNUM_CDC0_IN     0x82

#define EPNUM_CDC1_NOTIF  0x83
#define EPNUM_CDC1_OUT    0x04
#define EPNUM_CDC1_IN     0x84

#define EPNUM_CDC2_NOTIF  0x85
#define EPNUM_CDC2_OUT    0x06
#define EPNUM_CDC2_IN     0x86

#define EPNUM_CDC3_NOTIF  0x87
#define EPNUM_CDC3_OUT    0x08
#define EPNUM_CDC3_IN     0x88

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN * 4)

uint8_t const desc_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

  // CDC0: 制御ポート
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC0, 4, EPNUM_CDC0_NOTIF, 8, EPNUM_CDC0_OUT, EPNUM_CDC0_IN, 64),

  // CDC1: データポート1
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC1, 5, EPNUM_CDC1_NOTIF, 8, EPNUM_CDC1_OUT, EPNUM_CDC1_IN, 64),

  // CDC2: データポート2
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC2, 6, EPNUM_CDC2_NOTIF, 8, EPNUM_CDC2_OUT, EPNUM_CDC2_IN, 64),

  // CDC3: データポート3
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC3, 7, EPNUM_CDC3_NOTIF, 8, EPNUM_CDC3_OUT, EPNUM_CDC3_IN, 64),
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return desc_configuration;
}

// -------------------------------------------------------------------
// BOS デスクリプタ (Binary Object Store) + Microsoft OS 2.0
// -------------------------------------------------------------------
#define MS_OS_20_DESC_LEN  10   // Set Header のみ
#define BOS_TOTAL_LEN      (5 + 28)

// MS_OS_20 デスクリプタセット (Windows に渡す本体)
static const uint8_t desc_ms_os_20[] = {
  // Set Header Descriptor: 10 bytes
  0x0A, 0x00,              // wLength
  0x00, 0x00,              // wDescriptorType: MSOS20_SET_HEADER_DESCRIPTOR
  0x00, 0x00, 0x03, 0x06,  // dwWindowsVersion: Windows 8.1 (0x06030000)
  MS_OS_20_DESC_LEN, 0x00, // wTotalLength
};

// BOS デスクリプタ本体
static const uint8_t desc_bos[] = {
  // BOS Descriptor header: 5 bytes
  5,                       // bLength
  0x0F,                    // bDescriptorType: BOS
  BOS_TOTAL_LEN, 0x00,     // wTotalLength (little-endian)
  1,                       // bNumDeviceCaps

  // Microsoft OS 2.0 Platform Capability Descriptor: 28 bytes
  28,                      // bLength
  0x10,                    // bDescriptorType: Device Capability
  0x05,                    // bDevCapabilityType: Platform
  0x00,                    // bReserved
  // PlatformCapabilityUUID: {D8DD60DF-4589-4CC7-9CD2-659D9E648A9F} (little-endian)
  0xDF, 0x60, 0xDD, 0xD8,
  0x89, 0x45,
  0xC7, 0x4C,
  0x9C, 0xD2,
  0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
  // dwWindowsVersion: Windows 8.1 (0x06030000, little-endian)
  0x00, 0x00, 0x03, 0x06,
  // wMSOSDescriptorSetTotalLength
  MS_OS_20_DESC_LEN, 0x00,
  // bMS_VendorCode: Windows はこの値で Control Request を送ってくる
  0x01,
  // bAltEnumCode
  0x00,
};

uint8_t const* tud_descriptor_bos_cb(void) {
  return desc_bos;
}

// Windows から MS OS 2.0 デスクリプタセットを要求する Vendor Control Request を処理
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                 tusb_control_request_t const *request) {
  if (stage != CONTROL_STAGE_SETUP) return true;
  if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
      request->bRequest == 0x01 &&
      request->wIndex == 7) {
    return tud_control_xfer(rhport, request,
                            (void*)(uintptr_t)desc_ms_os_20,
                            sizeof(desc_ms_os_20));
  }
  return false;
}

// -------------------------------------------------------------------
// 文字列記述子
// -------------------------------------------------------------------
static const char* string_desc_arr[] = {
  (const char[]) { 0x09, 0x04 }, // LangID: English (0x0409)
  "Raspberry Pi Pico",           // Manufacturer
  "pico-vsp",                    // Product
  NULL,                          // Serial Number: 動的生成 (RP2040 unique ID)
  "pico-vsp Control",            // CDC0: 制御ポート
  "pico-vsp Data1",              // CDC1: データポート1
  "pico-vsp Data2",              // CDC2: データポート2
  "pico-vsp Data3",              // CDC3: データポート3
};

static uint16_t _desc_str[32];
static char _serial_str[17];  // 8 bytes * 2 hex chars + NUL

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  uint8_t chr_count;

  if (index == 0) {
    _desc_str[1] = 0x0409;
    chr_count = 1;
  } else if (index == 3) {
    // シリアル番号: RP2040 固有のボードID を 16 進文字列に変換
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    static const char hex[] = "0123456789ABCDEF";
    for (uint8_t i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
      _serial_str[i * 2]     = hex[(board_id.id[i] >> 4) & 0x0F];
      _serial_str[i * 2 + 1] = hex[board_id.id[i] & 0x0F];
    }
    _serial_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2] = '\0';
    chr_count = PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2;
    for (uint8_t i = 0; i < chr_count; i++) {
      _desc_str[1 + i] = (uint16_t)_serial_str[i];
    }
  } else {
    uint8_t num_arr = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]);
    if (index >= num_arr || string_desc_arr[index] == NULL) return NULL;
    const char* str = string_desc_arr[index];
    chr_count = (uint8_t)strlen(str);
    if (chr_count > 31) chr_count = 31;
    for (uint8_t i = 0; i < chr_count; i++) {
      _desc_str[1 + i] = str[i];
    }
  }

  _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
  return _desc_str;
}
