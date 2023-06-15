/** @file wifi_station.h
 * @author Czira Bence (czirabence@gmail.com)
 * 
 * @brief This is a modified version of esp-idf/examples/wifi/getting_started/station.
 * Set <b>CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD, CONFIG_ESP_MAXIMUM_RETRY</b>
 * and <b>ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD</b> under <b>Wifi Configuration</b> submenu in project configuration menu.
 * 
 * @version 0.1
 * @date 2023-06-12 
*/
/**
 * @brief initialize and start wifi station. Esp event loop must be created 
 * via esp_event_loop_create_default() before this function is called.
*/
void wifi_init_sta(void);

/**
 * @brief Initialise nvs. 
 * 
 */
void nvs_init(void);