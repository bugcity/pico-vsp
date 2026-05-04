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

| ポート | 用途 |
|--------|------|
| CDC0 | 制御ポート（コマンド受付） |
| CDC1 | デバイスA接続 |
| CDC2 | デバイスB接続 |
| CDC3 | キャプチャログ出力 |

CDC1 ↔ CDC2 は常時透過中継されます。CDC3 には通信ログが出力されます。

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

## ホスト側ブリッジの使い方

CDC2 と実シリアルポートをブリッジしつつ、CDC3 のログをファイルに保存します。

```shell
cd host
uv run bridge.py <CDC2> <実シリアルポート> <CDC3> <ログファイル>
```

シリアルポートの通信設定はデフォルト 8N1（データ8ビット・パリティなし・ストップビット1）です。

| オプション | デフォルト | 指定可能な値 |
|------------|-----------|-------------|
| `--baudrate` | 115200 | 任意の整数 |
| `--bytesize` | 8 | 5 / 6 / 7 / 8 |
| `--parity` | N | N / E / O / M / S |
| `--stopbits` | 1 | 1 / 1.5 / 2 |

**Mac / Linux**

```shell
# CDC2（3番目のモデム）・CDC3（4番目のモデム）を指定する
uv run bridge.py /dev/tty.usbmodem005 /dev/tty.usbserial-XXXX /dev/tty.usbmodem007 capture.log

# 通信設定を変える場合
uv run bridge.py /dev/tty.usbmodem005 /dev/tty.usbserial-XXXX /dev/tty.usbmodem007 capture.log \
  --baudrate 9600 --bytesize 7 --parity E --stopbits 2
```

**Windows**

```shell
uv run bridge.py COM3 COM4 COM5 capture.log

# 通信設定を変える場合
uv run bridge.py COM3 COM4 COM5 capture.log --baudrate 9600 --bytesize 7 --parity E --stopbits 2
```

ポート名はデバイスマネージャーで確認してください。

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
