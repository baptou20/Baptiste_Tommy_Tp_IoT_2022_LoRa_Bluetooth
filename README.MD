# Tp_IoT_2022_LoRa_Bluetooth Tommy et Baptiste.

**Compte-rendu : Communication ESP32 avec MQTT et LoRa pour un capteur de température**

1. Introduction**
Ce projet met en place un système de communication entre 2 cartes ESP32 dont une équipée d"un capteur. L'objectif est de les faire communiquer via le protocole MQTT et la technologie LoRa. 

2. Fonctionnement du programme**
Le programme se déroule en plusieurs étapes :

1. **Connexion au WiFi et au broker MQTT**
   - L'ESP32 se connecte à un réseau WiFi (iPhone de Baptiste).
   - Il établit une connexion avec le broker MQTT (test.mosquitto.org).
   - Il s'abonne à un topic spécifique pour recevoir et publier des messages. (kbsssa)

2. Echanges LoRa
   - L'ESP32 n° 1 prépare un message un mot de passe d'authentification envoyée via MQTT.
   - L'ESP32 n°2 renvoie le mot de passe via LoRa.

3. Réception et validation des données sur LoRa
   - L'ESP32 récepteur écoute les messages transmis par LoRa.
   - Il vérifie l'authenticité du message en comparant le mot de passe préfixé.
   - Si le message est valide, il affiche
Nous avons testé l'émission / réceptions dans les 2 sens.
4. Capteur
   - Avec la même procédure, mais on ajoute les données du capteur à la suite du mot de passe.
   - On affiches les données du capteurs juste après le mot de passe.
   - Une fois la température publiée sur MQTT, tout client abonné au topic peut récupérer les données.


