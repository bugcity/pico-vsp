#pragma once

// RP2040 MCU
#define CFG_TUSB_MCU             OPT_MCU_RP2040
#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS              OPT_OS_NONE
#endif
#define CFG_TUSB_RHPORT0_MODE    OPT_MODE_DEVICE
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN       __attribute__((aligned(4)))

#define CFG_TUD_VBUS_MONITORING 0

// EP0サイズ
#define CFG_TUD_ENDPOINT0_SIZE   64

// CDC x4: CDC0=制御ポート, CDC1〜3=データポート
#define CFG_TUD_CDC              4
#define CFG_TUD_CDC_RX_BUFSIZE   256
#define CFG_TUD_CDC_TX_BUFSIZE   256
