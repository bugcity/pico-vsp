# pico-hid

Raspberry Pi Pico を USB 複合デバイスとして動かし、以下の 3 つを PC に対して公開するリポジトリです。

- HID キーボード
- HID マウス
- CDC シリアルポート

起動後は USB デバイスとして待機し、シリアルポート経由でコマンドを受信するまではキーボード入力やマウス操作を行いません。

## できること

シリアルポートに改行区切りでコマンドを送ると、Pico が PC に対して HID 操作を行います。

- `MOVE <x> <y>`
  マウスカーソルを `x`, `y` の移動量ぶん相対移動します。実行時に LED が 2 回点滅します。
- `CLICK LEFT`
  左クリックします。実行時に LED が 2 回点滅します。
- `CLICK RIGHT`
  右クリックします。実行時に LED が 2 回点滅します。
- `TESTKEY`
  `@` を 1 文字送信します。実行時に LED が 4 回点滅します。

## Build

Docker コンテナ内でビルドします。

```shell
docker compose run --rm pico-dev bash
cmake -S . -B build
cmake --build build
```

ビルド後の成果物は `build/pico-hid.uf2` です。

## macOS

macOS では、接続時にキーボード設定アシスタントが表示されることがあります。
このリポジトリの用途では、表示された場合はそのまま終了して構いません。

ユースケースの例:

- ダミー HDMI を外部ディスプレイとして接続する
- Pico を USB キーボード/マウス兼シリアルデバイスとして接続する
- MacBook を給電したままフタを閉じる
- iPad などから Chrome Remote Desktop で接続する

この構成で、フタを閉じたまま Mac を継続利用できます。
