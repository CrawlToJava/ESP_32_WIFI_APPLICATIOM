#ifndef MAIN_HTTP_SERVER_H
#define MAIN_HTTP_SERVER_H

#define OTA_UPDATE_PENDING 0
#define OTA_UPDATE_SUCCESSFUL 1
#define OTA_UPDATE_FAILED -1

/*
 * Messages for the HTTP monitor
 */
typedef enum
{
    HTTP_MSG_WIFI_CONNECT_INIT = 0,
    HTTP_MSG_WIFI_CONNECT_SUCCESS,
    HTTP_MSG_WIFI_CONNECT_FAIL,
    HTTP_MSG_OTA_UPDATE_SUCCESSFUL,
    HTTP_MSG_OTA_UPDATE_FAILED,
} http_server_messages_e;

/**
 * Structure for the message queue
 */
typedef struct
{
    http_server_messages_e msgID;
} http_server_queue_message_t;

/**
 * Sends a message to the queue
 * @param msgID message ID from the http_server_messages_e
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE
 */
BaseType_t http_server_monitor_send_message(http_server_messages_e msgID);

/**
 * Starts the HTTP server
 */
void http_server_start(void);

/**
 * Stops HTTP server
 */
void http_server_stop(void);

/**
 * Timer callback function which calls esp_restart upon successful firmware update
 */
void http_server_fw_update_reset_callback(void *arg);

#endif