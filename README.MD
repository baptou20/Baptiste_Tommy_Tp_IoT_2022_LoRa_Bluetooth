# Tp_IoT_2022_LoRa_Bluetooth

Ce TP est à réaliser à deux groupes.

## 1. Mise en place

### 1. Installer l'IDE VScode et les dépendances nécessaires

cf. [https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/get-started/index.html](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/get-started/index.html)

Documentation de la board: [http://www.smartcomputerlab.org/](http://www.smartcomputerlab.org/)

### 2. Se connecter sur un point d'accès Wifi

### 3. Envoyer un message mqtt sur le broker test.mosquitto.org au topic tp/alban

## 2. LoRa

### 1. Définir des valeurs communes (à faire au tableau)

 #define MQTT_TOPIC "kbssa"
 #define MQTT_MESSAGE "kbssa123"

### 2. Communiquer via mqtt des données

Groupe 1: Envoyer un message mqtt donnant les valeurs nécessaires à une reception via LoRa.

Groupe 2. Ecouter les messages mqtt en en déduire les valeurs pour un envoi de données via Lora.

I (5921) baptiste et tommy: Message envoyé avec succès, msg_id=50214
I (6111) mqtt5_client: MQTT_MSG_TYPE_PUBACK return code is 0
I (6121) baptiste et tommy: MQTT_EVENT_PUBLISHED, msg_id=50214
I (7581) example_connect: Got IPv6 event: Interface "example_netif_sta" address: 2a02:8440:715b:a386:ce50:e3ff:fe99:2d50, type: ESP_IP6_ADDR_IS_GLOBAL

### 3. Communiquer via LoRa des données

Groupe 1: Écouter les messages LoRa et les afficher.

Groupe 2. Envoyer un message LoRa contenant des données (potentiellement en utilisant un caoteur.

## 3. Inversion

Après avoir réussi une communication, la carte du groupe 1 et celle du groupe 2 inversent leurs rôles.

## 4. Bluetooth (ou BLE)

À la place de communiquer les valeurs venant d'un capteur, continuer la discussion pour ouvrir un canal Bluetooth.


