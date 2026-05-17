# pico-vsp

Raspberry Pi Pico を USB 複合 CDC デバイスとして動作させ、2つのシリアルポート間の透過中継とトラフィックキャプチャを実現するツールです。

## 構成

```
pico-vsp/
├── firmware/   # Pico ファームウェア（C）
├── host/       # ホスト側ブリッジスクリプト（Python）
├── README.md
└── SPEC.md
```

## USBデバイス構成

| ポート | 用途 | Mac ポート名 | Windows ポート名 |
|--------|------|-------------|-----------------|
| CDC0 | 制御ポート（コマンド受付） | `/dev/cu.usbmodemXXXX` | `COMx` |
| CDC1 | デバイスA接続 | `/dev/cu.usbmodemXXXX` | `COMx` |
| CDC2 | デバイスB接続 | `/dev/cu.usbmodemXXXX` | `COMx` |
| CDC3 | キャプチャログ出力 | `/dev/cu.usbmodemXXXX` | `COMx` |

CDC1 ↔ CDC2 は常時透過中継されます。CDC3 には通信ログが出力されます。

> **Note:** ポート名は Pico 個体ごとに異なります。Mac では `ls /dev/cu.usbmodem*`、Windows ではデバイスマネージャーの「ポート（COM と LPT）」で確認してください。4 つのポートが pico-vsp Control / Data1 / Data2 / Data3 として表示されます。

## 制御コマンド（CDC0）

| コマンド | 動作 |
|----------|------|
| `STATUS` | 現在のログ状態・信号線状態を返す |
| `LOG HEX` | ログをHEX形式で出力（デフォルト） |
| `LOG ASCII` | ログをASCII形式で出力 |
| `LOG BOTH` | HEX+ASCII併記で出力 |
| `LOG START` | タイムスタンプリセット＆ログ開始 |
| `LOG STOP` | ログ出力を停止 |

## ファームウェアのビルド

```shell
cd firmware
docker compose run --rm pico-dev bash
cmake -S . -B build
cmake --build build
```

成果物: `firmware/build/pico-vsp.uf2`

## ホスト側ツールの使い方

CDC2ブリッジと CDC3キャプチャを独立して、または同時に使用できます。

| オプション | 説明 |
|------------|------|
| `--cdc2 PORT` | CDC2ポート（ブリッジ用） |
| `--serial PORT` | 実シリアルポート（ブリッジ用） |
| `--cdc3 PORT` | CDC3ポート（キャプチャ用） |
| `--log FILE` | ログファイル（キャプチャ用） |

シリアルポートの通信設定はデフォルト 8N1（データ8ビット・パリティなし・ストップビット1）です。

| オプション | デフォルト | 指定可能な値 |
|------------|-----------|-------------|
| `--baudrate` | 115200 | 任意の整数 |
| `--bytesize` | 8 | 5 / 6 / 7 / 8 |
| `--parity` | N | N / E / O / M / S |
| `--stopbits` | 1 | 1 / 1.5 / 2 |

**Mac**

```shell
# ポート名を確認
ls /dev/cu.usbmodem*

# CDC2ブリッジのみ
uv run vsp.py --cdc2 /dev/cu.usbmodemXXXX --serial /dev/cu.usbserial-XXXX

# CDC3キャプチャのみ
uv run vsp.py --cdc3 /dev/cu.usbmodemXXXX --log capture.log

# 両方同時
uv run vsp.py --cdc2 /dev/cu.usbmodemXXXX --serial /dev/cu.usbserial-XXXX \
              --cdc3 /dev/cu.usbmodemXXXX --log capture.log

# 通信設定を変える場合
uv run vsp.py --cdc2 /dev/cu.usbmodemXXXX --serial /dev/cu.usbserial-XXXX \
              --baudrate 9600 --bytesize 7 --parity E --stopbits 2
```

**Windows**

ドライバのインストールは不要です。接続すると自動認識されます（Windows 11 確認済み）。
デバイスマネージャーの「ポート（COM と LPT）」に pico-vsp の 4 ポートが表示されます。

```shell
# ポート番号はデバイスマネージャーで確認（例: CDC2=COM6, CDC3=COM8）

# CDC2ブリッジのみ
uv run vsp.py --cdc2 COM6 --serial COM4

# CDC3キャプチャのみ
uv run vsp.py --cdc3 COM8 --log capture.log

# 両方同時
uv run vsp.py --cdc2 COM6 --serial COM4 --cdc3 COM8 --log capture.log
```

詳細は `SPEC.md` を参照してください。

## ライセンス

MIT License - 詳細は [LICENSE](LICENSE) を参照してください。

本プロジェクトは以下のコンポーネントを使用しています。

| コンポーネント | ライセンス |
|---------------|-----------|
| [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) | BSD 3-Clause |
| [TinyUSB](https://github.com/hathach/tinyusb) | MIT |
| [pyserial](https://github.com/pyserial/pyserial) | BSD 3-Clause |
| [Typer](https://github.com/fastapi/typer) | MIT |
