# CW Trainer v1.0.0

Prima versiune stabilă a proiectului **CW Trainer** pentru Arduino Uno și
Arduino Nano. Schița transformă placa într-un exercițiu interactiv de telegrafie
Morse: ascultați caracterul redat, îl reproduceți cu cheia CW și primiți imediat
feedback pe LCD și prin buzzer.

## Noutăți

- Exerciții aleatorii pentru literele `A`–`Z` și afișarea caracterului împreună
  cu codul Morse înainte de redare.
- Redare audio CW la 700 Hz, cu viteză configurabilă de 15 WPM în mod implicit.
- Interpretarea apăsărilor cheii ca puncte și linii, inclusiv protecție pentru
  impulsuri de contact foarte scurte.
- Feedback clar pentru răspuns corect, greșit sau expirat, cu afișarea
  răspunsului așteptat.
- Documentație pentru componente, cablare, instalare, configurare și depanare.

## Compatibilitate și cerințe

- Placă Arduino compatibilă Uno sau Nano (ATmega328P recomandat).
- LCD I²C 16×2 compatibil cu biblioteca `LiquidCrystal_I2C` (adresa implicită
  este `0x27`).
- Buzzer pasiv/piezo pe D8 și cheie Morse sau buton momentan între D7 și GND.
- Biblioteca `LiquidCrystal_I2C`, instalată din Arduino Library Manager.

Consultați [README.md](README.md) pentru schema conexiunilor și pașii compleți
de încărcare a schiței `cw_trainer.ino`.
