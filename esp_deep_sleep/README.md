### Configure the project

```
idf.py menuconfig
```

* **Touch wake up** can be enabled/disabled via `Example configuration > Enable touch wake up`
* **ULT wake up** can be enabled/disabled via `Example configuration > Enable temperature monitoring by ULP`

Wake up sources that are unused or unconnected should be disabled in configuration to prevent inadvertent triggering of wake up as a result of floating pins.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(Replace PORT with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.


```
