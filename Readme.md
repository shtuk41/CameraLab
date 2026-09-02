# ESP32 Command & Setup Reference

A comprehensive collection of essential commands, environment paths, permission fixes, and workflow steps for ESP32 development with ESP-IDF.

---

## 1. Environment Initialization

Initialize the ESP-IDF environment variables in your current terminal session:

```bash
. /home/aleksander/.espressif/v6.1-beta1/esp-idf/export.sh
```

---

## 2. Project Target & Dependencies

### Set Target Hardware
Configure the project target for a specific microcontroller (e.g., ESP32-S3):
```bash
idf.py set-target esp32s3
```

### Add Component Dependencies
Add external components or libraries from the ESP Component Registry (e.g., the ESP32 camera driver):
```bash
idf.py add-dependency "espressif/esp32-camera"
```

### Reconfigure Project
Regenerate CMake files and apply dependency updates:
```bash
idf.py reconfigure
```

---

## 3. Build & Flash Workflows

### Build Firmware
Compile the project:
```bash
idf.py build
```

### Flash Firmware
Upload the compiled binary to your board (replace `/dev/your_actual_port` with your device port, such as `/dev/ttyUSB0` or `/dev/ttyACM0`):
```bash
idf.py -p /dev/your_actual_port flash
```

---

## 4. Port Discovery & Permissions

### Find Connected Devices
List all available USB/UART serial ports connected to the system:
```bash
ls -l /dev/tty[UA]*
```

### Grant Temporary Port Access
Quickly grant read/write permissions to a specific serial port if permission is denied:
```bash
sudo chmod a+rw /dev/ttyUSB0
# or for ACM devices:
sudo chmod a+rw /dev/ttyACM0
```

### Permanent Permission Setup (Dialout & Plugdev)
Add your user to the `dialout` and `plugdev` groups to access serial ports permanently without using `sudo` (requires a system logout/login or reboot to take effect):
```bash
sudo usermod -aG dialout,plugdev $USER