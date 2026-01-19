# Smart Node with Local Alarm and Web View (Project 1)

Ce projet implémente un nœud IoT basé sur Arduino pour la surveillance des vibrations et de la température. [cite_start]Le système utilise une approche "Gateway" : l'Arduino gère les capteurs et les alarmes locales, tandis qu'un serveur Python FastAPI expose les données sur une interface Web interactive[cite: 31, 32].

## 📋 Fonctionnalités

* **Surveillance de l'environnement** : Lecture de la température et de l'humidité via le capteur DHT20 toutes les 2 secondes
* **Détection de chocs** : Analyse de l'accélération (IMU) à une fréquence de 10 Hz (100ms) pour identifier les événements de choc
* **Logique d'alarme locale** :
    * L'alarme se déclenche si la température dépasse 30°C ou si plus de 10 chocs sont détectés
    * Indication visuelle (LED clignotante) et sonore (buzzer intermittent) via une machine à états non-bloquante
* **Contrôle par bouton** : Un appui court sur le bouton physique permet d'acquitter l'alarme et de réinitialiser le compteur de chocs
* **Interface Web** : Affichage en temps réel des mesures (T, H, chocs, état de l'alarme) et commandes à distance (START/STOP)

## 🛠️ Architecture Technique

### Arduino (Nœud local)
* **Capteurs** : IMU (LSM9DS1) et DHT20
* **Actuateurs** : LED (Pin 13) et Buzzer (Pin 2)
* **Communication** : Envoi de données structurées en série au format `key=value` (ex: `t=..., T=..., H=..., shock_count=..., alarm=...`)

### Python Gateway (FastAPI)
* **Background Reader** : Un thread lit en continu le port série pour garder les 100 dernières mesures en mémoire vive (deque).
* **API REST** :
    * `GET /data` : Renvoie les données récentes au format JSON pour le frontend
    * `POST /start` & `POST /stop` : Envoie les commandes correspondantes vers l'Arduino via le port série

### Frontend (HTML/JS)
* Interface statique utilisant `fetch` pour rafraîchir les données toutes les 500ms sans recharger la page.

## 🚀 Installation

### 1. Configuration Arduino
1. Installez les bibliothèques `Arduino_LSM9DS1` et `Arduino_Lib_DHT20` via le Library Manager.
2. Téléversez le fichier `IMU_Reading.ino` sur votre carte (Arduino Nano 33 BLE Sense recommandée).

### 2. Configuration Python
1. Installez les dépendances nécessaires :
   ```bash
   pip install fastapi uvicorn pyserial
