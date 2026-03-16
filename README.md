# 🏢 SYSTÈME DOMOTIQUE - SALLE D'INFORMATIQUE

Système complet de domotique pour salle d'informatique basé sur ESP32 avec contrôle d'accès RFID, monitoring énergétique, éclairage intelligent et sécurité.

**Plateforme:** PlatformIO avec Arduino Framework  
**Microcontrôleur:** MHE ESP32 DevKit  
**Langage:** C++17

---

## 📋 Table des matières

1. [Caractéristiques](#-caractéristiques)
2. [Architecture Matérielle](#-architecture-matérielle)
3. [Installation](#-installation)
4. [Commandes Série](#-commandes-série)
5. [Scénarios](#-scénarios)
6. [Dépannage](#-dépannage)

---

## ✨ Caractéristiques

### ✅ Fonctionnalités principales

- **Monitoring énergétique** : Lecture temps réel PZEM-004T (tension, courant, puissance, énergie)
- **Contrôle d'accès RFID** : 2 lecteurs RC522 (entrée/sortie) avec gestion des badges
- **Éclairage intelligent** : Lampes intérieure/extérieure avec logique jour/nuit
- **Sécurité** : Détection d'intrusion (nuit + absence)
- **Servo moteur** : Ouverture/fermeture portail (0° - 180°)
- **Relais contrôlés** : 74HC595 (8 sorties : lampes, prises, ventilo, buzzeur, LEDs)
- **Affichage LCD** : Écran 16x2 avec diagnostic temps réel
- **Interface série** : Console de commande complète (115200 bps)
- **Stockage EEPROM** : Persistence des données critiques

---

## 🔌 Architecture Matérielle

### Microcontrôleur

```
ESP32 MHE DevKit v1
├─ CPU: Xtensa Dual-Core 32-bit @ 240 MHz
├─ RAM: 520 KB SRAM
├─ Flash: 4 MB
└─ Tension: 3.3V / 5V USB
```

### Connexions Principales

```
PZEM-004T (Énergie)
├─ RX: GPIO 32
├─ TX: GPIO 33
└─ Baudrate: 9600 bps (UART2)

RFID RC522 x2 (SPI)
├─ SS1 (Entrée): GPIO 5
├─ SS2 (Sortie): GPIO 17
├─ CLK: GPIO 18
├─ MISO: GPIO 19
├─ MOSI: GPIO 23
└─ Frequency: 1 MHz

74HC595 Registre à Décalage
├─ LATCH: GPIO 26
├─ CLOCK: GPIO 14
└─ DATA: GPIO 27

Servo SG90
└─ PWM: GPIO 25

LCD I2C 16x2
├─ SDA: GPIO 21
├─ SCL: GPIO 22
└─ Address: 0x27

Boutons
├─ MODE: GPIO 4 (appui long)
└─ LAMPE: GPIO 15 (appui court)

LDR (Luminosité)
└─ ADC: GPIO 35
```

### Sorties Relais 74HC595

| Bit | Q   | Fonction         | Tension | Courant |
| --- | --- | ---------------- | ------- | ------- |
| 0   | Q0  | Lampe intérieure | 5V/12V  | 10A     |
| 1   | Q1  | Lampe extérieure | 5V/12V  | 10A     |
| 2   | Q2  | Prise 1          | 5V/12V  | 10A     |
| 3   | Q3  | Prise 2          | 5V/12V  | 10A     |
| 4   | Q4  | Ventilateur      | 5V/12V  | 10A     |
| 5   | Q5  | Buzzeur          | 5V      | 100mA   |
| 6   | Q6  | LED Rouge        | 5V      | 20mA    |
| 7   | Q7  | LED Verte        | 5V      | 20mA    |

---

## 🚀 Installation

### 1. Prérequis

```bash
# Installer PlatformIO
pip install platformio

# OU via IDE VS Code (extension PlatformIO)
```

### 2. Cloner/Créer le projet

```bash
mkdir domotique_salle_info
cd domotique_salle_info
platformio init -d . -b mhetesp32devkit
```

### 3. Copier les fichiers

- Copier `platformio.ini` à la racine
- Créer structure `src/` avec tous les fichiers `.h` et `.cpp`

### 4. Compiler et uploader

```bash
# Compiler
platformio run

# Uploader sur ESP32
platformio run -t upload

# Monitorer la sortie série
platformio device monitor --baud 115200
```

---

## 📡 Commandes Série

**Format:** `COMMANDE [PARAMÈTRE]`  
**Baudrate:** 115200 bps

### 📊 Commandes ÉNERGIE

```
E
  → Affiche mesures PZEM actuelles (V, A, W, kWh)

ER
  → Rapport énergétique détaillé
```

### 🔑 Commandes RFID

```
RLIST
  → Liste toutes les cartes autorisées

RMODE
  → Affiche mode actuel (ACCESS/REGISTRATION)

RREG
  → Passe en mode enregistrement

RACC
  → Passe en mode accès
```

### 💡 Commandes RELAIS

```
L0 1/0       → Lampe intérieure (1=ON, 0=OFF)
L1 1/0       → Lampe extérieure (1=ON, 0=OFF)
P1 1/0       → Prise 1 (1=ON, 0=OFF)
P2 1/0       → Prise 2 (1=ON, 0=OFF)
F 1/0        → Ventilateur (1=ON, 0=OFF)
```

### 🔧 Commandes DIAGNOSTIC

```
D
  → État système complet
  → Affiche: mode, jour/nuit, présence, LDR, intrusion

T
  → Test auto tous composants
  → Vérifie: PZEM, RFID, Relais, Servo, LDR, LCD

C
  → Affiche compteurs présence (Entrées/Sorties)
```

### ℹ️ Commandes SYSTÈME

```
INFO
  → Infos système (version, uptime, etc.)

HELP
  → Affiche ce menu

REBOOT
  → Redémarrage ESP32
```

---

## 📍 Scénarios

### Scénario 1️⃣ - Jour - Quelqu'un à l'entrée

```
1. Badge RFID scanné à l'entrée
   ↓
2. Vérification base de données
   ├─ ✓ Autorisé:
   │  ├─ Servo: 0° → 180° (porte ouvre)
   │  ├─ LED verte: ON
   │  ├─ Buzzeur: 1 bip court
   │  └─ Après 3s: Servo → 0° (porte ferme)
   │
   └─ ✗ Refusé:
      ├─ Servo: reste fermé
      ├─ LED rouge: ON 3s
      └─ Buzzeur: 2 bips courts

3. Compteur présence augmente
   ↓
4. Lampe intérieure s'allume
```

### Scénario 2️⃣ - Jour - Fin de journée (sortie)

```
1. Badge RFID scanné à la sortie
   ↓
2. Compteur présence diminue
   ↓
3. SI Compteur = 0 (salle vide):
   ├─ Lampe intérieure: OFF
   └─ Servo: reste fermé
```

### Scénario 3️⃣ - Nuit - Absence + Intrusion?

```
1. LDR détecte nuit (obscurité)
   ↓
2. Compteur présence = 0 (salle vide)
   ↓
3. 🚨 ALERTE INTRUSION!
   ├─ Lampe extérieure: ON
   ├─ Lampe intérieure: OFF
   ├─ LED rouge: ON continu
   ├─ Buzzeur: 3 bips longs répétés
   └─ Serial: Log détaillé

Résolution:
└─ Commande manuelle pour reset (nécessite admin)
```

### Scénario 4️⃣ - Mode Enregistrement Carte

```
1. Appui long (>2s) bouton GPIO4
   ↓
2. LED rouge clignote (indication enregistrement)
   ↓
3. Scannez une nouvelle carte
   ↓
4. Carte enregistrée automatiquement
   ├─ UID stocké en EEPROM
   └─ LED verte clignote (confirmation)

5. Revenir à mode accès
```

---

## 📁 Structure du Projet

```
domotique_salle_info/
├── platformio.ini              # Configuration PlatformIO
├── src/
│   ├── main.cpp               # Point d'entrée principal
│   ├── config.h               # Configuration centralisée
│   │
│   ├── energy/
│   │   ├── energy.h           # Interface PZEM
│   │   └── energy.cpp         # Implémentation
│   │
│   ├── rfid/
│   │   ├── rfid.h             # Interface RFID
│   │   └── rfid.cpp           # Implémentation
│   │
│   ├── relay/
│   │   ├── relay.h            # Interface 74HC595
│   │   └── relay.cpp          # Implémentation
│   │
│   ├── servo/
│   │   ├── servo.h            # Interface Servo
│   │   └── servo.cpp          # Implémentation
│   │
│   ├── button/
│   │   ├── button.h           # Interface Boutons
│   │   └── button.cpp         # Implémentation
│   │
│   ├── ldr/
│   │   ├── ldr.h              # Interface LDR
│   │   └── ldr.cpp            # Implémentation
│   │
│   ├── lcd/
│   │   ├── lcd.h              # Interface LCD I2C
│   │   └── lcd.cpp            # Implémentation
│   │
│   ├── serial_cmd/
│   │   ├── serial_cmd.h       # Interface Commandes
│   │   └── serial_cmd.cpp     # Parser + Handlers
│   │
│   ├── storage/
│   │   ├── storage.h          # Interface EEPROM
│   │   └── storage.cpp        # Gestion stockage
│   │
│   └── logic/
│       ├── logic.h            # Interface Logique
│       └── logic.cpp          # Orchestration système
│
├── lib/                       # Librairies (auto-générées)
├── .gitignore
├── README.md
└── .pio/                     # Build (auto-généré)
```

---

## 🔧 Dépannage

### ❌ Erreur: "PZEM ne répond pas"

```
Vérifier:
✓ Pins RX (GPIO32) / TX (GPIO33) corrects
✓ Baudrate 9600 configuré
✓ Connexion GND commune
✓ Câblage inversé? (RX ↔ TX)
```

### ❌ Erreur: "RFID ne scan pas"

```
Vérifier:
✓ Pins SPI corrects (CLK, MISO, MOSI)
✓ SS pins (GPIO5 et GPIO17) distincts
✓ Fréquence SPI = 1 MHz
✓ Cartes RC522 compatibles
```

### ❌ Erreur: "LCD ne s'affiche pas"

```
Vérifier:
✓ Adresse I2C: 0x27 ou 0x3F (tester les deux)
✓ Pins SDA (GPIO21) / SCL (GPIO22)
✓ Contrast potentiometer du LCD
✓ Backlight power (5V)
```

### ❌ Erreur: "Relais ne basculent pas"

```
Vérifier:
✓ Pins 74HC595: LATCH(26), CLOCK(14), DATA(27)
✓ Tension 5V sur module relais
✓ GND commun ESP32/Relais
✓ Type 74HC595 authentique (pas 74LS595)
```

### ⚠️ Avertissement: "Compilation lente"

```
Solution:
platformio run --target clean
platformio run
```

---

## 📊 Limitations & Considérations

| Aspect        | Limitation            | Note                       |
| ------------- | --------------------- | -------------------------- |
| Cartes RFID   | 50 max en EEPROM      | Extensible en NVS          |
| Historique    | 100 logs max          | Circularaire (overwrite)   |
| Affichage LCD | 2 lignes 16 chars     | Rafraîchissement 1s        |
| Servo         | 2 positions (0°/180°) | Facilement paramétrable    |
| PZEM          | Max 100A              | Version adaptée: 200A/300A |
| LDR           | Calibration manuelle  | Seuil: ~1500 ADC           |

---

## 🚀 Évolutions Futures

- [ ] WiFi & Web Dashboard
- [ ] Application mobile
- [ ] Cloud synchronisation
- [ ] Capteurs T°/Humidité
- [ ] Détection fumée/CO2
- [ ] Caméra IP
- [ ] Machine Learning détection anomalies

---

## 📝 Licence

MIT License - Libre d'utilisation et de modification

---

## 👤 Auteur

Projet domotique - 2024-2025

**Version:** 1.0.0  
**Dernière mise à jour:** 2024-01-15

---

## 📞 Support

Pour problèmes ou questions:

1. Vérifier la section [Dépannage](#-dépannage)
2. Consulter les logs série (115200 bps)
3. Exécuter la commande `T` (auto-test)

---

## 📚 Références Utiles

- [Fiche PZEM-004T](https://github.com/mandulaj/PZEM-004T-v30)
- [Datasheet RC522](https://datasheetspdf.com/pdf/MFRC522)
- [ESP32 Documentation](https://docs.espressif.com/)
- [PlatformIO Guide](https://docs.platformio.org/)
