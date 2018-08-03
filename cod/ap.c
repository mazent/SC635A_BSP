#include "ap.h"

#include "esp_wifi.h"

static const char *TAG = "ap";

bool AP_beg(const S_AP * ap)
{
	bool esito = false ;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {
        .ap = {
            .max_connection = ap->max_connection,
			.channel = 9
        }
    };

    do {
    	if (ESP_OK != esp_wifi_init(&cfg))
    		break ;

    	strcpy((char *) wifi_config.ap.ssid, ap->ssid) ;
    	wifi_config.ap.ssid_len = strlen(ap->ssid) ;
    	wifi_config.ap.ssid_hidden = ap->ssid_hidden ;
    	switch (ap->auth) {
        case AUTH_OPEN:
        	wifi_config.ap.authmode = WIFI_AUTH_OPEN ;
        	break ;
        case AUTH_WEP:
        	wifi_config.ap.authmode = WIFI_AUTH_WEP ;
        	break ;
        case AUTH_WPA_WPA2_PSK:
        	wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK ;
        	break ;
    	}

    	if (ESP_OK != esp_wifi_set_mode(WIFI_MODE_AP))
    		break ;

    	if (ESP_OK != esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config))
    		break ;

#if 0
    	if (ESP_OK != esp_wifi_set_protocol(ESP_IF_WIFI_AP, WIFI_PROTOCOL_11B))
    		break ;
#endif
#if 1
    	if (ESP_OK != esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, WIFI_BW_HT20))
    		break ;
#endif
    	if (ESP_OK != esp_wifi_start())
    		break ;


    	esito = true ;
    } while (false) ;

    return esito ;
}

void AP_end(void)
{
	esp_wifi_stop() ;
}

void AP_evn(AP_EVN evn, void * v)
{
	system_event_info_t * info = v ;

	switch (evn) {
    case AP_EVN_START:
    case AP_EVN_STOP:
    	break ;
    case AP_EVN_STACONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                 MAC2STR(info->sta_connected.mac),
                 info->sta_connected.aid);
        break ;
    case AP_EVN_STADISCONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                 MAC2STR(info->sta_disconnected.mac),
                 info->sta_disconnected.aid);
        break ;
    case AP_EVN_STAIPASSIGNED:
    	break ;
	}
}
