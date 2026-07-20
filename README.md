# CW Trainer

An Arduino Nano–based Morse code (CW) trainer with USB-C. The sketch randomly
selects a letter or digit for the user to reproduce with a button, while the LCD
and buzzer provide immediate feedback.

<p align="center">
  <img src="images/2-removebg-preview.png" alt="Front view of the CW Trainer" width="48%">
  <img src="images/cw-trainer-front.png" alt="Perspective view of the CW Trainer" width="48%">
</p>

## How it works

1. At startup, the display shows the configured speed (**15 WPM** by default).
2. The device randomly selects a character from `A`–`Z` and `0`–`9`, displays
   the character and its Morse code on the LCD, and plays its rhythm through the
   buzzer.
3. Press the key to reproduce the code you heard:
   - a press shorter than two time units is a dot;
   - a longer press is a dash;
   - a five-time-unit pause completes the character.
4. The display shows `CORECT!`, `GRESIT`, or `Timp expirat`, together with the
   expected answer. The response must start within 10 seconds.

## Bill of materials

| Quantity | Component | Specification / notes |
| ---: | --- | --- |
| 1 | [Arduino Nano V3.0 compatible, USB-C](https://www.emag.ro/modul-nano-v3-0-cu-usb-c-compatibil-cu-arduino-arduino-nano-328-usbc/pd/DVW798MBM/) | Arduino Nano-compatible board with an ATmega328P and USB-C connector. |
| 1 | [Green LCD 1602 IIC/I²C](https://www.emag.ro/ecran-lcd-1602-iic-i2c-verde-ai849/pd/D9WQLTMBM/) | 16×2 I²C display compatible with `LiquidCrystal_I2C`; the sketch uses `0x27` as its default address. |
| 1 | [Active piezo buzzer, 3–24 V HND-2312](https://www.emag.ro/buzzer-piezoelectric-activ-3-24v-hnd-2312-sjduen-buzzer-hnd-2312/pd/DSK1PD2BM/) | Connects to D8 and GND. Its internal oscillator lets the sketch turn it on and off to play the Morse rhythm. |
| 1 | [Momentary, normally open red button, F22 mm](https://www.emag.ro/buton-de-pornire-fara-blocare-no-rosu-f22mm-05718/pd/DJTBGF3BM/) | Connects between D7 and GND and serves as the Morse key. |
| 1 | Breadboard | Optional, for prototyping. |
| 1 set | Jumper wires | For all connections. |
| 6 | M3 × 6 mm screws | For securing components in the enclosure. |
| 1 | USB-A–to–USB-C or USB-C–to–USB-C cable | For programming and powering the Nano board. |

No pull-up resistor is required for the key: the sketch enables the internal
resistor with `INPUT_PULLUP`.

## Connections

| Module / signal | Arduino Nano | Notes |
| --- | --- | --- |
| LCD VCC | 5V | Check the LCD module voltage. |
| LCD GND | GND | Common ground. |
| LCD SDA | A4 / SDA | I²C bus. |
| LCD SCL | A5 / SCL | I²C bus. |
| Buzzer `+` | D8 | Active 3–24 V buzzer; switched digitally on and off. |
| Buzzer `−` | GND |  |
| One button contact | D7 | Input configured with `INPUT_PULLUP`. |
| Other button contact | GND | A press is read as `LOW`. |

On the Nano, the I²C pins are A4/SDA and A5/SCL. If the LCD displays nothing,
check the wiring and try `0x3F` in the sketch's LCD constructor.

## Installation and upload

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Use the Library Manager to install **LiquidCrystal I2C** (the
   `LiquidCrystal_I2C.h` header). The `Wire` library is included with Arduino IDE.
3. Open `cw_trainer.ino` in Arduino IDE.
4. Select the correct board and serial port from the **Tools** menu.
5. Compile and upload the sketch.

## Configuration

The main parameters are at the beginning of `cw_trainer.ino`:

| Parameter | Default value | Purpose |
| --- | ---: | --- |
| `buzzerPin` | `8` | Buzzer pin. |
| `keyPin` | `7` | Morse key pin. |
| `wpm` | `15` | Training speed; dot duration is calculated as `1200 / wpm`. |
| `answerTimeout` | `10000` ms | Maximum time allowed to start the response. |

### Exercise set

Each round can use one of these **36 characters**:

- letters `A`–`Z`;
- digits `0`–`9`.

Random selection automatically uses the size of the character set defined in
the sketch, giving every letter and digit an equal chance of appearing. Digits
use the five-symbol International Morse codes: for example, `0` is `-----`,
`5` is `.....`, and `9` is `----.`.

## Troubleshooting

- **Blank LCD:** adjust the module contrast potentiometer and try address
  `0x3F` instead of `0x27`.
- **Button behaves in reverse:** confirm that it is wired between D7 and GND,
  not 5V.
- **No buzzer sound:** check the polarity (`+` to D8 and `−` to GND) and verify
  that the model is active with an internal oscillator.
