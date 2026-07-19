# CW Trainer

Un antrenor de telegrafie Morse (CW) bazat pe un Arduino Nano cu USB-C. Schița
alege aleatoriu o literă sau cifră, utilizatorul o reproduce cu un buton, iar
LCD-ul și buzzerul oferă feedback imediat.

## Funcționare

1. La pornire, ecranul afișează viteza configurată (implicit **15 WPM**).
2. Dispozitivul alege aleatoriu un caracter din setul `A`–`Z` și `0`–`9`,
   afișează pe LCD caracterul și codul Morse, apoi redă ritmul acestuia prin
   buzzer.
3. Apăsați cheia pentru a reproduce codul auzit:
   - o apăsare mai scurtă de două unități de timp este un punct;
   - o apăsare mai lungă este o linie;
   - o pauză de cinci unități de timp încheie caracterul.
4. Ecranul arată `CORECT!`, `GRESIT` sau `Timp expirat`, împreună cu răspunsul
   așteptat. Răspunsul trebuie început în cel mult 10 secunde.

## BOM (Bill of Materials)

| Cantitate | Componentă | Specificație / observații |
| ---: | --- | --- |
| 1 | [Arduino Nano V3.0 compatibil, USB-C](https://www.emag.ro/modul-nano-v3-0-cu-usb-c-compatibil-cu-arduino-arduino-nano-328-usbc/pd/DVW798MBM/) | Placă compatibilă Arduino Nano, cu ATmega328P și conector USB-C. |
| 1 | [LCD 1602 IIC/I²C verde](https://www.emag.ro/ecran-lcd-1602-iic-i2c-verde-ai849/pd/D9WQLTMBM/) | Afișaj 16×2 cu I²C, compatibil `LiquidCrystal_I2C`; adresa implicită din schiță este `0x27`. |
| 1 | [Buzzer piezoelectric activ 3–24 V HND-2312](https://www.emag.ro/buzzer-piezoelectric-activ-3-24v-hnd-2312-sjduen-buzzer-hnd-2312/pd/DSK1PD2BM/) | Se conectează la D8 și GND. Are oscilator intern, deci schița îl pornește/oprește pentru a reda ritmul Morse. |
| 1 | [Buton fără reținere, NO, roșu, F22 mm](https://www.emag.ro/buton-de-pornire-fara-blocare-no-rosu-f22mm-05718/pd/DJTBGF3BM/) | Contact normal deschis; se conectează între D7 și GND și înlocuiește cheia Morse. |
| 1 | Breadboard | Opțional, pentru prototipare. |
| 1 set | Fire jumper | Pentru toate conexiunile. |
| 6 | Șurub M3 × 6 mm | Pentru fixarea componentelor în carcasă. |
| 1 | Cablu USB-A–USB-C sau USB-C–USB-C | Pentru programarea și alimentarea plăcii Nano. |

Nu este necesară o rezistență de pull-up pentru cheie: schița activează
rezistența internă cu `INPUT_PULLUP`.

## Conexiuni

| Modul / semnal | Arduino Nano | Note |
| --- | --- | --- |
| LCD VCC | 5V | Verificați tensiunea modulului LCD. |
| LCD GND | GND | Masă comună. |
| LCD SDA | A4 / SDA | Magistrala I²C. |
| LCD SCL | A5 / SCL | Magistrala I²C. |
| Buzzer `+` | D8 | Buzzer activ 3–24 V; este pornit/oprit digital. |
| Buzzer `−` | GND |  |
| Buton NO, un contact | D7 | Intrare configurată `INPUT_PULLUP`. |
| Buton NO, celălalt contact | GND | O apăsare este citită ca `LOW`. |

Pe Nano, pinii I²C sunt A4/SDA și A5/SCL. Dacă LCD-ul nu afișează nimic,
verificați cablajul și încercați adresa `0x3F` în constructorul LCD din schiță.

## Instalare și încărcare

1. Instalați [Arduino IDE](https://www.arduino.cc/en/software).
2. Din Library Manager instalați biblioteca **LiquidCrystal I2C** (header
   `LiquidCrystal_I2C.h`). Biblioteca `Wire` este inclusă în Arduino IDE.
3. Deschideți `cw_trainer.ino` în Arduino IDE.
4. Selectați placa și portul serial corecte din meniul **Tools**.
5. Compilați și încărcați schița.

## Configurare

Parametrii principali se află la începutul fișierului `cw_trainer.ino`:

| Parametru | Valoare implicită | Rol |
| --- | ---: | --- |
| `buzzerPin` | `8` | Pinul buzzerului. |
| `keyPin` | `7` | Pinul cheii Morse. |
| `wpm` | `15` | Viteza de antrenament; durata punctului este calculată ca `1200 / wpm`. |
| `answerTimeout` | `10000` ms | Timpul maxim pentru a începe răspunsul. |

### Setul de exerciții

Fiecare rundă poate conține una dintre cele **36 de caractere** de mai jos:

- literele `A`–`Z`;
- cifrele `0`–`9`.

Selecția aleatorie folosește automat dimensiunea setului de caractere definit
în schiță, astfel încât toate literele și cifrele au aceeași șansă de a apărea.
Cifrele folosesc codurile Morse internaționale de cinci semne: de exemplu,
`0` este `-----`, `5` este `.....`, iar `9` este `----.`.

## Depanare

- **LCD gol:** ajustați potențiometrul de contrast de pe modul și încercați
  adresa `0x3F` în loc de `0x27`.
- **Butonul pare inversat:** confirmați că este legat între D7 și GND, nu la 5V.
- **Nu se aude buzzerul:** verificați polaritatea (`+` la D8 și `−` la GND) și
  faptul că modelul este unul activ, cu oscilator intern.
