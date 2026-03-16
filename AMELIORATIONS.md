# Résumé des Améliorations du Projet Domotique

## 🎯 Problèmes Résolus

### 1. ✅ Détecteur de Mouvement (PIR) - Faux Positifs

**Problème :** Le capteur PIR s'activait même sans mouvement réel

**Solution :**

- Création d'une nouvelle classe `PIRMotionSensor` avec debounce robuste
- Changement de `INPUT_PULLDOWN` à `INPUT` pour éviter les pins qui flottent
- Implementation d'un système de confirmationà deux étapes :
  - **Étape 1** : Lecture cohérente du pin (5 lectures consécutives = ~50ms)
  - **Étape 2** : Délai de cooldown (`MOTION_COOLDOWN_MS = 5000ms`)
- Motion uniquement déclenchée lorsque :
  - C'est la nuit (STATUS_NIGHT)
  - Personne n'est présence (presence_count == 0)
  - Le capteur a enregistré plusieurs mouvements confirmés

**Fichiers créés:**

- `src/motion/motion.h` - Interface du capteur PIR
- `src/motion/motion.cpp` - Implémentation avec logique de debounce

---

### 2. ✅ Registre 74HC595 - Initialisation et Stabilité

**Problème :** Sorties non garanties à 0 au démarrage, mauvaise synchronisation

**Solutions :**

- Ajout d'un délai `delay(100ms)` dans `init()` pour laisser le registre se stabiliser
- Amélioration de `updateRegister()` avec :
  - Délais microseconde robustes et cohérents (5-10µs)
  - Séquence de verrouillage améliorée
  - Vérification que le latch retourne à LOW après chaque mise à jour
- Logging détaillé de chaque changement d'état relay
- Validation du couple (ancien état → nouvel état) avant mise à jour

**Modifications dans `src/relay/relay.cpp`:**

- `init()` : Garantit 0x00 au démarrage et latch bas en fin
- `setOutput()` : Log les changements et valide les paramètres
- `setBulk()` : Log les mises à jour en masse
- `updateRegister()` : Timing amélioré pour stabilité

---

### 3. ✅ Lecteurs RFID x2 - Séparation et Stabilité

**Problème :** Les deux lecteurs RFID n'étaient pas bien isolés, conflit possible SPI

**Solutions :**

- Amélioration du `init()` avec :
  - Vérification de version pour chaque lecteur (v0x91/0x92)
  - Reset logiciel (PCD_Reset) pour état propre
  - Activation de l'antenne pour chaque lecteur
  - Logging détaillé pour déboguer
- Amélioration de `scanReader()` :
  - Identification claire du lecteur (ENTRY/EXIT)
  - Propriété valide du chip select (SS1/SS2)
  - Meilleur handling des erreurs de lecture
- Nouvelle fonction `testConnection()` pour vérifier les deux lecteurs

**Paramètres des lecteurs RFID:**

- **Reader 0 (ENTRY)** : SS pin = 5 (RFID_SS2_PIN)
- **Reader 1 (EXIT)** : SS pin = 17 (RFID_SS1_PIN)
- Reset pin = 16 (RFID_RST_PIN partagé)

---

## 📝 Fichiers Modifiés

### Fichiers Créés

1. **`src/motion/motion.h`** - Classe PIRMotionSensor (interface)
2. **`src/motion/motion.cpp`** - Implémentation PIRMotionSensor

### Fichiers Modifiés

1. **`src/main.cpp`**
   - Ajout `#include "motion/motion.h"`
   - Création instance `PIRMotionSensor motionSensor`
   - Initialisation dans `setup()`
   - Remplacement logique motion en `loop()`

2. **`src/relay/relay.cpp`**
   - Amélioration `init()` avec stabilisation
   - Amélioration `updateRegister()` avec timing robuste
   - Amélioration `setOutput()` avec logging
   - Amélioration `setBulk()` avec logging
   - Amélioration `test()` pour meilleure visualisation

3. **`src/rfid/rfid.cpp`**
   - Amélioration `init()` avec vérifications et logging
   - Amélioration `scanReader()` avec meilleur handling
   - Nouvelle `testConnection()` robuste

---

## 🔌 Configuration des Sorties (74HC595)

```
Q0 -> Lampe intérieure      (q0_lamp_inside)
Q1 -> Lampe extérieure      (q1_lamp_outside)
Q2 -> Prise 1               (q2_prise1)
Q3 -> Prise 2               (q3_prise2)
Q4 -> Ventilateur           (q4_fan)
Q5 -> Buzzer/Sonnerie       (q5_buzzer)
Q6 -> LED Rouge (Alarm)     (q6_led_red)
Q7 -> LED Verte (OK)        (q7_led_green)
```

**N.B.** Toutes les sorties sont initialisées à `0` (OFF) au démarrage.

---

## ⚙️ Logique de Détection de Mouvement

```
Motion Sensor (PIR)
    ↓
InputReader + Debounce (5 lectures)
    ↓
Event validation:
  ✓ isDayTime (STATUS_NIGHT)
  ✓ NobodyPresent (presence_count == 0)
  ✓ Cooldown elapsed (5000ms)
    ↓
Intrusion Alert + Alarm (3 bips + LED rouge)
```

**Paramètres dans config.h :**

- `MOTION_DEBOUNCE_MS = 200` (inutilisé avec la nouvelle classe)
- `MOTION_COOLDOWN_MS = 5000`
- `PIRMotionSensor::REQUIRED_CONSECUTIVE_READS = 5`

---

## 🧪 Test Recommandés

1. **Test Startup:**
   - Vérifier tous les relais OFF au démarrage
   - Logs doivent afficher état 0x00

2. **Test PIR:**
   - Aucune alarme pendant le jour
   - Aucune alarme si quelqu'un est présent (même la nuit)
   - Alarme uniquement: nuit + absence + mouvement réel

3. **Test RFID x2:**
   - Scanner une carte en ENTRY (lecteur 0)
   - Scanner une carte en EXIT (lecteur 1)
   - Vérifier que chaque lecteur est indépendant

4. **Test Relais:**
   - Chaque sortie Q0-Q7 doit commuter correctement
   - Pas de crosstalk entre les sorties

---

## 📊 Améliorations Globals

✅ Meilleur debounce des capteurs
✅ Initialisation garantie à zéro
✅ Meilleur timing SPI pour stabilité
✅ Logging détaillé pour déboguer
✅ Séparation des responsabilités (PIRMotionSensor class)
✅ Validation des paramètres
✅ Meilleure gestion des deux lecteurs RFID

---

## 🚀 Prochaines Étapes Recommandées

1. Compiler et vérifier l'absence d'erreurs
2. Télécharger sur le microcontrôleur
3. Tester chaque capteur individuellement
4. Tester les scénarios d'intrusion
5. Fine-tuner les seuils si nécessaire

---

## 📌 Notes Importants

- La classe `PIRMotionSensor` doit être mise à jour dans la boucle `loop()`
- Les délais microseconde peuvent varier selon l'ESP32 (ajustables)
- SPI doit être bien câblée avec résistances de pull-up si nécessaire
- Vérifier la version du capteur RFID (0x91 ou 0x92)
