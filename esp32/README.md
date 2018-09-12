# ESP32
**Board: Espressif esp32-devKitC**

Project is generated via the following platformio-command:
```bash
pio init -b esp32dev --ide vscode -O framework=espidf -O monitor_speed=115200
```
to get the correct framework etc.


Project is (built and) uploaded and automatically attached to monitor like this:
```
pio run -t upload -t monitor
```