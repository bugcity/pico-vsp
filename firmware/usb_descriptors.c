#include "tusb.h"
#include <string.h>

// デバイス記述子
tusb_desc_device_t const desc_device = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0200,
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

// 文字列記述子
char const* string_desc_arr[] = {
  (const char[]) { 0x09, 0x04 }, // LangID: English (0x0409)
  "Raspberry Pi Pico",           // Manufacturer
  "pico-vsp",                    // Product
  "123456",                      // Serial Number
  "pico-vsp Control",            // CDC0: 制御ポート
  "pico-vsp Data1",              // CDC1: データポート1
  "pico-vsp Data2",              // CDC2: データポート2
  "pico-vsp Data3",              // CDC3: データポート3
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  uint8_t chr_count;

  if (index == 0) {
    _desc_str[1] = 0x0409;
    chr_count = 1;
  } else {
    const char* str = string_desc_arr[index];
    chr_count = strlen(str);
    if (chr_count > 31) chr_count = 31;
    for (uint8_t i = 0; i < chr_count; i++) {
      _desc_str[1 + i] = str[i];
    }
  }

  _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
  return _desc_str;
}
