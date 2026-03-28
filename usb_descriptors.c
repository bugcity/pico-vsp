#include "tusb.h"
#include <string.h>

#define HID_COUNTRY_NOT_SUPPORTED 0
#define HID_COUNTRY_US 33

#define TUD_HID_DESCRIPTOR_COUNTRY(itfnum, stridx, protocol, country_code, report_len, epin, size, interval) \
  9, TUSB_DESC_INTERFACE, (itfnum), 0, 1, TUSB_CLASS_HID, HID_SUBCLASS_BOOT, (protocol), (stridx),           \
  9, HID_DESC_TYPE_HID, U16_TO_U8S_LE(0x0111), (country_code), 1, HID_DESC_TYPE_REPORT,                       \
  U16_TO_U8S_LE(report_len),                                                                                   \
  7, TUSB_DESC_ENDPOINT, (epin), TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(size), (interval)

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
  .idProduct          = 0x4001,
  .bcdDevice          = 0x0100,
  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,
  .bNumConfigurations = 1
};

uint8_t const* tud_descriptor_device_cb(void) {
  return (uint8_t const*)&desc_device;
}

// HIDレポート記述子（Report IDなし, HID×2）
uint8_t const desc_hid_report_kbd[] = {
  TUD_HID_REPORT_DESC_KEYBOARD()
};

uint8_t const desc_hid_report_mouse[] = {
  TUD_HID_REPORT_DESC_MOUSE()
};

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
  if (instance == 0) return desc_hid_report_kbd;
  if (instance == 1) return desc_hid_report_mouse;
  return NULL;
}

// インターフェース番号
enum {
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
  ITF_NUM_HID_KBD,
  ITF_NUM_HID_MOUSE,
  ITF_NUM_TOTAL
};

// エンドポイント定義
#define EPNUM_CDC_NOTIF   0x81
#define EPNUM_CDC_OUT     0x02
#define EPNUM_CDC_IN      0x82
#define EPNUM_HID_KBD     0x83
#define EPNUM_HID_MOUSE   0x84

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN * 2)

uint8_t const desc_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

  // CDC
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

  // HID Keyboard
  TUD_HID_DESCRIPTOR_COUNTRY(ITF_NUM_HID_KBD, 5, HID_ITF_PROTOCOL_KEYBOARD,
    HID_COUNTRY_US, sizeof(desc_hid_report_kbd), EPNUM_HID_KBD, 16, 1),

  // HID Mouse
  TUD_HID_DESCRIPTOR_COUNTRY(ITF_NUM_HID_MOUSE, 6, HID_ITF_PROTOCOL_MOUSE,
    HID_COUNTRY_NOT_SUPPORTED, sizeof(desc_hid_report_mouse), EPNUM_HID_MOUSE, 16, 1)
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return desc_configuration;
}

// 文字列記述子
char const* string_desc_arr[] = {
  (const char[]) { 0x09, 0x04 }, // LangID: English (0x0409)
  "Raspberry Pi Pico",           // Manufacturer
  "pico-hid",                    // Product
  "123456",                      // Serial Number
  "pico-hid CDC",                // CDC Interface
  "pico-hid Keyboard",           // Keyboard Interface
  "pico-hid Mouse"               // Mouse Interface
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
