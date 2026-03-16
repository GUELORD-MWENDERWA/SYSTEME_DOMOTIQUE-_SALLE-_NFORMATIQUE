# ✅ Checklist de Validation des Changements

## 📂 Fichiers Créés (À Vérifier)

- [ ] `src/motion/motion.h` - Interface PIRMotionSensor
- [ ] `src/motion/motion.cpp` - Implémentation PIRMotionSensor

## 📝 Fichiers Modifiés (À Compiler)

- [ ] `src/main.cpp` - Include motion.h + intégration
- [ ] `src/relay/relay.cpp` - Amélioration initialisation et timing
- [ ] `src/rfid/rfid.cpp` - Amélioration gestion dual RFID

## 🔧 Compilation

- [ ] Lancer `pio run` - Vérifier 0 erreurs
- [ ] Lancer `pio run --target upload` - Télécharger sur ESP32

## 🧪 Tests Matériel (Post-Upload)

### Test Démarrage

- [ ] LCD affiche "Système OK"
- [ ] Tous les relais OFF (0x00)
- [ ] Serial monitor montre tous les INIT OK

### Test Relais 74HC595

- [ ] Chaque sortie Q0-Q7 s'active/désactive correctement
- [ ] Pas de crosstalk entre sorties
- [ ] LED verte flash au démarrage (Q7)

### Test Détecteur PIR

- [ ] AUCUNE alarme pendant le jour (même avec mouvement)
- [ ] AUCUNE alarme si quelqu'un est présent (même la nuit)
- [ ] Alarme correcte: nuit + absence + mouvement réel
- [ ] Délai de 5s entre deux alarmes (cooldown)

### Test Lecteurs RFID x2

- [ ] Lecteur 1 (ENTRY, SS2=pin5) détecte cartes
- [ ] Lecteur 2 (EXIT, SS1=pin17) détecte cartes
- [ ] Compteurs entry/exit augmentent correctement
- [ ] Log affiche "ENTRY" ou "EXIT" clairement

### Test Lampes Automatiques

- [ ] Lampe intérieure ON quand quelqu'un présent
- [ ] Lampe extérieure ON que la nuit et personne absent

### Test Mode Nuit/Jour

- [ ] Transition jour→nuit automatique (LDR)
- [ ] Pas de faux positif de motion la journée
- [ ] LED rouge s'allume en intrusion alert

## 🎯 Critères de Succès

- ✅ Tous les relais à 0 au démarrage
- ✅ Aucun faux positif du PIR pendant le jour
- ✅ Aucun faux positif du PIR quand il y a du monde
- ✅ Deux lecteurs RFID fonctionnels et indépendants
- ✅ Alarme intrusion correcte (nuit + absent + mouvement)
- ✅ Zéro erreur de compilation
