# Guide Complet de Débogage - Domotique v1.0

## ✅ État Actuel
- **Compilation:** ✓ OK (Exit Code 0)
- **Intégration:** ✓ Complète - Tous les systèmes intégrés
- **Code:** ✓ Cohérent et synchronisé

---

## 1. TEST RELAY 74HC595 - ÉTAPES CRITIQUES

### Diagnostic Rapide
Pour vérifier si le registre fonctionne:

1. **Servez le port série** à 115200 baud
2. Accédez au menu serial et tapez: `test relay`
3. Observez la console pour les messages

### Ce Qui Doit se Passer
```
Output Q0 (Lampe intérieure): ON → OFF
Output Q1 (Lampe extérieure): ON → OFF
Output Q2-Q7: Chacun s'allume puis s'éteint pendant 150ms
Buzzer et LEDs doivent activer simplement (pas d'alerte)
```

### Points de Vérification Physiques
| Pin ESP32 | Pin HC595 | Signal | État Attendu |
|-----------|-----------|--------|--------------|
| GPIO 26 | STCP (Latch) | Impulsion | Pulse: LOW→HIGH→LOW |
| GPIO 14 | SHCP (Clock) | Horloge rapide | ~50kHz frequency |
| GPIO 27 | DS (Data) | Données série | Pattern: 10000000 (LSB first) |

### Timing Critique (OBLIGATOIRE pour ESP32)
- **Délai avant/après Latch:** 20µs minimum
- **Délai Clock HIGH/LOW:** 10µs chacun
- **Délai Data setup:** 10µs avant Clock
- **Délai après tout:** 10µs avant prochain bit

Le code utilise maintenant:
```cpp
delayMicroseconds(20);  // Avant latch
delayMicroseconds(10);  // Data setup
delayMicroseconds(20);  // Clock pulse
delayMicroseconds(10);  // Après clock
delayMicroseconds(20);  // Latch finalization
```

### Si Ça Ne Marche Pas:
1. **Vérifiez les fils:** Latch pin 26, Clock pin 14, Data pin 27
2. **Tensions:** 5V sur Vcc (HC595), GND sur pin de masse
3. **Capacité:** Ajouter 100nF entre Vcc et GND si oscillations observées
4. **Horloge:** Si horloge trop rapide, augmentez les delayMicroseconds

---

## 2. TEST RFID DUAL READERS

### Lecteur 0 (ENTRY) vs Lecteur 1 (EXIT)

**Connectivité SPI (partagée):**
| Signal | Pin ESP32 | Pin MFRC522 |
|--------|-----------|-------------|
| MOSI | GPIO 23 | MOSI |
| MISO | GPIO 19 | MISO |
| CLK | GPIO 18 | SCK |
| **CS Lecteur 0** | **GPIO 5** | **SDA** |
| **CS Lecteur 1** | **GPIO 17** | **SDA** |
| RST (partagé) | GPIO 16 | RST |

### Diagnostic
```bash
# Via série, tapez:
listcards
```

Cela doit afficher 3 cartes par défaut:
```
[RFID] Authorized Cards:
[1] UID: 12345678 | CARD_12345678
[2] UID: 87654321 | CARD_87654321
[3] UID: DEADBEEF | CARD_DEADBEEF
```

### Test de Scanning
1. Rapprochez une carte de **Lecteur 0 (ENTRY)**
2. Regardez le port série pour: `[RFID_ENTRY] Card: XXXXXXXX`
3. Reprendre l'étape 1 pour **Lecteur 1 (EXIT)** avec message `[RFID_EXIT]`

### Dépannage Lecteur 2
Si le lecteur EXIT ne fonctionne pas:

**Le code now does:**
- Initialise le pin SS individuel pour chaque lecteur
- Ajoute délais entre les inits (100ms)
- Met à jour seulement le lecteur sélectionné

**En cas de problème:**
1. Vérifiez que **GPIO 17 est libre** (non utilisé ailleurs)
2. Testez avec un oscilloscope: Le CS pin 17 doit passer LOW quand on scan
3. Vérifiez la résistance pull-up sur le SPI (généralement 10kΩ nécessaire)

---

## 3. TEST DÉTECTEUR DE MOUVEMENT (PIR)

### Comportement Normal
- **Pas de mouvement:** Aucun message intrusion
- **Mouvement de nuit + ABSENCE:** Message `[MOTION] MOTION DETECTED AT NIGHT`
- **Mouvement le jour:** Ignoré
- **Mouvement + PRÉSENCE:** Ignoré (pas d'intrusion si quelqu'un est dedans)

### Débounce
- Après 5 lectures HIGH consécutives = Motion confirmée
- À ~50ms de polling = ~250ms délai de confirmation
- Cooldown 2 secondes après détection

### Diagnostic
Via série (dans le menu):
```bash
test motion
```

Cela affichera pendant 5 secondes:
```
Raw: HIGH | Debounced: YES | Consecutive reads: 5
✓ MOTION EVENT DETECTED!
```

### Dépannage
| Théorie | Diagnostic |
|---------|-----------|
| Pin flottant | Utilisz INPUT, pas INPUT_PULLDOWN |
| Polarité inversée | Passez à HIGH au lieu de LOW? Vérifiez spec sensor |
| Pas de polling rapide | motionSensor.update() doit être appelé chaque 10-20ms |
| Pas de réso | Assurez que lastMotionTrigger cooldown testé |

---

## 4. TEST BUTTONS

### MODE Button (GPIO 4)
- **Appui court:** Ignoré (INPUT_PULLUP, besoin long press)
- **Appui long (800ms):** Basculer entre ACCESS et REGISTRATION mode
- **Feedback:** LEDs différentes pour chaque mode (rouge = enregistrement)

### LAMP Button (GPIO 15)
- **Appui court:** Basculer ON/OFF lampe intérieure
- **Feedback:** Lampe doit s'allumer/éteindre immédiatement

### Test Debounce
Appuyez et tenez les boutons rapidement. Si l'action se répète accidentellement, c'est un problème de debounce (actuellement 50ms).

---

## 5. TEST INTRUSION ALARM

### Séquence Normale
1. **Jour or Présence:** Aucune alarme, LED rouge OFF
2. **Nuit + Absence + Motion:** 
   - Alarme démarre (3 bips + LED rouge ON)
   - Se répète toutes les 5 secondes
3. **RFID ENTRY scannée:**
   - Intrusion désactivée immédiatement
   - LED rouge OFF
   - Alarme arrêtée

### Vérification Code
Dans main.cpp, boucle principale:
```cpp
if (globalSystemStatus.daynight == STATUS_NIGHT &&
    globalSystemStatus.counters.presence_count == 0 &&
    motionSensor.wasMotionDetected()) {
    // Déclenche intrusion_detected = true
}
```

---

## 6. ARCHITECTURE SYNCHRONISATION COMPLÈTE

### Séquence d'Événement Typique - RFID ENTRY

```
1. rfidManager.scanReader(RFID_ENTRY) détecte carte
   ↓
2. systemLogic.handleRFIDScan(RFID_ENTRY, UID)
   ├→ Vérifie si carte autorisée
   ├→ Incrémente counters.entries_count
   ├→ **DÉSACTIVE intrusion_detected**
   ├→ Appelle executeAccessGrantedScenario()
   │    └→ Servo 0°→180°
   │    └→ LED verte + 1 bip
   │    └→ Servo 180°→0°
   └→ Appelle updateLightingLogic()
   ↓
3. systemLogic.updateSystemState() 
   ├→ Vérifie si intrusion_detected (maintenant FALSE)
   └→ LED rouge OFF (si elle était ON)
```

### État Global Synchronisé
```cpp
SystemStatus globalSystemStatus {
    .current_mode      // ACCESS or REGISTRATION
    .state             // IDLE, CARD_SCANNED, ACCESS_GRANTED, etc
    .daynight          // DAY or NIGHT (de LDR)
    .counters = {
        .presence_count      // entries - exits (RFID)
        .entries_count       // cumul
        .exits_count         // cumul
        .total_access_attempts
        .denied_access_count
    }
    .intrusion_detected  // true si motion(night+absence)
}
```

---

## 7. FLUX COMPLET DE DÉPANNAGE

### Étape 1: Vérifiez Relay
```bash
test relay
```
- Si ❌ Relais non activés: Problème timing ou bus SPI/GPIO
- Si ✓ Relais changent: Passez à Étape 2

### Étape 2: Vérifiez RFID Reader 0 (ENTRY)
```bash
listcards
```
Scannez une carte. Doit voir `[RFID_ENTRY] Card:`.
- Si ❌ Rien: Problèmes connexion/SPI
- Si ✓ Carte détectée: Passez à Étape 3

### Étape 3: Vérifiez RFID Reader 1 (EXIT)  
Scannez une carte à l'autre lecteur. Doit voir `[RFID_EXIT] Card:`.
- Si ❌ Rien: Lecteur 1 non initialisé corretement
  - Vérifiez GPIO 17 libre
  - Vérifiez délais d'init 100ms + 500ms
- Si ✓ Détecté: Passez à Étape 4

### Étape 4: Vérifiez Motion
```bash
test motion
```
Bougez près du capteur. Doit voir débounce progression et "MOTION DETECTED".
- Si ❌ Pas de détection: Capteur pas connecté ou GPIO 12
- Si ✓ Détecté: Passez à Étape 5

### Étape 5: Vérifiez Buttons
Appuyez MODE long (800ms). Mode passe de ACCESS → REGISTRATION.
Appuyez LAMP court. Lampe intérieure bascule.
- Si ❌ Pas de réponse: GPIO 4/15 ou debounce
- Si ✓ Fonctionnent: Tous systèmes OK

### Étape 6: Test Intégration Complète
1. Mode NIGHT (LDR ou forcé)
2. Assurez ABSENCE (ne scannez pas RFID entré)
3. Bougez près capteur mouvement
4. **Doit:**
   - Déclencher alarme (beeps + LED rouge)
   - 5s cooldown avant prochains beeps
5. Scannez une carte valide à lecteur ENTRY
6. **Doit:**
   - Arrêter alarme immédiatement
   - LED rouge OFF
   - Servo ouvre/ferme

---

## 8. POINTS CRITIQUES POUR CHAQUE SYSTÈME

### 🔴 Relay/74HC595
**Problème le plus probable:** Timing des edges trop serré
- Solution: Les délais 20-10µs doivent être respectés
- Test: Oscilloscope sur pins LATCH/CLOCK/DATA

### 🟠 RFID Lecteur 2
**Problème le plus probable:** Init du deuxième lecteur non complète
- Solution: Délais 50/100/500ms entre les étapes
- Test: Regarder la console pour"RFID Reader 1 initialized"

### 🟡 Motion Sensor  
**Problème le plus probable:** Pas de polling régulier
- Solution: motionSensor.update() appelé chaque boucle
- Test: Vérifier "Consecutive reads" progresse de 0→5

### 🟢 Buttons
**Problème le plus probable:** Debounce à 50ms trop court
- Solution: Augmenter BUTTON_DEBOUNCE_MS si besoin  
- Test: Appui rapide doit compter comme 1 seul événement

### 🔵 Alarme Intrusion
**Problème le plus probable:** Pas désactivé par RFID ENTRY
- Solution: handleRFIDScan.(){intrusion_detected=false} pour ENTRY
- Test: Vérifier alarme s'arrête immédiatement post-RFID

---

## 9. Commandes Serial Disponibles

Via le port série à 115200 baud:

| Commande | Fonction |
|----------|----------|
| `test relay` | Test tous les 8 relais séquentiellement |
| `test rfid` | Teste les deux lecteurs RFID |
| `test motion` | Monitor capteur pendant 5s |
| `test button` | Test mode |
| `listcards` | Affiche toutes les cartes enregistrées |
| `mode access` | Force MODE_ACCESS |
| `mode registration` | Force MODE_REGISTRATION |
| `day` | Force STATUS_DAY |
| `night` | Force STATUS_NIGHT |
| `reset` | Reset EEPROM et état |

---

## 10. Résumé des Modifications Appliquées

### relay.cpp ✓
- Délais augmentés: 1-2µs → 10-20µs
- Latch sequence corrigée
- LSB-first bit shift confirmé

### rfid.cpp ✓
- Ajout pinMode pour chaque SS pin
- Délais entre inits: 50ms → 100ms → 500ms
- Init boucle pour les 2 lecteurs

### motion.cpp ✓
- Debounce à 5 lectures consécutives
- Cooldown 2s pour éviter spam
- Update() called chaque boucle

### logic.cpp ✓
- updateRelayOutputs() synchronisé
- updateSystemState() avec cooldown alarmeMaintenance
- Intrusion disabled on RFID ENTRY

### main.cpp ✓
- Motion sensor integration complete
- Button handling (MODE=registration toggle, LAMP=light toggle)
- Dual RFID scanning (both readers in loop)
- Intrude alert sequence (night+absence+motion)

### button.cpp ✓
- INPUT_PULLUP mode confirmed
- Debounce logic functional
- Long press detection (800ms)

---

## 11. À Vérifier en PRIORITÉ

1. **Relay Works?**
   - Oscilloscope sur LATCH/CLOCK/DATA
   - Ou regardez simple "test relay"

2. **Both RFID Read?**
   - Scan carte à chaque lecteur
   - Doit voir [RFID_ENTRY] et [RFID_EXIT]

3. **Motion Detected?**
   - "test motion" pendant 5s
   - Bougez près capteur

4. **Buttons Respond?**
   - Long press MODE → Registration mode
   - Short press LAMP → Light toggle

5. **Intrusion Works?**
   - Force NIGHT mode
   - Bougez capteur
   - Doit entendre beeps + LED rouge
   - Scan RFID → arrête alarme

---

**Code Status: ✅ PRÊT POUR TEST MATÉRIEL**

Tous fichiers compilent sans erreur.
Tous systèmes intégrés et synchronisés.
Prêt pour débogage matériel étape par étape.
