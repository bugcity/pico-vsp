#!/usr/bin/env python3
"""
pico-vsp bridge

CDC2 <-> 実シリアルポート の双方向ブリッジ
CDC3 -> ログファイル への書き出し
"""

import threading
import signal
from datetime import datetime
from pathlib import Path

import serial
import typer

app = typer.Typer()

stop_event = threading.Event()


def relay(src: serial.Serial, dst: serial.Serial, label: str) -> None:
    """src から読んで dst に書き続けるスレッド"""
    while not stop_event.is_set():
        try:
            data = src.read(src.in_waiting or 1)
            if data:
                dst.write(data)
        except serial.SerialException as e:
            typer.echo(f"[{label}] serial error: {e}", err=True)
            stop_event.set()
            break


def capture_log(cdc3: serial.Serial, log_path: Path) -> None:
    """CDC3 から受信したデータをログファイルに書き出すスレッド"""
    with log_path.open("a", encoding="utf-8") as f:
        f.write(f"# log opened at {datetime.now().isoformat()}\n")
        f.flush()
        while not stop_event.is_set():
            try:
                data = cdc3.read(cdc3.in_waiting or 1)
                if data:
                    text = data.decode("utf-8", errors="replace")
                    f.write(text)
                    f.flush()
            except serial.SerialException as e:
                typer.echo(f"[CDC3] serial error: {e}", err=True)
                stop_event.set()
                break


@app.command()
def main(
    cdc2: str = typer.Argument(help="CDC2 ポートのパス (例: /dev/tty.usbmodem****)"),
    serial_port: str = typer.Argument(help="実シリアルポートのパス (例: /dev/tty.usbserial-****)"),
    cdc3: str = typer.Argument(help="CDC3 ポートのパス (例: /dev/tty.usbmodem****)"),
    log_file: Path = typer.Argument(help="ログ出力先ファイルパス"),
    baudrate: int = typer.Option(115200, "--baudrate", "-b", help="実シリアルポートのボーレート"),
    bytesize: int = typer.Option(8, "--bytesize", help="データビット数 (5/6/7/8)"),
    parity: str = typer.Option("N", "--parity", help="パリティ (N/E/O/M/S)"),
    stopbits: float = typer.Option(1.0, "--stopbits", help="ストップビット (1/1.5/2)"),
) -> None:
    """CDC2 <-> 実シリアルポート のブリッジ + CDC3 ログキャプチャ"""

    bytesize_map = {5: serial.FIVEBITS, 6: serial.SIXBITS, 7: serial.SEVENBITS, 8: serial.EIGHTBITS}
    parity_map = {"N": serial.PARITY_NONE, "E": serial.PARITY_EVEN, "O": serial.PARITY_ODD,
                  "M": serial.PARITY_MARK, "S": serial.PARITY_SPACE}
    stopbits_map = {1.0: serial.STOPBITS_ONE, 1.5: serial.STOPBITS_ONE_POINT_FIVE, 2.0: serial.STOPBITS_TWO}

    if bytesize not in bytesize_map:
        typer.echo(f"ERROR: invalid bytesize: {bytesize}", err=True)
        raise typer.Exit(1)
    if parity.upper() not in parity_map:
        typer.echo(f"ERROR: invalid parity: {parity}", err=True)
        raise typer.Exit(1)
    if stopbits not in stopbits_map:
        typer.echo(f"ERROR: invalid stopbits: {stopbits}", err=True)
        raise typer.Exit(1)

    def handle_signal(sig, frame):
        typer.echo("\n[bridge] stopping...")
        stop_event.set()

    signal.signal(signal.SIGINT, handle_signal)
    if hasattr(signal, "SIGTERM"):
        signal.signal(signal.SIGTERM, handle_signal)

    typer.echo(f"[bridge] opening CDC2:      {cdc2}")
    typer.echo(f"[bridge] opening serial:    {serial_port} @ {baudrate} {bytesize}{parity.upper()}{stopbits:g}")
    typer.echo(f"[bridge] opening CDC3:      {cdc3}")
    typer.echo(f"[bridge] log file:          {log_file}")

    try:
        cdc2_ser = serial.Serial(cdc2, timeout=0.1)
        real_ser = serial.Serial(
            serial_port,
            baudrate=baudrate,
            bytesize=bytesize_map[bytesize],
            parity=parity_map[parity.upper()],
            stopbits=stopbits_map[stopbits],
            timeout=0.1,
        )
        cdc3_ser = serial.Serial(cdc3, timeout=0.1)
    except serial.SerialException as e:
        typer.echo(f"[bridge] failed to open port: {e}", err=True)
        raise typer.Exit(1)

    threads = [
        threading.Thread(target=relay, args=(cdc2_ser, real_ser, "CDC2->serial"), daemon=True),
        threading.Thread(target=relay, args=(real_ser, cdc2_ser, "serial->CDC2"), daemon=True),
        threading.Thread(target=capture_log, args=(cdc3_ser, log_file), daemon=True),
    ]

    for t in threads:
        t.start()

    typer.echo("[bridge] running. press Ctrl+C to stop.")
    stop_event.wait()

    cdc2_ser.close()
    real_ser.close()
    cdc3_ser.close()
    typer.echo("[bridge] stopped.")


if __name__ == "__main__":
    app()
