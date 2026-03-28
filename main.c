#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"
#include "hardware/gpio.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "hardware/regs/usb.h"
#include "hardware/structs/usb.h"

#define MAX_CMD_LEN 64
#define LED_PIN PICO_DEFAULT_LED_PIN

#define MOD_SHIFT KEYBOARD_MODIFIER_LEFTSHIFT
#define MOD_NONE 0

#define HID_KEY_AT_JIS HID_KEY_BRACKET_LEFT  // 「@」 JISでは「[」位置
#define HID_KEY_COLON_JIS HID_KEY_SEMICOLON  // 「:」 JISでは「;」+Shift
#define HID_KEY_CARET_JIS HID_KEY_GRAVE      // 「^」 JISでは「`」位置
#define HID_KEY_YEN_JIS 0x89                 // 「¥」 JIS独自キーコード
#define HID_KEY_RO_JIS 0x87                  // 「ろ」キー
#define HID_KEY_BACKSLASH_JIS 0x87           // 「\」 JISでは「ろ」の位置
#define HID_KEY_UNDERSCORE_JIS HID_KEY_MINUS // 「_」 JISでは「-」+Shift
#define HID_KEY_QUOTE_JIS 0x2B

static char cmd_buffer[MAX_CMD_LEN];
static int cmd_index = 0;

typedef enum
{
  MAP_US,
  MAP_JIS
} KeyboardLayout;

static KeyboardLayout current_layout = MAP_US;

typedef struct
{
  uint8_t buttons;
  int8_t x;
  int8_t y;
  int8_t wheel;
  int8_t pan;
} __attribute__((packed)) my_mouse_report_t;

void blink_led(int times)
{
  for (int i = 0; i < times; i++)
  {
    gpio_put(LED_PIN, 1);
    sleep_ms(100);
    gpio_put(LED_PIN, 0);
    sleep_ms(100);
  }
}

void set_layout(const char *map)
{
  if (strcasecmp(map, "US") == 0)
    current_layout = MAP_US;
  else if (strcasecmp(map, "JIS") == 0)
    current_layout = MAP_JIS;
}

void send_key(uint8_t modifier, uint8_t keycode)
{
  uint8_t report[8] = {modifier, 0, keycode, 0, 0, 0, 0, 0};
  while (!tud_hid_n_ready(0))
    tud_task();
  tud_hid_n_report(0, 0, report, sizeof(report));

  sleep_ms(30);

  uint8_t release[8] = {0};
  while (!tud_hid_n_ready(0))
    tud_task();
  tud_hid_n_report(0, 0, release, sizeof(release));

  sleep_ms(30);
}

void send_char(char c)
{
  uint8_t keycode = 0;
  uint8_t modifier = 0;

  if (current_layout == MAP_US)
  {
    // US配列用
    if (c >= 'a' && c <= 'z')
    {
      keycode = HID_KEY_A + (c - 'a');
    }
    else if (c >= 'A' && c <= 'Z')
    {
      keycode = HID_KEY_A + (c - 'A');
      modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    }
    else if (c >= '0' && c <= '9')
    {
      // 数字キーは個別処理
      switch (c)
      {
      case '0':
        keycode = HID_KEY_0;
        break;
      case '1':
        keycode = HID_KEY_1;
        break;
      case '2':
        keycode = HID_KEY_2;
        break;
      case '3':
        keycode = HID_KEY_3;
        break;
      case '4':
        keycode = HID_KEY_4;
        break;
      case '5':
        keycode = HID_KEY_5;
        break;
      case '6':
        keycode = HID_KEY_6;
        break;
      case '7':
        keycode = HID_KEY_7;
        break;
      case '8':
        keycode = HID_KEY_8;
        break;
      case '9':
        keycode = HID_KEY_9;
        break;
      }
    }
    else
    {
      // 記号・特殊キー
      switch (c)
      {
      case ' ':
        keycode = HID_KEY_SPACE;
        break;
      case '\n':
        keycode = HID_KEY_ENTER;
        break;
      case '!':
        keycode = HID_KEY_1;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '"':
        keycode = HID_KEY_APOSTROPHE;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '#':
        keycode = HID_KEY_3;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '$':
        keycode = HID_KEY_4;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '%':
        keycode = HID_KEY_5;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '&':
        keycode = HID_KEY_7;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '*':
        keycode = HID_KEY_8;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '\'':
        keycode = HID_KEY_APOSTROPHE;
        break;
      case '(':
        keycode = HID_KEY_9;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case ')':
        keycode = HID_KEY_0;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '=':
        keycode = HID_KEY_EQUAL;
        break;
      case '+':
        keycode = HID_KEY_EQUAL;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '~':
        keycode = HID_KEY_GRAVE;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '^':
        keycode = HID_KEY_6;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '[':
        keycode = HID_KEY_BRACKET_LEFT;
        break;
      case ']':
        keycode = HID_KEY_BRACKET_RIGHT;
        break;
      case '{':
        keycode = HID_KEY_BRACKET_LEFT;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '}':
        keycode = HID_KEY_BRACKET_RIGHT;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '\\':
        keycode = HID_KEY_BACKSLASH;
        break;
      case '|':
        keycode = HID_KEY_BACKSLASH;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case ';':
        keycode = HID_KEY_SEMICOLON;
        break;
      case ':':
        keycode = HID_KEY_SEMICOLON;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '<':
        keycode = HID_KEY_COMMA;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '>':
        keycode = HID_KEY_PERIOD;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case ',':
        keycode = HID_KEY_COMMA;
        break;
      case '.':
        keycode = HID_KEY_PERIOD;
        break;
      case '?':
        keycode = HID_KEY_SLASH;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '/':
        keycode = HID_KEY_SLASH;
        break;
      case '@':
        keycode = HID_KEY_2;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      }
    }
  }
  else
  {
    // JIS配列用
    if (c >= 'a' && c <= 'z')
    {
      keycode = HID_KEY_A + (c - 'a');
    }
    else if (c >= 'A' && c <= 'Z')
    {
      keycode = HID_KEY_A + (c - 'A');
      modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    }
    else if (c >= '0' && c <= '9')
    {
      switch (c)
      {
      case '0':
        keycode = HID_KEY_0;
        break;
      case '1':
        keycode = HID_KEY_1;
        break;
      case '2':
        keycode = HID_KEY_2;
        break;
      case '3':
        keycode = HID_KEY_3;
        break;
      case '4':
        keycode = HID_KEY_4;
        break;
      case '5':
        keycode = HID_KEY_5;
        break;
      case '6':
        keycode = HID_KEY_6;
        break;
      case '7':
        keycode = HID_KEY_7;
        break;
      case '8':
        keycode = HID_KEY_8;
        break;
      case '9':
        keycode = HID_KEY_9;
        break;
      }
    }
    else
    {
      switch (c)
      {
      case ' ':
        keycode = HID_KEY_SPACE;
        break;
      case '\n':
        keycode = HID_KEY_ENTER;
        break;
      case '@':
        keycode = HID_KEY_BRACKET_LEFT;
        break; // JIS: @ on [ key
      case '`':
        keycode = HID_KEY_BRACKET_LEFT;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break; // JIS: @ on [ key
      case '[':
        keycode = HID_KEY_BRACKET_RIGHT;
        break; // JIS: [ on @ key
      case ']':
        keycode = HID_KEY_BACKSLASH;
        break; // JIS: ] on = key
      case '\\':
        keycode = HID_KEY_YEN_JIS;
        break; // yen key
      case '|':
        keycode = HID_KEY_YEN_JIS;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case ':':
        keycode = HID_KEY_APOSTROPHE;
        break;
      case '*':
        keycode = HID_KEY_APOSTROPHE;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case ';':
        keycode = HID_KEY_SEMICOLON;
        break;
      case '+':
        keycode = HID_KEY_SEMICOLON;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case ',':
        keycode = HID_KEY_COMMA;
        break;
      case '.':
        keycode = HID_KEY_PERIOD;
        break;
      case '<':
        keycode = HID_KEY_COMMA;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '>':
        keycode = HID_KEY_PERIOD;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '^':
        keycode = HID_KEY_EQUAL;
        break;
      case '/':
        keycode = HID_KEY_SLASH;
        break;
      case '?':
        keycode = HID_KEY_SLASH;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '!':
        keycode = HID_KEY_1;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '"':
        keycode = HID_KEY_2;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '#':
        keycode = HID_KEY_3;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '$':
        keycode = HID_KEY_4;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '%':
        keycode = HID_KEY_5;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '&':
        keycode = HID_KEY_6;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '\'':
        keycode = HID_KEY_7;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '(':
        keycode = HID_KEY_8;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case ')':
        keycode = HID_KEY_9;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '-':
        keycode = HID_KEY_MINUS;
        break;
      case '=':
        keycode = HID_KEY_MINUS;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case '~':
        keycode = HID_KEY_EQUAL;
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      }
    }
  }

  if (keycode != 0)
  {
    send_key(modifier, keycode);
  }
}

void send_mouse_report(uint8_t buttons, int8_t x, int8_t y)
{
  my_mouse_report_t report = {buttons, x, y, 0, 0};
  while (!tud_hid_n_ready(1))
    tight_loop_contents();
  tud_hid_n_report(1, 0, &report, sizeof(report)); // Report ID = 0 (省略)
}

void exec_command(const char *cmd)
{
  if (strncmp(cmd, "MOVE", 4) == 0)
  {
    int x = 0, y = 0;
    if (sscanf(cmd + 5, "%d %d", &x, &y) == 2)
    {
      blink_led(2);
      send_mouse_report(0, (int8_t)x, (int8_t)y);
    }
  }
  else if (strncmp(cmd, "CLICK LEFT", 10) == 0)
  {
    blink_led(2);
    send_mouse_report(1, 0, 0);
    sleep_ms(50);
    send_mouse_report(0, 0, 0);
  }
  else if (strncmp(cmd, "CLICK RIGHT", 11) == 0)
  {
    blink_led(2);
    send_mouse_report(2, 0, 0);
    sleep_ms(50);
    send_mouse_report(0, 0, 0);
  }
  else if (strncmp(cmd, "TESTKEY", 7) == 0)
  {
    blink_led(4);
    send_char('@');
  }
}

void tud_cdc_rx_cb(uint8_t itf)
{
  while (tud_cdc_available())
  {
    char c = tud_cdc_read_char();
    if (c == '\r')
      continue;
    if (c == '\n')
    {
      cmd_buffer[cmd_index] = '\0';
      exec_command(cmd_buffer);
      cmd_index = 0;
    }
    else if (cmd_index < MAX_CMD_LEN - 1)
    {
      cmd_buffer[cmd_index++] = c;
    }
  }
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen)
{
  return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize)
{
}

#include "hardware/regs/usb.h"
#include "hardware/structs/usb.h"

void reset_usb_phy(void)
{
  usb_hw->main_ctrl = 0;
  usb_hw->sie_ctrl = 0;
  usb_hw->inte = 0;
  sleep_ms(10);
}

int main(void)
{
  board_init();
  sleep_ms(500);
  reset_usb_phy();
  tusb_init();
  sleep_ms(200);
  tud_disconnect();
  sleep_ms(200);
  tud_connect();
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  while (true)
  {
    tud_task();
  }
}
