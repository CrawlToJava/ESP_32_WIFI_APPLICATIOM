#include "wifi_app.h"
#include "http_server.h"

#include "freertos/event_groups.h" // Библиотека отвечающая за обработку событий
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/netdb.h" // Библиотека котора преобразует домены в IP адреса

#include "rgb_led.h"
#include "tasks_common.h"

// Tag used for ESP serial monitor
static const char *TAG = "wifi_app";

// Used for returning the WiFi configuration
wifi_config_t *wifi_config = NULL;

// Used to track the number for retries when a connection attempt fails
static int g_retry_number;

static QueueHandle_t wifi_app_queue;

// netif objects for the station and AP
esp_netif_t *esp_netif_sta = NULL;
esp_netif_t *esp_netif_ap = NULL;

/**
 * WiFi application event handler
 * @param arg data, aside from data, that is passed to the handler when it is called
 * @param event_base the base is of the event to register the handler for
 * @param event_id the id for the event to register the handler for
 * @param event_data data
 */
static void wifi_app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_AP_START:
            ESP_LOGI(TAG, "WIFI_EVENT_AP_START");
            break;

        case WIFI_EVENT_AP_STOP:
            ESP_LOGI(TAG, "WIFI_EVENT_AP_STOP");
            break;

        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED");
            break;

        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_AP_DISSTACONNECTED");
            break;

        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
            break;

        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");

            wifi_event_sta_disconnected_t *wifi_event_sta_disconnected = (wifi_event_sta_disconnected_t *)malloc(sizeof(wifi_event_sta_disconnected_t));
            *wifi_event_sta_disconnected = *((wifi_event_sta_disconnected_t *)event_data);
            printf("WIFI_EVENT_STA_DISCONNECTED, reason code %d\n", wifi_event_sta_disconnected->reason);

            if (g_retry_number < MAX_CONNECTIONS_RETRIES)
            {
                esp_wifi_connect();
                g_retry_number++;
            }
            else
            {
                wifi_app_send_message(WIFI_APP_MSG_STA_DISCONNECTED);
            }

            break;
        default:
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");

            wifi_app_send_message(WIFI_APP_MSG_STA_CONNECTED_GOT_IP);

            break;
        default:
            break;
        }
    }
}

/**
 * Initializes the WiFi application event handler for WiFi and IP events.
 */
static void wifi_app_event_handler_init(void)
{
    // Event loop for the WiFi driver
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Event handler for the connection
    esp_event_handler_instance_t instance_wifi_event;
    esp_event_handler_instance_t instance_ip_event;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_wifi_event));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_ip_event));
}

/**
 * Initializes the TCP stack and default
 */
static void wifi_app_default_wifi_init(void)
{
    // Initialize the TCP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Default WiFi config
    wifi_init_config_t wifi_init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    esp_netif_sta = esp_netif_create_default_wifi_sta();
    esp_netif_ap = esp_netif_create_default_wifi_ap();
}

/**
 * Configures the WiFi acces point settings and assigns the static IP to the softAp
 */
static void wifi_app_soft_ap_config(void)
{
    wifi_config_t ap_config =
        {
            .ap = {
                .ssid = WIFI_AP_SSID,
                .ssid_len = strlen(WIFI_AP_SSID),
                .password = WIFI_AP_PASSWORD,
                .channel = WIFI_AP_CHANNEL,
                .ssid_hidden = WIIF_AP_SSID_HIDDEN,
                .authmode = WIFI_AUTH_WPA2_PSK,
                .max_connection = WIFI_AP_MAX_CONNECTIONS,
                .beacon_interval = WIFI_AP_BEACON_INTERVAL,
            },
        };

    // Configure DHCP for the AP
    esp_netif_ip_info_t ap_ip_info;                // В данную структуру происходит запись параметров
    memset(&ap_ip_info, 0x00, sizeof(ap_ip_info)); // Функция обеспечивает устнановку всех байтов структуры в ноль, перед записью значений
    /**
     * Этот вызов останавливает DHCP сервер, который обслуживает точку доступа.
     * Таким образом, мы гарантируем, что сервер остановлен перед его настройкой с новыми параметрами
     * must call this first
     */
    esp_netif_dhcps_stop(esp_netif_ap);

    /*
     * функция inet_pton переводит текстовый вариант ip адреса, gateway, netmask, в бинарный формат.
     */
    inet_pton(AF_INET, WIFI_AP_IP, &ap_ip_info.ip); ///> Assign access point's static IP, GW, and netmask
    inet_pton(AF_INET, WIFI_AP_GATEWAY, &ap_ip_info.gw);
    inet_pton(AF_INET, WIFI_AP_NETMASK, &ap_ip_info.netmask);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info)); ///> Statically configure the network interface
    ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap));              ///> Start the AP DHCP server (for connecting stations e.g. your mobile device)

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));                    ///> Setting the mode as Access Point / Station Mode
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));       ///> Set our configuration
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_AP_BANDWIDTH)); ///> Our default bandwidth 20 MHz
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_STA_POWER_SAVE));                  ///> Power save set to "NONE"
}

/**
 * Connecting the ESP32 to an external AP using the update station configuration
 */
static void wifi_app_connect_sta(void)
{
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_app_get_wifi_config()));
    ESP_ERROR_CHECK(esp_wifi_connect());
}

/**
 * Main task for the WiFi application
 * @param pvParameters which can be passed to the task
 */
static void wifi_app_task(void *pvParametr)
{
    wifi_app_queue_message_t msg;

    // Inititalize the event handler
    wifi_app_event_handler_init();

    // Initialize TCP/IP stack
    wifi_app_default_wifi_init();

    // SoftAp config
    wifi_app_soft_ap_config();

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);

    while (1)
    {
        if (xQueueReceive(wifi_app_queue, &msg, portMAX_DELAY))
        {
            switch (msg.msgID)
            {
            case WIFI_APP_MSG_START_HTTP_SERVER:
                ESP_LOGI(TAG, "WIFI_APP_MSG_START_HTTP_SERVER");
                http_server_start();
                rgb_led_http_server_started();
                break;
            case WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER:
                ESP_LOGI(TAG, "WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER");

                // Attempt a connection
                wifi_app_connect_sta();

                // Set current numbers of retries to zero
                g_retry_number = 0;

                // Let the HTTP server know about the connection attempt
                http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_INIT);

                break;
            case WIFI_APP_MSG_STA_CONNECTED_GOT_IP:
                ESP_LOGI(TAG, "WIFI_APP_MSG_STA_CONNECTED_GOT_IP");
                rgb_led_wifi_connected();
                http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_SUCCESS);
                break;
            case WIFI_APP_MSG_STA_DISCONNECTED:
                ESP_LOGI(TAG, "WIFI_APP_MSG_DISCONNECTED");
                http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_FAIL);
                break;
            default:
                break;
            }
        }
    }
}

BaseType_t wifi_app_send_message(wifi_app_message_e msgID)
{
    wifi_app_queue_message_t msg = {
        .msgID = msgID};
    return xQueueSend(wifi_app_queue, &msg, portMAX_DELAY);
}

wifi_config_t *wifi_app_get_wifi_config(void)
{
    if (wifi_config == NULL)
    {
        wifi_config = (wifi_config_t *)malloc(sizeof(wifi_config_t));
    }
    return wifi_config;
}

void wifi_app_start(void)
{
    ESP_LOGI(TAG, "STARTING WIFI APPLICATION");

    // Start WiFi started LED
    rgb_led_wifi_app_started();

    // Disable default wifi logging messages
    esp_log_level_set("wifi", ESP_LOG_NONE);

    // Allocate memory for the wifi_configuration
    wifi_config = (wifi_config_t *)malloc(sizeof(wifi_config_t));

    memset(wifi_config, 0x00, sizeof(wifi_config_t)); // Функция обеспечивает устнановку всех байтов структуры в ноль, перед записью значений

    // Create message queue
    wifi_app_queue = xQueueCreate(3, sizeof(wifi_app_queue_message_t));

    // Start the wifi application
    xTaskCreatePinnedToCore(&wifi_app_task, "wifi app task", WIFI_APP_STACK_SIZE, NULL, WIFI_APP_TASK_PRIORITY, NULL, WIFI_APP_TASK_CORE_ID);
}
