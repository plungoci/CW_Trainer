# CW Trainer

Un antrenor de telegrafie Morse (CW) pentru Arduino Nano sau UNO. Schița redă aleatoriu o
literă, utilizatorul o reproduce cu o cheie Morse, iar LCD-ul și buzzerul oferă
feedback imediat.

## Funcționare

1. La pornire, ecranul afișează viteza configurată (implicit **15 WPM**).
2. Dispozitivul alege aleatoriu un caracter din setul `A`–`Z` și `0`–`9`,
   afișează pe LCD caracterul și codul Morse care urmează să fie ascultat, apoi
   îl redă la 700 Hz.
3. Apăsați cheia pentru a reproduce codul auzit:
   - o apăsare mai scurtă de două unități de timp este un punct;
   - o apăsare mai lungă este o linie;
   - o pauză de cinci unități de timp încheie caracterul.
4. Ecranul arată `CORECT!`, `GRESIT` sau `Timp expirat`, împreună cu răspunsul
   așteptat. Răspunsul trebuie început în cel mult 10 secunde.

## BOM (Bill of Materials)

| Cantitate | Componentă | Specificație / observații |
| ---: | --- | --- |
| 1 | Placă Arduino compatibilă | Arduino Uno sau Nano (ATmega328P) recomandat. |
| 1 | LCD cu I²C | 16×2, controler compatibil `LiquidCrystal_I2C`, adresă implicită `0x27`. |
| 1 | Buzzer pasiv piezo | Pentru redarea tonurilor Morse; conectat la pinul digital 8. |
| 1 | Cheie Morse sau buton momentan | Contact normal deschis; conectat între pinul digital 7 și GND. |
| 1 | Breadboard | Opțional, pentru prototipare. |
| 1 set | Fire jumper | Pentru toate conexiunile. |
| 1 | Cablu USB | Pentru programarea și alimentarea plăcii. |

Nu este necesară o rezistență de pull-up pentru cheie: schița activează
rezistența internă cu `INPUT_PULLUP`.

## Conexiuni

| Modul / semnal | Arduino Uno / Nano | Note |
| --- | --- | --- |
| LCD VCC | 5V | Verificați tensiunea modulului LCD. |
| LCD GND | GND | Masă comună. |
| LCD SDA | A4 / SDA | Magistrala I²C. |
| LCD SCL | A5 / SCL | Magistrala I²C. |
| Buzzer `+` | D8 | Buzzerul pasiv este comandat cu `tone()`. |
| Buzzer `−` | GND |  |
| Cheie, un contact | D7 | Intrare configurată `INPUT_PULLUP`. |
| Cheie, celălalt contact | GND | O apăsare este citită ca `LOW`. |

Pentru alte plăci Arduino, folosiți pinii I²C marcați `SDA` și `SCL` în locul
numerelor A4/A5. Dacă LCD-ul nu afișează nimic, verificați cablajul și încercați
adresa `0x3F` în constructorul LCD din schiță.

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
| `cwFrequency` | `700` Hz | Frecvența tonului CW. |
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
- **Cheia pare inversată:** confirmați că este legată între D7 și GND, nu la 5V.
- **Nu se aude tonul:** utilizați un buzzer pasiv/piezo; un buzzer activ nu va
  reda corect frecvențele generate de `tone()`.
