#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lora.h"

#define WIFI_SSID "iPhone de Baptiste"
#define WIFI_PASS "baptou20"
#define MQTT_BROKER_URI "mqtt://test.mosquitto.org"
#define MQTT_TOPIC "kbssa"
#define LORA_PASSWORD "kbssa123"

static const char *TAG = "LoRa_MQTT";
static esp_mqtt_client_handle_t mqtt_client = NULL;

// 📡 Connexion au WiFi
esp_err_t wifi_init_sta(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
    return ESP_OK;
}

// 📡 Gestion MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Connecté");
        esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC, 1);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT Reçu: %.*s", event->data_len, event->data);
        
        // 🔐 Ajout du mot de passe avant d'envoyer sur LoRa
        char lora_msg[256];
        snprintf(lora_msg, sizeof(lora_msg), "[%s] %.*s", LORA_PASSWORD, event->data_len, event->data);
        
        // 📡 Envoi sur LoRa
        lora_send_packet((uint8_t *)lora_msg, strlen(lora_msg));
        ESP_LOGI(TAG, "LORA Envoyé: %s", lora_msg);
        break;

    default:
        break;
    }
}

// 📡 Démarrage MQTT
static void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

// 📡 Écoute en LoRa et renvoi en MQTT
void task_rx(void *pvParameters) {
    ESP_LOGI(TAG, "LORA Écoute démarrée");
    uint8_t buf[256];  
    while(1) {
        lora_receive(); 
        if (lora_received()) {
            int rxLen = lora_receive_packet(buf, sizeof(buf));
            buf[rxLen] = '\0';
            ESP_LOGI(TAG, "LORA Reçu: %s", buf);
            
            // 🔐 Vérification du mot de passe
            if (strncmp((char *)buf, "[" LORA_PASSWORD "]", strlen(LORA_PASSWORD) + 2) == 0) {
                char *message = (char *)buf + strlen(LORA_PASSWORD) + 3;
                ESP_LOGI(TAG, "LORA Authentifié: %s", message);
                
                // 📡 Publier sur MQTT
                esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, message, 0, 1, 0);
                ESP_LOGI(TAG, "MQTT Envoyé: %s", message);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// 📡 Fonction principale
void app_main() {
    ESP_ERROR_CHECK(wifi_init_sta());  
    mqtt_app_start();

    if (lora_init() == 0) {
        ESP_LOGE(TAG, "LORA Erreur: Module non reconnu !");
        while(1) { vTaskDelay(1); }
    }

    // 🎯 Configuration LoRa
    lora_set_frequency(866e6);
    lora_enable_crc();
    lora_set_coding_rate(1);
    lora_set_bandwidth(7);
    lora_set_spreading_factor(7);

    // 📡 Lancement de la tâche LoRa
    xTaskCreate(&task_rx, "RX", 1024*3, NULL, 5, NULL);
}