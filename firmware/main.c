#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"
#include "hardware/gpio.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "hardware/regs/usb.h"
#include "hardware/structs/usb.h"

#define MAX_CMD_LEN  64
#define LED_PIN      PICO_DEFAULT_LED_PIN
#define LOG_CHUNK    16   // 1ログ行あたりの最大バイト数

typedef enum { LOG_HEX, LOG_ASCII, LOG_BOTH } LogMode;

static bool     log_active   = false;
static uint64_t log_start_us = 0;
static LogMode  log_mode     = LOG_HEX;

static bool cdc_dtr[4] = {false, false, false, false};
static bool cdc_rts[4] = {false, false, false, false};
static bool flow_ctrl   = false;

static char cmd_buffer[MAX_CMD_LEN];
static int  cmd_index = 0;

// CDC0にメッセージ送信
static void ctrl_write(const char *msg) {
    tud_cdc_n_write_str(0, msg);
    tud_cdc_n_write_flush(0);
}

// CDC3にログ送信
static void log_write(const char *msg) {
    tud_cdc_n_write_str(3, msg);
    tud_cdc_n_write_flush(3);
}

// タイムスタンプ文字列を生成 "[    0.000]"
static void format_timestamp(char *buf, size_t size) {
    uint64_t elapsed_us = time_us_64() - log_start_us;
    uint32_t secs = (uint32_t)(elapsed_us / 1000000);
    uint32_t ms   = (uint32_t)((elapsed_us % 1000000) / 1000);
    snprintf(buf, size, "[%5u.%03u]", secs, ms);
}

// データをCDC3にログ出力
static void log_data(uint8_t src, uint8_t dst, const uint8_t *data, uint32_t len) {
    if (!log_active) return;

    char ts[16];
    format_timestamp(ts, sizeof(ts));

    for (uint32_t offset = 0; offset < len; offset += LOG_CHUNK) {
        uint32_t chunk = len - offset;
        if (chunk > LOG_CHUNK) chunk = LOG_CHUNK;
        const uint8_t *p = data + offset;

        char line[128];
        int pos = 0;

        // "[    0.000][1→2] "  (→ = UTF-8: \xe2\x86\x92)
        pos += snprintf(line + pos, sizeof(line) - pos,
                        "%s[%u\xe2\x86\x92%u] ", ts, src, dst);

        if (log_mode == LOG_HEX || log_mode == LOG_BOTH) {
            for (uint32_t i = 0; i < chunk && pos < (int)sizeof(line) - 4; i++) {
                pos += snprintf(line + pos, sizeof(line) - pos, "%02X ", p[i]);
            }
        }

        if (log_mode == LOG_BOTH) {
            // 列を揃えるためHEX部分をLOG_CHUNK幅に揃える
            int written_hex = (int)(chunk * 3);
            int max_hex     = LOG_CHUNK * 3;
            for (int i = written_hex; i < max_hex && pos < (int)sizeof(line) - 1; i++) {
                line[pos++] = ' ';
            }
            if (pos < (int)sizeof(line) - 1) line[pos++] = ' ';
        }

        if (log_mode == LOG_ASCII || log_mode == LOG_BOTH) {
            for (uint32_t i = 0; i < chunk && pos < (int)sizeof(line) - 3; i++) {
                char c = (char)p[i];
                line[pos++] = (c >= 0x20 && c <= 0x7e) ? c : '.';
            }
        }

        if (pos < (int)sizeof(line) - 2) {
            line[pos++] = '\r';
            line[pos++] = '\n';
            line[pos]   = '\0';
        }
        log_write(line);
    }
}

// 信号線状態変化をCDC3にログ出力
static void log_signal(uint8_t port, bool dtr, bool rts) {
    if (!log_active) return;
    char ts[16];
    format_timestamp(ts, sizeof(ts));
    char line[64];
    snprintf(line, sizeof(line), "%s[%u DTR=%d RTS=%d]\r\n",
             ts, port, dtr ? 1 : 0, rts ? 1 : 0);
    log_write(line);
}

void exec_command(const char *cmd) {
    if (strcmp(cmd, "STATUS") == 0) {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "Log: %s (%s)\r\n"
                 "Flow: %s\r\n"
                 "CDC1 DTR=%d RTS=%d\r\n"
                 "CDC2 DTR=%d RTS=%d\r\n",
                 log_active   ? "active" : "stopped",
                 log_mode == LOG_HEX ? "HEX" : log_mode == LOG_ASCII ? "ASCII" : "BOTH",
                 flow_ctrl ? "ON" : "OFF",
                 cdc_dtr[1] ? 1 : 0, cdc_rts[1] ? 1 : 0,
                 cdc_dtr[2] ? 1 : 0, cdc_rts[2] ? 1 : 0);
        ctrl_write(buf);
    } else if (strcmp(cmd, "LOG HEX") == 0) {
        log_mode = LOG_HEX;
        ctrl_write("OK\r\n");
    } else if (strcmp(cmd, "LOG ASCII") == 0) {
        log_mode = LOG_ASCII;
        ctrl_write("OK\r\n");
    } else if (strcmp(cmd, "LOG BOTH") == 0) {
        log_mode = LOG_BOTH;
        ctrl_write("OK\r\n");
    } else if (strcmp(cmd, "LOG START") == 0) {
        log_start_us = time_us_64();
        log_active   = true;
        ctrl_write("OK\r\n");
    } else if (strcmp(cmd, "LOG STOP") == 0) {
        log_active = false;
        ctrl_write("OK\r\n");
    } else if (strcmp(cmd, "FLOW ON") == 0) {
        flow_ctrl = true;
        ctrl_write("OK\r\n");
    } else if (strcmp(cmd, "FLOW OFF") == 0) {
        flow_ctrl = false;
        ctrl_write("OK\r\n");
    } else if (cmd[0] != '\0') {
        ctrl_write("ERROR: unknown command\r\n");
    }
}

// CDC0: 制御コマンドを読み取る
static void process_control(void) {
    while (tud_cdc_n_available(0)) {
        char c = (char)tud_cdc_n_read_char(0);
        if (c == '\r' || c == '\n') {
            if (cmd_index > 0) {
                cmd_buffer[cmd_index] = '\0';
                exec_command(cmd_buffer);
                cmd_index = 0;
            }
        } else if (cmd_index < MAX_CMD_LEN - 1) {
            cmd_buffer[cmd_index++] = c;
        }
    }
}

// CDC1↔CDC2 透過中継 + CDC3ログ出力
static void relay_data(void) {
    uint8_t buf[64];

    // CDC1 → CDC2
    if (!flow_ctrl || cdc_rts[1]) {
        uint32_t avail = tud_cdc_n_available(1);
        uint32_t space = tud_cdc_n_write_available(2);
        if (avail > 0 && space > 0) {
            uint32_t to_read = avail < sizeof(buf) ? avail : sizeof(buf);
            if (to_read > space) to_read = space;
            uint32_t count = tud_cdc_n_read(1, buf, to_read);
            if (count > 0) {
                tud_cdc_n_write(2, buf, count);
                tud_cdc_n_write_flush(2);
                log_data(1, 2, buf, count);
            }
        }
    }

    // CDC2 → CDC1
    if (!flow_ctrl || cdc_rts[2]) {
        uint32_t avail = tud_cdc_n_available(2);
        uint32_t space = tud_cdc_n_write_available(1);
        if (avail > 0 && space > 0) {
            uint32_t to_read = avail < sizeof(buf) ? avail : sizeof(buf);
            if (to_read > space) to_read = space;
            uint32_t count = tud_cdc_n_read(2, buf, to_read);
            if (count > 0) {
                tud_cdc_n_write(1, buf, count);
                tud_cdc_n_write_flush(1);
                log_data(2, 1, buf, count);
            }
        }
    }
}

// TinyUSB: 信号線状態変化コールバック (DTR/RTS)
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    if (itf == 1 || itf == 2) {
        cdc_dtr[itf] = dtr;
        cdc_rts[itf] = rts;
        log_signal(itf, dtr, rts);
    }
}

// TinyUSB: ボーレート変化コールバック
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *p_coding) {
    if (!log_active) return;
    if (itf != 1 && itf != 2) return;
    char ts[16];
    format_timestamp(ts, sizeof(ts));
    char line[64];
    snprintf(line, sizeof(line), "%s[%u baud=%u]\r\n",
             ts, itf, (unsigned int)p_coding->bit_rate);
    log_write(line);
}

void reset_usb_phy(void) {
    usb_hw->main_ctrl = 0;
    usb_hw->sie_ctrl  = 0;
    usb_hw->inte      = 0;
    sleep_ms(10);
}

int main(void) {
    board_init();
    sleep_ms(500);
    reset_usb_phy();
    tusb_init();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        tud_task();
        process_control();
        relay_data();
    }
}
