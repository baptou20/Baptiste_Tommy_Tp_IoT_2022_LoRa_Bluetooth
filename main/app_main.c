/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
 #include <inttypes.h>
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "esp_log.h"
 #include "lora.h"

 #define WIFI_SSID "iPhone de Baptiste"
 #define WIFI_PASS "baptou20"
 #define MQTT_BROKER_URI "mqtt://test.mosquitto.org"
 #define MQTT_TOPIC "kbssa"
 #define MQTT_MESSAGE "kbssa"
 
static const char *TAG = "baptiste et tommy";

#if CONFIG_SENDER
void task_tx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) {
		TickType_t nowTick = xTaskGetTickCount();
		int send_len = sprintf((char *)buf,"Je s'appelle Groot !! %"PRIu32, nowTick);
		lora_send_packet(buf, send_len);
		ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent...", send_len);
		int lost = lora_packet_lost();
		if (lost != 0) {
			ESP_LOGW(pcTaskGetName(NULL), "%d packets lost", lost);
		}
		vTaskDelay(pdMS_TO_TICKS(5000));
	} // end while
}
#endif // CONFIG_SENDER


 // Fonction de gestion des événements Wi-Fi
 static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
         ESP_LOGI(TAG, "Connecté à %s", WIFI_SSID);
     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
         ESP_LOGI(TAG, "Déconnecté du Wi-Fi");
     }
 }
 esp_err_t wifi_init_sta(void) {
     // Initialiser la NVS
     esp_err_t ret = nvs_flash_init();
     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
         ESP_ERROR_CHECK(nvs_flash_erase());
         ret = nvs_flash_init();
     }
     ESP_ERROR_CHECK(ret);
 
     // Initialisation du Wi-Fi
     esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
 
     // Correctement initialiser la configuration Wi-Fi
     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();  // Crée une configuration par défaut
     ESP_ERROR_CHECK(esp_wifi_init(&cfg));  // Passe la configuration à la fonction d'initialisation
 
     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
     ESP_ERROR_CHECK(esp_wifi_start());
 
     // Connexion au Wi-Fi
     wifi_config_t wifi_config = {
         .sta = {
             .ssid = WIFI_SSID,
             .password = WIFI_PASS,
         },
     };
     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
     ESP_ERROR_CHECK(esp_wifi_connect());
 
     ESP_LOGI(TAG, "Connexion au Wi-Fi en cours...");
     return ESP_OK;
 }
 
 
 static void log_error_if_nonzero(const char *message, int error_code)
 {
     if (error_code != 0) {
         ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
     }
 }
 
 static esp_mqtt5_user_property_item_t user_property_arr[] = {
         {"board", "esp32"},
         {"u", "user"},
         {"p", "password"}
     };
 
 #define USE_PROPERTY_ARR_SIZE   sizeof(user_property_arr)/sizeof(esp_mqtt5_user_property_item_t)
 
 static esp_mqtt5_publish_property_config_t publish_property = {
     .payload_format_indicator = 1,
     .message_expiry_interval = 1000,
     .topic_alias = 0,
     .response_topic = MQTT_TOPIC,
     .correlation_data = "123456",
     .correlation_data_len = 6,
 };
 
 static esp_mqtt5_subscribe_property_config_t subscribe_property = {
     .subscribe_id = 25555,
     .no_local_flag = false,
     .retain_as_published_flag = false,
     .retain_handle = 0,
     .is_share_subscribe = true,
     .share_name = "group1",
 };
 
 static esp_mqtt5_subscribe_property_config_t subscribe1_property = {
     .subscribe_id = 25555,
     .no_local_flag = true,
     .retain_as_published_flag = false,
     .retain_handle = 0,
 };
 
 static esp_mqtt5_unsubscribe_property_config_t unsubscribe_property = {
     .is_share_subscribe = true,
     .share_name = "group1",
 };
 
 static esp_mqtt5_disconnect_property_config_t disconnect_property = {
     .session_expiry_interval = 60,
     .disconnect_reason = 0,
 };
 
 static void print_user_property(mqtt5_user_property_handle_t user_property)
 {
     if (user_property) {
         uint8_t count = esp_mqtt5_client_get_user_property_count(user_property);
         if (count) {
             esp_mqtt5_user_property_item_t *item = malloc(count * sizeof(esp_mqtt5_user_property_item_t));
             if (esp_mqtt5_client_get_user_property(user_property, item, &count) == ESP_OK) {
                 for (int i = 0; i < count; i ++) {
                     esp_mqtt5_user_property_item_t *t = &item[i];
                     ESP_LOGI(TAG, "key is %s, value is %s", t->key, t->value);
                     free((char *)t->key);
                     free((char *)t->value);
                 }
             }
             free(item);
         }
     }
 }
 
 /*
  * @brief Event handler registered to receive MQTT events
  *
  *  This function is called by the MQTT client event loop.
  *
  * @param handler_args user data registered to the event.
  * @param base Event base for the handler(always MQTT Base in this example).
  * @param event_id The id for the received event.
  * @param event_data The data for the event, esp_mqtt_event_handle_t.
  */
 static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
 {
     ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
     esp_mqtt_event_handle_t event = event_data;
     esp_mqtt_client_handle_t client = event->client;
     int msg_id;
 
     ESP_LOGD(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
     switch ((esp_mqtt_event_id_t)event_id) {
     case MQTT_EVENT_CONNECTED:
         ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
         msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, MQTT_MESSAGE, 0, 1, 0);
         ESP_LOGI(TAG, "Message envoyé avec succès, msg_id=%d", msg_id);
         print_user_property(event->property->user_property);
         esp_mqtt5_client_set_user_property(&publish_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
         esp_mqtt5_client_set_publish_property(client, &publish_property);
         break;
     case MQTT_EVENT_DISCONNECTED:
         ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
         print_user_property(event->property->user_property);
         break;
     case MQTT_EVENT_SUBSCRIBED:
         ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
         print_user_property(event->property->user_property);
         esp_mqtt5_client_set_publish_property(client, &publish_property);
         msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
         ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
         break;
     case MQTT_EVENT_UNSUBSCRIBED:
         ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
         print_user_property(event->property->user_property);
         esp_mqtt5_client_set_user_property(&disconnect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
         esp_mqtt5_client_set_disconnect_property(client, &disconnect_property);
         esp_mqtt5_client_delete_user_property(disconnect_property.user_property);
         disconnect_property.user_property = NULL;
         esp_mqtt_client_disconnect(client);
         break;
     case MQTT_EVENT_PUBLISHED:
         ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
         print_user_property(event->property->user_property);
         break;
     case MQTT_EVENT_DATA:
         ESP_LOGI(TAG, "MQTT_EVENT_DATA");
         print_user_property(event->property->user_property);
         ESP_LOGI(TAG, "payload_format_indicator is %d", event->property->payload_format_indicator);
         ESP_LOGI(TAG, "response_topic is %.*s", event->property->response_topic_len, event->property->response_topic);
         ESP_LOGI(TAG, "correlation_data is %.*s", event->property->correlation_data_len, event->property->correlation_data);
         ESP_LOGI(TAG, "content_type is %.*s", event->property->content_type_len, event->property->content_type);
         ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
         ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
         break;
     case MQTT_EVENT_ERROR:
         ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
         print_user_property(event->property->user_property);
         ESP_LOGI(TAG, "MQTT5 return code is %d", event->error_handle->connect_return_code);
         if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
             log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
             log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
             log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
             ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
         }
         break;
     default:
         ESP_LOGI(TAG, "Other event id:%d", event->event_id);
         break;
     }
 }
 
 static void mqtt5_app_start(void)
 {
     esp_mqtt5_connection_property_config_t connect_property = {
         .session_expiry_interval = 10,
         .maximum_packet_size = 1024,
         .receive_maximum = 65535,
         .topic_alias_maximum = 2,
         .request_resp_info = true,
         .request_problem_info = true,
         .will_delay_interval = 10,
         .payload_format_indicator = true,
         .message_expiry_interval = 10,
         .response_topic = MQTT_TOPIC,
         .correlation_data = "123456",
         .correlation_data_len = 6,
     };
 
     esp_mqtt_client_config_t mqtt5_cfg = {
         .broker.address.uri = MQTT_BROKER_URI,
         .session.protocol_ver = MQTT_PROTOCOL_V_5,
         .network.disable_auto_reconnect = true,
         .session.last_will.topic = MQTT_TOPIC,
         .session.last_will.msg = MQTT_MESSAGE,
         .session.last_will.msg_len = 21,
         .session.last_will.qos = 1,
         .session.last_will.retain = true,
     };
 
 
     esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt5_cfg);
 
     /* Set connection properties and user properties */
     esp_mqtt5_client_set_user_property(&connect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
     esp_mqtt5_client_set_user_property(&connect_property.will_user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
     esp_mqtt5_client_set_connect_property(client, &connect_property);
 
     /* If you call esp_mqtt5_client_set_user_property to set user properties, DO NOT forget to delete them.
      * esp_mqtt5_client_set_connect_property will malloc buffer to store the user_property and you can delete it after
      */
     esp_mqtt5_client_delete_user_property(connect_property.user_property);
     esp_mqtt5_client_delete_user_property(connect_property.will_user_property);
 
     /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
     esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
     esp_mqtt_client_start(client);
 }
 
 void app_main(void)
 {
    if (lora_init() == 0) {
		ESP_LOGE(pcTaskGetName(NULL), "Does not recognize the module");
		while(1) {
			vTaskDelay(1);
		}
	}

	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 866MHz");
	lora_set_frequency(868e6); // 866MHz

	lora_enable_crc();

	int cr = 1;
	int bw = 7;
	int sf = 7;


	lora_set_coding_rate(cr);
	//lora_set_coding_rate(CONFIG_CODING_RATE);
	//cr = lora_get_coding_rate();
	ESP_LOGI(pcTaskGetName(NULL), "coding_rate=%d", cr);

	lora_set_bandwidth(bw);
	//lora_set_bandwidth(CONFIG_BANDWIDTH);
	//int bw = lora_get_bandwidth();
	ESP_LOGI(pcTaskGetName(NULL), "bandwidth=%d", bw);

	lora_set_spreading_factor(sf);
	//lora_set_spreading_factor(CONFIG_SF_RATE);
	//int sf = lora_get_spreading_factor();
	ESP_LOGI(pcTaskGetName(NULL), "spreading_factor=%d", sf);

#if CONFIG_SENDER
	xTaskCreate(&task_tx, "TX", 1024*3, NULL, 5, NULL);
#endif
    
     ESP_LOGI(TAG, "[APP] Startup..");
     ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
     ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
 
     esp_log_level_set("*", ESP_LOG_INFO);
     esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
     esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
     esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
     esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
     esp_log_level_set("transport", ESP_LOG_VERBOSE);
     esp_log_level_set("outbox", ESP_LOG_VERBOSE);
 
     ESP_ERROR_CHECK(nvs_flash_init());
     ESP_ERROR_CHECK(esp_netif_init());
     ESP_ERROR_CHECK(esp_event_loop_create_default());
 
     /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
      * Read "Establishing Wi-Fi or Ethernet Connection" section in
      * examples/protocols/README.md for more information about this function.
      */
     ESP_ERROR_CHECK(example_connect());
     mqtt5_app_start();
 }