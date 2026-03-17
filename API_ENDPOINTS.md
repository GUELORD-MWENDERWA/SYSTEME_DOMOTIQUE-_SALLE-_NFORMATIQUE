# Documentation des Endpoints API - Domotique

## Pages Web (GET)

```
http://192.168.x.x/                 → index.html (dash principal)
http://192.168.x.x/index.html       → Dashboard complet
http://192.168.x.x/config.html      → Configuration WiFi (AP mode)
http://192.168.x.x/style.css        → Styles CSS
http://192.168.x.x/app.js           → JavaScript
```

## API Endpoints

### 🔌 Système & Énergie

```
GET /api/system
Retourne: voltage, current, power, frequency, counters, relays status, mode, daynight, alarmes
Réponse: {"voltage":230.5,"current":0.25,"power":57,"frequency":50.0,...}
```

### 🔄 Contrôle Relais

```
POST /api/relay?id=0
Bascule le relais (0-7)
Relais disponibles:
  0 → Lampe intérieure (Q0_LAMP_INSIDE)
  1 → Lampe extérieure (Q1_LAMP_OUTSIDE)
  2 → Prise 1 (Q2_PRISE1)
  3 → Prise 2 (Q3_PRISE2)
  4 → Ventilateur (Q4_FAN)
  5 → Buzzer (Q5_BUZZER)
  6 → LED Rouge (Q6_LED_RED)
  7 → LED Verte (Q7_LED_GREEN)

Réponse: {"relay":0,"state":true,"name":"Lampe intérieure"}
```

### 🔐 Gestion Cartes RFID

```
GET /api/rfid/list
Liste toutes les cartes RFID enregistrées
Réponse: [{"uid":"0x12345678","name":"Jean","authorized":true,"isInside":false}, ...]

POST /api/rfid/register
Enregistre une nouvelle carte RFID
Body: {"uid":"0x12345678","name":"Jean"}
Réponse: {"status":"ok","uid":"0x12345678","name":"Jean"}

POST /api/rfid/delete?uid=0x12345678
Supprime une carte RFID
Réponse: {"status":"ok","deleted":"0x12345678"}
```

### 💻 Commandes Système

```
POST /api/cmd?cmd=ZERO
Reset complet du système (relais, compteurs, alarmes)
Réponse: {"status":"ok","message":"System reset"}

POST /api/cmd?cmd=RESET
Reset des compteurs uniquement
Réponse: {"status":"ok","message":"Counters reset"}

POST /api/cmd?cmd=RELAY
Affiche l'état de tous les relais
Réponse: {"status":"ok","relays":[false,false,true,...]}

POST /api/cmd?cmd=REBOOT
Redémarre l'ESP32
Réponse: Redémarrage en ~3 secondes
```

### 🔧 Configuration WiFi

```
GET /api/config/wifi?ssid=702SH_AP&password=12345678___1
Configuration rapide via URL (AP mode)
Réponse: {"status":"success","message":"WiFi configured. Rebooting..."}

POST /api/config/wifi
Configuration POST (AP mode)
Body: {"ssid":"702SH_AP","password":"12345678___1"}
Réponse: {"status":"success","message":"WiFi configured. Rebooting..."}
```

### 🔍 Diagnostiques

```
GET /api/files
Liste les fichiers en SPIFFS (pour debug)
Réponse: ["index.html","style.css","app.js","config.html"]
```

## Paramètres WiFi Par Défaut

```
SSID: 702SH_AP
Mot de passe: 12345678___1
```

## Mode d'Accès Point (AP)

Si la connexion WiFi échoue:

- **SSID AP**: Domotique_Setup
- **Password AP**: domotique2024
- **IP AP**: http://192.168.4.1
- **Configuration**: http://192.168.4.1/config.html

## Exemples de Requêtes

### cURL

```bash
# Obtenir status système
curl http://192.168.x.x/api/system

# Basculer relais 0 (lampe intérieure)
curl -X POST http://192.168.x.x/api/relay?id=0

# Lister cartes RFID
curl http://192.168.x.x/api/rfid/list

# Reset système
curl -X POST http://192.168.x.x/api/cmd?cmd=ZERO

# Configurer WiFi
curl -X POST http://192.168.x.x/api/config/wifi \
  -H "Content-Type: application/json" \
  -d '{"ssid":"MonReseau","password":"MonMotdepasse"}'
```

### JavaScript Fetch

```javascript
// Obtenir données système
const data = await fetch("/api/system").then((r) => r.json());
console.log(data.current, data.voltage, data.power);

// Basculer relais
await fetch("/api/relay?id=0", { method: "POST" });

// Lister RFID
const cards = await fetch("/api/rfid/list").then((r) => r.json());

// Configurer WiFi
await fetch("/api/config/wifi", {
  method: "POST",
  headers: { "Content-Type": "application/json" },
  body: JSON.stringify({ ssid: "702SH_AP", password: "12345678___1" }),
});
```

## Codes de Réponse HTTP

```
200 OK           → Requête réussie
400 Bad Request  → Paramètres invalides
404 Not Found    → Endpoint inexistant
500 Server Error → Erreur système
```

## Notes Importantes

- Tous les endpoints retournent du JSON (sauf les fichiers statiques)
- L'API fonctionne en **HTTP** (pas HTTPS)
- La plupart des endpoints retournent rapidement (<100ms)
- Les données WiFi sont sauvegardées en EEPROM
