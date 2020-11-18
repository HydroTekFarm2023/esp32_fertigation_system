/*
 * This is a simple application which will create a WIFI AP mode and HTTP server for POST request.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_http_server.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/*
 * Just change the below entries to strings with
 * the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
 */
#define EXAMPLE_ESP_WIFI_SSID      "esp32"      //CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      "esp12345"   //CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   1            //CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       1            //CONFIG_ESP_MAX_STA_CONN

/* A simple example that demonstrates how to create POST
 * handlers for the web server.
 */
static const char *TAG = "WIFI_AP_HTTP_SERVER";

/* An HTTP POST handler */
static esp_err_t echo_post_handler(httpd_req_t *req)
{
   char buf[200];
   int ret, remaining = req->content_len;

   while (remaining > 0) {
      /* Read the data for the request */
      if ((ret = httpd_req_recv(req, buf,
                  MIN(remaining, sizeof(buf)))) <= 0) {
         if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* Retry receiving if timeout occurred */
            continue;
         }
         return ESP_FAIL;
      }

      remaining -= ret;

      /* Log data received */
      ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
      ESP_LOGI(TAG, "%.*s", ret, buf);
      ESP_LOGI(TAG, "====================================");
   }

   // End response
   httpd_resp_send_chunk(req, NULL, 0);
   return ESP_OK;
}

static const httpd_uri_t echo = {
   .uri       = "/echo",
   .method    = HTTP_POST,
   .handler   = echo_post_handler,
   .user_ctx  = NULL
};

static httpd_handle_t start_webserver(void)
{
   httpd_handle_t server = NULL;
   httpd_config_t config = HTTPD_DEFAULT_CONFIG();

   // Start the httpd server
   ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
   if (httpd_start(&server, &config) == ESP_OK) {
      // Set URI handlers
      ESP_LOGI(TAG, "Registering URI handlers");
      httpd_register_uri_handler(server, &echo);
      return server;
   }

   ESP_LOGI(TAG, "Error starting server!");
   return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
   // Stop the httpd server
   httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
      int32_t event_id, void* event_data)
{
   httpd_handle_t* server = (httpd_handle_t*) arg;
   if (*server) {
      ESP_LOGI(TAG, "Stopping webserver");
      stop_webserver(*server);
      *server = NULL;
   }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
      int32_t event_id, void* event_data)
{
   httpd_handle_t* server = (httpd_handle_t*) arg;
   if (*server == NULL) {
      ESP_LOGI(TAG, "Starting webserver");
      *server = start_webserver();
   }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
      int32_t event_id, void* event_data)
{
   if (event_id == WIFI_EVENT_AP_STACONNECTED) {
      wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
      ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
            MAC2STR(event->mac), event->aid);
   } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
      wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
      ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
            MAC2STR(event->mac), event->aid);
   }
}

void wifi_init_softap(void)
{
   esp_netif_create_default_wifi_ap();
   /* Initialize WIFI */
   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   ESP_ERROR_CHECK(esp_wifi_init(&cfg));

   ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

   /* WIFI configuration  */
   wifi_config_t wifi_config = {
      .ap = {
         .ssid = EXAMPLE_ESP_WIFI_SSID,
         .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
         .channel = EXAMPLE_ESP_WIFI_CHANNEL,
         .password = EXAMPLE_ESP_WIFI_PASS,
         .max_connection = EXAMPLE_MAX_STA_CONN,
         .authmode = WIFI_AUTH_WPA_WPA2_PSK
      },
   };
   if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
      wifi_config.ap.authmode = WIFI_AUTH_OPEN;
   }

   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
   ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
   ESP_ERROR_CHECK(esp_wifi_start());

   ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
         EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

void app_main(void)
{
   static httpd_handle_t server = NULL;

   ESP_ERROR_CHECK(nvs_flash_init());
   ESP_ERROR_CHECK(esp_netif_init());
   ESP_ERROR_CHECK(esp_event_loop_create_default());

   /*
    * Read "Establishing Wi-Fi or Ethernet Connection" section in
    * examples/protocols/README.md for more information about this function.
    */
   wifi_init_softap();

   /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
    * and re-start it upon connection.
    */
   ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
   ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

   /* Start the server for the first time */
   server = start_webserver();
}
