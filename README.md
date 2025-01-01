SensorProjekt

Beschreibung

Das SensorProjekt ist ein ESP32-basiertes System zur Sensormessung und Visualisierung von Umgebungsdaten. Es verwendet das Arduino-Framework und wird in PlatformIO innerhalb von Visual Studio Code entwickelt. Ziel des Projekts ist es, verschiedene Umweltdaten (z. B. Temperatur, Luftfeuchtigkeit, UV-Intensität) zu messen und über ein Webinterface anzuzeigen.

Funktionen

Sensormessung:

Unterstützung für mehrere Sensoren (z. B. Temperatur, Luftfeuchtigkeit, UV-Intensität).

Daten werden regelmäßig erfasst und geglättet.

Webinterface:

Anzeige der gemessenen Daten in Echtzeit.

Graphische Visualisierung der Daten.

Datenübertragung:

Speicherung der Daten auf einem MySQL-Server für Langzeitüberwachung.

Hardware:

Steuerung eines Servo-Systems, das sich automatisch auf den besten Lichtwert ausrichtet (z. B. Lichtverfolgung).

Anforderungen

Software

PlatformIO in Visual Studio Code

Arduino Framework

Git für Versionierung

Hardware

ESP32-Board

Sensoren (z. B. DHT22, LDRs, UV-Sensor)

Servos für Bewegungssteuerung

LED zur Simulation von Aktionen

Installation

Repository klonen:

git clone https://github.com/callmepain/SensorProjekt.git

PlatformIO-Projekt öffnen:

Öffne Visual Studio Code.

Lade das geklonte Projekt.

Abhängigkeiten installieren:

PlatformIO installiert automatisch alle benötigten Bibliotheken, die im platformio.ini definiert sind.

Firmware hochladen:

Schließe den ESP32 an deinen Computer an.

Klicke in PlatformIO auf "Upload".

Nutzung

Starte das ESP32-Board nach dem Hochladen der Firmware.

Öffne das Webinterface über die angegebene IP-Adresse im seriellen Monitor.

Beobachte die Messwerte und passe die Einstellungen nach Bedarf an.

Lizenz

Dieses Projekt steht unter der MIT-Lizenz.

Kontakt

Wenn du Fragen oder Feedback hast, erstelle ein Issue im Repository oder kontaktiere mich direkt über GitHub: callmepain.