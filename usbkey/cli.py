from __future__ import annotations

import time
from pathlib import Path
import typer
from rich import print
from rich.panel import Panel

from usbkey.config import Settings
from usbkey.device.factory import make_device
from usbkey.workflows import encrypt_flow, decrypt_flow, ValidationError
from usbkey.logging_utils import setup_logging

app = typer.Typer(add_completion=False, help="USB Key host application (CLI).")

@app.callback()
def _cb():
    setup_logging()

@app.command()
def status():
    """Show current USB key status."""
    settings = Settings()
    dev = make_device(settings)
    dev.connect()
    st = dev.status()
    dev.close()
    print(Panel.fit(
        f"[b]Serial:[/b] {st.serial}\n[b]Firmware:[/b] {st.firmware}\n[b]Fingerprint enrolled:[/b] {st.fingerprint_enrolled}",
        title="USB Key Status"
    ))

@app.command()
def watch(interval_s: float = 1.0):
    """Continuously poll the device status (useful for insertion/removal testing)."""
    settings = Settings()
    dev = make_device(settings)
    print("[dim]Polling device status. Ctrl+C to stop.[/dim]")
    while True:
        try:
            dev.connect()
            st = dev.status()
            print(f"[green]online[/green] serial={st.serial} enrolled={st.fingerprint_enrolled}")
        except Exception as e:
            print(f"[red]offline[/red] {e}")
        time.sleep(interval_s)

@app.command()
def enroll():
    """Enroll a fingerprint on the USB key (delegated to firmware)."""
    settings = Settings()
    dev = make_device(settings)
    dev.connect()
    print("Starting fingerprint enrollment. Follow device prompts (LEDs / sensor).")
    dev.enroll_fingerprint()
    print("[green]Enrollment completed.[/green]")
    dev.close()

@app.command()
def auth():
    """Run fingerprint authentication and print the result."""
    settings = Settings()
    dev = make_device(settings)
    dev.connect()
    res = dev.authenticate(timeout_s=15.0)
    dev.close()
    if res.accepted:
        print(f"[green]Accepted[/green] ({res.elapsed_ms} ms)")
    else:
        print(f"[red]Rejected[/red]: {res.reason}")

@app.command()
def encrypt(path: Path, recipient_serial: str = typer.Option(..., "--recipient-serial", "-r")):
    """Create a demo envelope for a recipient serial (NOT secure)."""
    settings = Settings()
    try:
        out = encrypt_flow(settings, path, recipient_serial)
        print(f"[green]Wrote envelope:[/green] {out}")
    except ValidationError as e:
        print(f"[red]Error:[/red] {e}")
        raise typer.Exit(code=2)

@app.command()
def decrypt(path: Path):
    """Decrypt a demo envelope after device + fingerprint verification."""
    settings = Settings()
    dev = make_device(settings)
    try:
        out_path, cleanup = decrypt_flow(settings, dev, path)
    except ValidationError as e:
        print(f"[red]Error:[/red] {e}")
        raise typer.Exit(code=2)

    print(Panel.fit(
        f"[b]Decrypted file available at:[/b]\n{out_path}\n\nKeep the device connected. Press Enter to purge and exit.",
        title="Decrypted File"
    ))
    try:
        input()
    finally:
        cleanup()
        print("[yellow]Purged decrypted file.[/yellow]")

if __name__ == "__main__":
    app()
