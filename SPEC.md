# pico-vsp 仕様書 v1.0

## 概要

Raspberry Pi Pico を USB 複合 CDC デバイスとして動作させ、2つのシリアルポート間の透過中継とトラフィックキャプチャを実現する。

## USBデバイス構成

| ポート | 用途 |
|--------|------|
| CDC0 | 制御ポート（コマンド受付・状態通知） |
| CDC1 | デバイスA接続 |
| CDC2 | デバイスB接続 |
| CDC3 | キャプチャログ出力 |

## データフロー

- CDC1 ↔ CDC2 を常時透過中継（双方向）
- CDC1/CDC2 の全通信を CDC3 にログ出力（LOG START/STOP で制御）

## ログフォーマット

```
[    0.000][1→2] 48 65 6C 6C 6F  Hello
[    0.123][2→1] 4F 4B            OK
[    0.125][1 DTR=1 RTS=0]
[    0.200][2 DTR=0 RTS=1]
```

- タイムスタンプ：`LOG START` コマンド受信時点からの経過時間（秒.ミリ秒）
- 実装：`time_us_64()` を使用

## ログ出力モード

| モード | 内容 |
|--------|------|
| HEX | HEX形式のみ（デフォルト） |
| ASCII | ASCII形式のみ |
| BOTH | HEX+ASCII併記 |

## 制御コマンド（CDC0）

| コマンド | 動作 |
|----------|------|
| `STATUS` | 現在のログ状態・信号線状態を返す |
| `LOG HEX` | ログをHEX形式で出力 |
| `LOG ASCII` | ログをASCII形式で出力 |
| `LOG BOTH` | HEX+ASCII併記で出力 |
| `LOG START` | タイムスタンプリセット＆ログ開始 |
| `LOG STOP` | ログ出力を停止 |

## 信号線キャプチャ

- DTR・RTS の変化を検知し CDC3 にログ出力
- TinyUSB コールバック：`tud_cdc_line_state_cb(itf, dtr, rts)`
- ボーレート変化も記録：`tud_cdc_line_coding_cb(itf, coding)`

## リポジトリ構成

```
pico-vsp/
├── firmware/               # Pico ファームウェア（C）
│   ├── CMakeLists.txt
│   ├── Dockerfile
│   ├── docker-compose.yml
│   ├── main.c
│   ├── pico_sdk_import.cmake
│   ├── tusb_config.h
│   └── usb_descriptors.c
├── host/                   # ホスト側（Python）
│   ├── bridge.py
│   └── pyproject.toml
├── README.md
└── SPEC.md
```

## 実装ファイル

### firmware/

| ファイル | 役割 |
|----------|------|
| `tusb_config.h` | CDC×4、HID×0 |
| `usb_descriptors.c` | CDC4本分のディスクリプタ |
| `main.c` | 中継・ログ・コマンド処理 |

### host/

| ファイル | 役割 |
|----------|------|
| `vsp.py` | CDC2↔実シリアルポート ブリッジ、CDC3ログ保存（独立・同時使用可） |
| `pyproject.toml` | uv 依存関係定義 |

### vsp.py オプション

CDC2ブリッジと CDC3キャプチャは独立して使用可能。実シリアルポートの通信設定はすべて指定可能（デフォルト: 8N1）。

| オプション | デフォルト | 指定可能な値 | 対象 |
|------------|-----------|-------------|------|
| `--cdc2` | — | ポートパス | CDC2ポート（ブリッジ用） |
| `--serial` | — | ポートパス | 実シリアルポート（ブリッジ用） |
| `--cdc3` | — | ポートパス | CDC3ポート（キャプチャ用） |
| `--log` | — | ファイルパス | ログ出力先（キャプチャ用） |
| `--baudrate` | 115200 | 任意の整数 | 実シリアルポート |
| `--bytesize` | 8 | 5 / 6 / 7 / 8 | 実シリアルポート |
| `--parity` | N | N / E / O / M / S | 実シリアルポート |
| `--stopbits` | 1 | 1 / 1.5 / 2 | 実シリアルポート |

## ビルド環境

- Pico C SDK
- Docker（`firmware/docker-compose.yml`）

## ホスト実行環境

- Python 3.11+
- uv
