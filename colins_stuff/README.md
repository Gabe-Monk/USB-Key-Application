# To build (Windows & WSL2)
1. Powershell: `winget install usbipd-win`
2. Close that Powershell terminal and open a new one.
3. Do `usbipd list` in Powershell with the device disconnected.
4. Plug the device into the USB port you plan to use it with and do `usbipd list` again, seeing which entry is there now that wasn't before. Take note of its BUSID (e.g., "1-2").
5. In Powershell, do `usbipd bind --force --busid <YOUR BUSID>`, then do `usbipd attach --wsl --busid <YOUR BUSID>`. You may have to reattach it every time you plug it back in (but if you're gonna be unplugging often, add `--auto-attach` to that command and leave the Powershell terminal running).
6. In WSL: `sudo apt install libserialport-dev`
7. In WSL, build via `g++ main.cpp -o serial_test -lserialport`
8. When done, do `usbipd unbind --busid <YOUR BUSID>` in Powershell