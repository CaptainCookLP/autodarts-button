# 🔴 Autodarts Button

Ein großer, frei programmierbarer USB-Button für den **Seeed Studio XIAO ESP32-S3**.

Der Button kann Tastenkombinationen senden oder sichere Aktionen auf einem Linux-Rechner auslösen. Die Einrichtung erfolgt bequem über eine lokale Webseite – ohne Cloud und ohne Programmierkenntnisse.

## Was kann das Tool?

- **USB-Tastenkombinationen senden**, zum Beispiel `Ctrl + Shift + K`, Pfeiltasten oder F-Tasten
- **Linux-Aktionen auslösen**, zum Beispiel Präsentation weiterschalten, Mikrofon stummschalten oder ein eigenes Skript starten
- Aktionen über ein **einfaches Webinterface** konfigurieren
- WLAN beim ersten Start über einen eigenen Setup-Hotspot einrichten
- Firmware später direkt im Webinterface aktualisieren
- Einstellungen exportieren, importieren oder vollständig zurücksetzen
- Als USB-Tastatur und USB-Serial-Gerät gleichzeitig arbeiten
- Tastendrücke zuverlässig entprellen und pro Druck nur einmal auslösen
- Optional mit einer Chrome-/Chromium-Extension zusammenarbeiten

Alle Einstellungen werden lokal auf dem Gerät gespeichert. Es werden keine Cloud-Dienste benötigt.

## Screenshots

### Webinterface

> 📷 **Coming soon**

Hier wird das Webinterface zur Auswahl der Button-Aktion, Tastenkombination und WLAN-Konfiguration zu sehen sein.

### Browser-Installer

> 📷 **Coming soon**

Hier wird die Installation der Firmware direkt aus Chrome oder Edge zu sehen sein.

### Hardware

> 📷 **Coming soon**

Hier folgen Bilder des XIAO ESP32-S3 und der Verkabelung mit einem großen externen Taster.

## Installation

### 1. Button anschließen

Der Taster wird als potentialfreier **NO-Kontakt** angeschlossen:

```text
Button NO  ───── XIAO D1 / GPIO
Button COM ───── XIAO GND
```

Der interne Pull-up des ESP32-S3 wird automatisch verwendet. Es ist kein zusätzlicher Widerstand erforderlich.

### 2. Firmware im Browser installieren

1. XIAO ESP32-S3 mit einem USB-**Datenkabel** am Computer anschließen.
2. Den Installer in **Google Chrome** oder **Microsoft Edge** öffnen:

   ## 👉 [Red Button jetzt installieren](https://captaincooklp.github.io/autodarts-button/installer/)

3. Auf **Firmware installieren** klicken.
4. Den angezeigten USB-/JTAG-Port auswählen.
5. Warten, bis die Installation abgeschlossen ist.

Falls das Board nicht angezeigt wird:

1. **BOOT** gedrückt halten.
2. **RESET** kurz drücken.
3. **BOOT** loslassen.
4. Die Installation erneut starten.

> Der Browser-Installer benötigt Chrome oder Edge auf einem Desktop-Computer. Firefox, Safari und die meisten mobilen Browser unterstützen das notwendige Web Serial nicht.

### 3. WLAN einrichten

Nach dem ersten Start öffnet der Button ein WLAN mit einem Namen wie:

```text
RedButton-1A2B
```

1. Mit diesem WLAN verbinden.
2. [http://192.168.4.1](http://192.168.4.1) öffnen.
3. Das eigene WLAN auswählen und das Passwort eingeben.
4. Speichern und den Button neu starten lassen.

Danach ist das Webinterface normalerweise hier erreichbar:

## 👉 [http://redbutton.local](http://redbutton.local)

Falls der Name nicht funktioniert, kann die vom Router vergebene IP-Adresse verwendet werden.

## Button konfigurieren

Im Bereich **Button Action** stehen drei Betriebsarten zur Auswahl:

### Disabled

Der Tastendruck löst nichts aus.

### USB HID Shortcut

Der Button verhält sich wie eine USB-Tastatur. Im Webinterface können Taste und Modifier gewählt werden:

- Ctrl
- Shift
- Alt
- GUI / Windows / Super
- Buchstaben und Ziffern
- Pfeil-, Enter-, Tab-, Escape- und Leertaste
- F1 bis F12

Beispiel: `Ctrl + Shift + K`

### USB Serial Event

Der Button sendet eine Action-ID an den Linux-Dienst, zum Beispiel:

```text
presentation_next
mute
custom_1
```

Nur Aktionen, die lokal auf dem Linux-Rechner freigegeben wurden, können ausgeführt werden. Vom Button empfangene Texte werden niemals direkt als Shell-Befehl verwendet.

## Linux-Dienst installieren

Der Linux-Dienst wird nur für **USB Serial Events** benötigt. Für normale Tastenkombinationen ist keine zusätzliche Software notwendig.

1. Auf der [Installer-Seite](https://captaincooklp.github.io/autodarts-button/installer/) das **Linux-Dienst**-Paket herunterladen.
2. Archiv entpacken.
3. Im entpackten Ordner ausführen:

```bash
sudo ./scripts/install-linux.sh
```

4. Danach die erlaubten Aktionen konfigurieren:

```bash
sudo nano /etc/redbutton/actions.json
```

Beispiel:

```json
{
  "actions": {
    "presentation_next": ["/usr/local/bin/presentation-next.sh"],
    "mute": ["/usr/local/bin/toggle-mute.sh"],
    "custom_1": ["/usr/local/bin/custom-action.sh"]
  }
}
```

Status und Log anzeigen:

```bash
systemctl status redbutton-daemon
journalctl -u redbutton-daemon -f
```

Der Dienst verbindet sich nach dem Abziehen des USB-Kabels automatisch erneut.

## Browser-Extension installieren

Die Extension ist optional. Sie kann auf einen vom Button gesendeten Shortcut reagieren und anschließend eine Browser-Aktion ausführen.

1. Auf der [Installer-Seite](https://captaincooklp.github.io/autodarts-button/installer/) die **Chrome Extension** herunterladen.
2. ZIP-Datei entpacken.
3. `chrome://extensions` öffnen.
4. **Entwicklermodus** aktivieren.
5. **Entpackte Erweiterung laden** wählen.
6. Den entpackten Extension-Ordner auswählen.

Der Beispiel-Shortcut ist `Ctrl + Shift + K`. Er kann unter `chrome://extensions/shortcuts` angepasst werden.

## Firmware aktualisieren

Nach der Erstinstallation können Updates ohne erneutes vollständiges Flashen installiert werden:

1. `http://redbutton.local` öffnen.
2. Unter **Firmware Update** die neue `firmware.bin` auswählen.
3. Upload starten.
4. Den automatischen Neustart abwarten.

Während eines Updates darf die Stromversorgung nicht getrennt werden.

## Zurücksetzen

Im Bereich **System** kann das Gerät neu gestartet oder auf Werkseinstellungen zurückgesetzt werden. Beim Factory Reset werden WLAN- und Button-Einstellungen gelöscht. Zur Sicherheit muss der Vorgang ausdrücklich mit `RESET` bestätigt werden.

## Probleme?

### Der Installer findet den XIAO nicht

- Anderes USB-Kabel testen – viele Kabel können nur laden.
- Chrome oder Edge verwenden.
- BOOT gedrückt halten, RESET kurz drücken und BOOT loslassen.

### `redbutton.local` ist nicht erreichbar

- Prüfen, ob Computer und Button im gleichen WLAN sind.
- Die IP-Adresse im Router nachsehen.
- Nach einem fehlgeschlagenen WLAN-Start erneut mit `RedButton-XXXX` verbinden.

### Der Tastendruck löst nichts aus

- Verkabelung zwischen `D1` und `GND` prüfen.
- Im Webinterface kontrollieren, ob die Action auf **Disabled** steht.
- Mit **Test Action** prüfen, ob die konfigurierte Aktion grundsätzlich funktioniert.

### Der Linux-Dienst reagiert nicht

```bash
systemctl status redbutton-daemon
journalctl -u redbutton-daemon -f
```

Außerdem prüfen, ob die verwendete Action-ID in `/etc/redbutton/actions.json` vorhanden ist.

## Downloads

Alle benötigten Downloads befinden sich auf einer Seite:

## 👉 [https://captaincooklp.github.io/autodarts-button/installer/](https://captaincooklp.github.io/autodarts-button/installer/)

- Firmware-Installer
- Chrome-/Chromium-Extension
- Linux-Dienst
- SHA-256-Prüfsummen

## Lizenz

Dieses Projekt steht unter der [GNU General Public License v3.0](LICENSE).
