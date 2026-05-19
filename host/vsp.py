#!/usr/bin/env python3
"""pico-vsp host tool"""

import threading
import signal
from datetime import datetime
from pathlib import Path
from typing import Optional

import serial
import typer

app = typer.Typer(help="CDC2 <-> 実シリアルポート のブリッジ と CDC3 ログキャプチャ（両方同時使用可）")

stop_event = threading.Event()


def relay(src: serial.Serial, dst: serial.Serial, label: str) -> None:
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
    cdc2: Optional[str] = typer.Option(None, "--cdc2", metavar="PORT", help="CDC2ポート（ブリッジ用）"),
    serial_port: Optional[str] = typer.Option(None, "--serial", metavar="PORT", help="実シリアルポート（ブリッジ用）"),
    cdc3: Optional[str] = typer.Option(None, "--cdc3", metavar="PORT", help="CDC3ポート（キャプチャ用）"),
    log_file: Optional[Path] = typer.Option(None, "--log", metavar="FILE", help="ログファイル（キャプチャ用）"),
    baudrate: int = typer.Option(115200, "--baudrate", "-b", help="実シリアルポートのボーレート"),
    bytesize: int = typer.Option(8, "--bytesize", help="データビット数 (5/6/7/8)"),
    parity: str = typer.Option("N", "--parity", help="パリティ (N/E/O/M/S)"),
    stopbits: float = typer.Option(1.0, "--stopbits", help="ストップビット (1/1.5/2)"),
    rtscts: bool = typer.Option(False, "--rtscts", help="RTS/CTSハードウェアフロー制御を有効化"),
    xonxoff: bool = typer.Option(False, "--xonxoff", help="XON/XOFFソフトウェアフロー制御を有効化"),
) -> None:
    bridge_mode = cdc2 is not None or serial_port is not None
    capture_mode = cdc3 is not None or log_file is not None

    if not bridge_mode and not capture_mode:
        typer.echo("ERROR: --cdc2/--serial または --cdc3/--log を指定してください", err=True)
        raise typer.Exit(1)

    if bridge_mode and (cdc2 is None or serial_port is None):
        typer.echo("ERROR: --cdc2 と --serial は両方指定が必要です", err=True)
        raise typer.Exit(1)

    if capture_mode and (cdc3 is None or log_file is None):
        typer.echo("ERROR: --cdc3 と --log は両方指定が必要です", err=True)
        raise typer.Exit(1)

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
        typer.echo("\n[vsp] stopping...")
        stop_event.set()

    signal.signal(signal.SIGINT, handle_signal)
    if hasattr(signal, "SIGTERM"):
        signal.signal(signal.SIGTERM, handle_signal)

    ports = []
    threads = []

    try:
        if bridge_mode:
            typer.echo(f"[vsp] opening CDC2:   {cdc2}")
            typer.echo(f"[vsp] opening serial: {serial_port} @ {baudrate} {bytesize}{parity.upper()}{stopbits:g}")
            cdc2_ser = serial.Serial(cdc2, timeout=0.01)
            real_ser = serial.Serial(
                serial_port,
                baudrate=baudrate,
                bytesize=bytesize_map[bytesize],
                parity=parity_map[parity.upper()],
                stopbits=stopbits_map[stopbits],
                rtscts=rtscts,
                xonxoff=xonxoff,
                timeout=0.01,
            )
            ports += [cdc2_ser, real_ser]
            threads += [
                threading.Thread(target=relay, args=(cdc2_ser, real_ser, "CDC2->serial"), daemon=True),
                threading.Thread(target=relay, args=(real_ser, cdc2_ser, "serial->CDC2"), daemon=True),
            ]

        if capture_mode:
            typer.echo(f"[vsp] opening CDC3:  {cdc3}")
            typer.echo(f"[vsp] log file:      {log_file}")
            cdc3_ser = serial.Serial(cdc3, timeout=0.01)
            ports.append(cdc3_ser)
            threads.append(threading.Thread(target=capture_log, args=(cdc3_ser, log_file), daemon=True))

    except serial.SerialException as e:
        typer.echo(f"[vsp] failed to open port: {e}", err=True)
        for p in ports:
            p.close()
        raise typer.Exit(1)

    for t in threads:
        t.start()

    typer.echo("[vsp] running. press Ctrl+C to stop.")
    stop_event.wait()

    for p in ports:
        p.close()
    typer.echo("[vsp] stopped.")


if __name__ == "__main__":
    app()
