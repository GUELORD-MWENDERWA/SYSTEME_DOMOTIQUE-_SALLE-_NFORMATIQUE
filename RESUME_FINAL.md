# RÉSUMÉ COMPLET - Code Domotique v1.0 Finalisé

## 🎯 État Final

**Compilation:** ✅ 100% réussite (0 erreurs/warnings)  
**Intégration:** ✅ Tous les systèmes connectés et synchronisés  
**Code Status:** ✅ Prêt pour test matériel sur la planche

---

## 📋 Modifications Apportées

### 1. **relay.cpp** - Timing du Registre HC595

**Problème:** Délais trop courts (1-2µs) causaient des latchs incomplets
**Solution appliquée:**

```cpp
// AVANT (ne fonctionnait pas):
digitalWrite(HC595_CLOCK_PIN, HIGH);
delayMicroseconds(1);
digitalWrite(HC595_CLOCK_PIN, LOW);
delayMicroseconds(1);

// APRÈS (robuste pour ESP32):
digitalWrite(HC595_CLOCK_PIN, HIGH);
delayMicroseconds(20);
digitalWrite(HC595_CLOCK_PIN, LOW);
delayMicroseconds(10);
```

**Impact:** Relais doivent maintenant répondre correctement aux commandes

### 2. **rfid.cpp** - Initialisation Dual Reader

**Problème:** Lecteur 1 (EXIT) ne s'initialisait pas  
**Solution appliquée:**

```cpp
for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    pinMode(ssPins[reader], OUTPUT);
    digitalWrite(ssPins[reader], HIGH);  // NEW: Déselect initially
    delay(50);                            // NEW: Setup delay

    mfrc522[reader].PCD_Init(ssPins[reader], RFID_RST_PIN);
    delay(100);                           // NEW: Post-init stabilization
}
delay(500);                               // NEW: Final settle time
```

**Impact:** Les deux lecteurs RFID doivent maintenant scanner indépendamment

### 3. **motion.cpp** - Debounce Confirmé ✓

Déjà correct, 5 lectures consécutives = ~250ms confirmation

### 4. **logic.cpp** - Intégration Alarme ✓

Déjà correct:

- Alarme tous les 5 secondes (cooldown)
- Désactivée par RFID ENTRY
- LED rouge reste ON tant qu'intrusion_detected

### 5. **button.cpp** - INPUT_PULLUP Confirmé ✓

MODE (pin 4) + LAMP (pin 15) déjà bien configurés

### 6. **main.cpp** - Loop Synchronisée ✓

- Motion update() appelé chaque boucle
- Dual RFID scanReaders() pour ENTRY et EXIT
- Button updates avant RFID processing
- systemLogic.updateSystemState() appelé chaque cycle

---

## 🔧 Configuration Matérielle Confirmée

### Pinouts Critiques

```
RELAY REGISTER (74HC595):
  GPIO 26 → LATCH (STCP)
  GPIO 14 → CLOCK (SHCP)
  GPIO 27 → DATA (DS)

RFID READERS (x2 MFRC522):
  Lecteur 0 (ENTRY):   SS = GPIO 5
  Lecteur 1 (EXIT):    SS = GPIO 17
  RST (partagé):       GPIO 16
  SPI Bus (partagé):   MOSI=23, MISO=19, CLK=18

PIR MOTION:
  GPIO 12 → Capteur HC-SR501

BUTTONS:
  GPIO 4  → MODE (long press = registration toggle)
  GPIO 15 → LAMP (short press = lamp toggle)

LDR:
  GPIO 35 → Light sensor (day/night detection)

LCD:
  I2C 0x27 (20x4 display)

ENERGY:
  GPIO 32/33 → PZEM-004T (9600 baud)

SERVO:
  GPIO 25 → Door control servo (0°=closed, 180°=open)
```

---

## ✅ Checklist de Vérification

- [x] Relay.cpp timing corrigé
- [x] RFID.cpp dual-reader init amélioré
- [x] Motion.cpp debounce vérifié
- [x] Logic.cpp synchronisation confirmée
- [x] Button.cpp INPUT_PULLUP vérifié
- [x] Main.cpp boucle complète intégrée
- [x] Compilation 0 erreurs/warnings
- [x] Tous les includes présents
- [x] Structure SystemStatus cohérente
- [x] Sauvegarde EEPROM intégrée

---

## 🧪 Tests à Effectuer (Par Priorité)

### Test 1: Relay (FIRST - blocker pour tout le reste)

```bash
serial → "test relay"
```

**Attendu:** Les 8 relais s'allument/éteignent séquentiellement  
**Si échoue:** Vérifiez timing HC595 avec oscilloscope

### Test 2: RFID Reader 0 (ENTRY)

Scannez une carte.  
**Attendu:** Message `[RFID_ENTRY] Card: XXXXXXXX`

### Test 3: RFID Reader 1 (EXIT)

Scannez la même carte à l'autre lecteur.  
**Attendu:** Message `[RFID_EXIT] Card: XXXXXXXX`

### Test 4: Motion Sensor

```bash
serial → "test motion"
```

Bougez près du capteur.  
**Attendu:** Barre de "Consecutive reads" augmente jusqu'à 5, puis "MOTION EVENT DETECTED!"

### Test 5: Buttons

Long press MODE (800ms) → Mode passe à REGISTRATION (LED rouge clignote)  
Short press LAMP → Lampe intérieure toggle (ON/OFF)

### Test 6: Intrusion Alarm (Complet)

1. Force system en NIGHT mode
2. Scan RFID ENTRY (absence confirmée)
3. Bougez capteur mouvement
4. **Attendu:** 3 bips + LED rouge ON, puis silence pendant 5s, puis recommence
5. Scan RFID ENTRY
6. **Attendu:** Alarme arrête immédiatement, LED rouge OFF

---

## 📊 Architecture Synchronisation

```
BOUCLE PRINCIPALE:
┌─ Lecture énergie (chaque 2s)
├─ Lecture LDR (chaque 1s) → daynight status
├─ UPDATE MOTION SENSOR (chaque cycle)
│  └─ Débounce progression
├─ SCAN RFID ENTRY (chaque cycle)
│  └─ Si trouvé: handleRFIDScan(ENTRY)
│     ├─ Incrémente counters
│     ├─ Désactive intrusion_detected
│     ├─ Ouvre servo
│     └─ updateLightingLogic()
├─ SCAN RFID EXIT (chaque cycle)
│  └─ Si trouvé: handleRFIDScan(EXIT)
├─ UPDATE BUTTONS (chaque cycle)
│  ├─ MODE long press → toggle registration mode
│  └─ LAMP short press → toggle inside lamp
├─ Motion detected + night + absence
│  └─ Déclenche intrusion_detected = true
├─ UPDATE LCD (chaque 500ms)
└─ updateSystemState()
   └─ Si intrusion_detected: beep + LED rouge (avec cooldown 5s)
```

---

## 🎓 Concepts Clés Intégrés

### Debounce Motor Sensor

```
Pin = HIGH → Counter++ (jusqu'à 5)
Pin = LOW  → Counter = 0 (réinitialise)
Counter == 5 → Confirmation, reset counter
Cooldown 2s → Évite spam
```

### Relay State Structure

```cpp
struct RelayOutputs {
    bool q0_lamp_inside;   // GPIO mapping via 74HC595 Q0
    bool q1_lamp_outside;  // GPIO mapping via 74HC595 Q1
    bool q2_prise1;
    bool q3_prise2;
    bool q4_fan;
    bool q5_buzzer;        // Utilisé pour alarme + notifications
    bool q6_led_red;       // Intrusion alarm + denied access
    bool q7_led_green;     // Access granted notification
};
```

### RFID Access Logic

```
Scan ENTRY + Access Granted
  → Incrémente entries_count
  → Sets card.isInside = true
  → Calcule presence_count = entries - exits
  → Désactive intrusion_detected (force)
  → Ouvre servo

Scan EXIT + Access Granted
  → Incrémente exits_count
  → Sets card.isInside = false
  → Recalcule presence_count
```

---

## 🔐 Sauvegarde Persistante

Chaque changement d'état important est sauvegardé en EEPROM:

- Registre relay (bit mask 0x00-0xFF)
- Compteurs RFID (entries, exits, presence)
- Mode système (ACCESS vs REGISTRATION)
- État intrusion
- Base de cartes RFID

**Restauration au démarrage:** État antérieur chargé automatiquement

---

## 📝 Logs Clés à Surveiller

```
[INIT] Relay Controller (74HC595)...     → Init de base
[RELAY] 74HC595 initialized              → Init complète
[RFID] Reader 0 initialized (SS:5)       → Reader ENTRY OK
[RFID] Reader 1 initialized (SS:17)      → Reader EXIT OK
[MOTION] PIR sensor initialized on pin 12
[BUTTON] Manager initialized on pins: MODE=4 LAMP=15

[RFID_ENTRY] Card: XXXXXXXX             → Scan détecté
✓ ACCESS GRANTED                          → Carte valide
[MOTION] MOTION DETECTED AT NIGHT         → Alarme déclenche
```

---

## ⚡ Points Critiques de Performance

1. **Relay Timing:** 20-10µs minimum, latch pulse obligatoire
2. **RFID Init:** 50ms entre delay GPIO setup, 100ms après init, 500ms final
3. **Motion Debounce:** 5 lectures à ~10-20ms chacune = ~50-100ms délai
4. **Alarme Cooldown:** max une alerte toutes les 5000ms
5. **Servo Duration:** 2000ms (2 secondes) portes ouvertes

---

## 🚀 Prochaines Étapes

1. **Téléchargez le code** via PlatformIO
2. **Testez Relay d'abord** (commande `test relay`)
3. **Puis testez RFID** (chaque lecteur individuellement)
4. **Puis testez Motion** (commande `test motion`)
5. **Testez Buttons** (MODE + LAMP)
6. **Test complet:** Simulez intrusion (nuit+absence+motion) et RFID unlock

---

## 📞 Si Ça Ne Fonctionne Pas

**Relay ne répond pas?**  
→ Vérifiez timing avec `test relay`, puis oscilloscope si besoin

**RFID ne scan pas?**  
→ Vérifie pins SPI (18/19/23), RFID RST pin (16), lecteur SS pins (5/17)

**Motion faux positif?**  
→ Augmentez REQUIRED_CONSECUTIVE_READS dans motion.h (de 5 à 10)

**Buttons ne marche pas?**  
→ Vérifiez INPUT_PULLUP en cours, augmentez debounce si besoin

**Alarme ne s'arrête pas?**  
→ Vérifiez que RFID ENTRY scans correctement et desactive intrusion_detected

---

**Code Status: PRODUIT FINAL - PRÊT POUR TEST MATÉRIEL**

Tous les fichiers sont cohérents, compilent sans erreur, et les systèmes sont entièrement intégrés. Le code est prêt pour le débogage matériel étape par étape suivant le guide fourni.
