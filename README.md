# ESP-Mailbox
This projekt is made for a ESP32C3 (could possibly work with other models) to report when a mailbox is opened. 
### Implemented functions
 - Deep sleep
 - Wi-Fi
 - MQTT
 - Ultrasonic distance sensor (In its own branch)
### Project configuration
This project uses platformIO so we can access the configuration with: \
```pio run -t menuconfig``` \
The menu Mailbox contains all the project specific configuration settings, the rest is espressif basics. The RTC IO interrupt pin is meant as a pullup input.
