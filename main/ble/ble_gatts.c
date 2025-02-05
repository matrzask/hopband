#include "ble_gatts.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "ble_gap.h"

#include "services/gps_service.h"
#include "services/wifi_service.h"
#include "services/thermistor_service.h"
#include "services/heartrate_service.h"

#define TAG "BLE-GATTS"

#define PROFILE_NUM 1
#define PROFILE_APP_IDX 0
#define ESP_APP_ID 0x55
#define SVC_INST_ID 0

#define PREPARE_BUF_MAX_SIZE 1024

#define ADV_CONFIG_FLAG (1 << 0)
#define SCAN_RSP_CONFIG_FLAG (1 << 1)

static uint16_t gatts_mtu = 23;

typedef struct
{
    uint8_t *prepare_buf;
    int prepare_len;
    uint16_t handle;
} prepare_type_env_t;

static prepare_type_env_t prepare_write_env;
static prepare_type_env_t prepare_read_env;

struct gatts_profile_inst
{
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

typedef struct
{
    uint16_t gatts_if;
    uint16_t conn_id;
    bool has_conn;
} gatts_profile_inst_t;

static gatts_profile_inst_t profile;

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst bt_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

void gatts_proc_read(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_read_env, esp_ble_gatts_cb_param_t *param, uint8_t *p_rsp_v, uint16_t v_len)
{
    if (!param->read.need_rsp)
    {
        return;
    }
    uint16_t value_len = gatts_mtu - 1;
    if (v_len - param->read.offset < (gatts_mtu - 1))
    { // read response will fit in one MTU?
        value_len = v_len - param->read.offset;
    }
    else if (param->read.offset == 0) // it's the start of a long read  (could also use param->read.is_long here?)
    {
        ESP_LOGI(TAG, "long read, handle = %d, value len = %d", param->read.handle, v_len);

        if (v_len > PREPARE_BUF_MAX_SIZE)
        {
            ESP_LOGE(TAG, "long read too long");
            return;
        }
        if (prepare_read_env->prepare_buf != NULL)
        {
            ESP_LOGW(TAG, "long read buffer not free");
            free(prepare_read_env->prepare_buf);
            prepare_read_env->prepare_buf = NULL;
        }

        prepare_read_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_read_env->prepare_len = 0;
        if (prepare_read_env->prepare_buf == NULL)
        {
            ESP_LOGE(TAG, "long read no mem");
            return;
        }
        memcpy(prepare_read_env->prepare_buf, p_rsp_v, v_len);
        prepare_read_env->prepare_len = v_len;
        prepare_read_env->handle = param->read.handle;
    }
    esp_gatt_rsp_t rsp;
    memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
    rsp.attr_value.handle = param->read.handle;
    rsp.attr_value.len = value_len;
    rsp.attr_value.offset = param->read.offset;
    memcpy(rsp.attr_value.value, &p_rsp_v[param->read.offset], value_len);
    esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
}

// continuation of read, use buffered value
void gatts_proc_long_read(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_read_env, esp_ble_gatts_cb_param_t *param)
{

    if (prepare_read_env->prepare_buf && (prepare_read_env->handle == param->read.handle)) // something buffered?
    {
        gatts_proc_read(gatts_if, prepare_read_env, param, prepare_read_env->prepare_buf, prepare_read_env->prepare_len);
        if (prepare_read_env->prepare_len - param->read.offset < (gatts_mtu - 1)) // last read?
        {
            free(prepare_read_env->prepare_buf);
            prepare_read_env->prepare_buf = NULL;
            prepare_read_env->prepare_len = 0;
            ESP_LOGI(TAG, "long_read ended");
        }
    }
    else
    {
        ESP_LOGE(TAG, "long_read not buffered");
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
    {
        profile.gatts_if = gatts_if;
        ble_gap_setAdvertisingData();

        /*
         *  Add calls to register new attribute tables here
         *  Template:
         *  esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(#SERVICE GATT DB#, gatts_if, #SERVICE MAX INDEX#, SVC_INST_ID);
         *  if (create_attr_ret){
         *      ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
         *  }
         */

        esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gps_serv_gatt_db, gatts_if, GPS_SERV_NUM_ATTR, SVC_INST_ID);
        if (create_attr_ret)
        {
            ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
        }

        create_attr_ret = esp_ble_gatts_create_attr_tab(wifi_serv_gatt_db, gatts_if, WIFI_SERV_NUM_ATTR, SVC_INST_ID);
        if (create_attr_ret)
        {
            ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
        }

        create_attr_ret = esp_ble_gatts_create_attr_tab(therm_serv_gatt_db, gatts_if, THERM_SERV_NUM_ATTR, SVC_INST_ID);
        if (create_attr_ret)
        {
            ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
        }

        create_attr_ret = esp_ble_gatts_create_attr_tab(heartrate_serv_gatt_db, gatts_if, HEARTRATE_SERV_NUM_ATTR, SVC_INST_ID);
        if (create_attr_ret)
        {
            ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
        }

        /* End calls to register external service attribute tables */
    }
    break;
    case ESP_GATTS_READ_EVT:
    {
        // Note that if char is defined with ESP_GATT_AUTO_RSP, then this event is triggered after the internal value has been sent.  So this event is not very useful.
        // Otherwise if it is ESP_GATT_RSP_BY_APP, then call helper function gatts_proc_read()
        if (!param->read.is_long)
        {
            // If is.long is false then this is the first (or only) request to read data
            int attrIndex;
            esp_gatt_rsp_t rsp;
            rsp.attr_value.len = 0;
            /*
             *  Add calls to handle read events for each external service here
             *  Template:
             *  if( (attrIndex = getAttributeIndexBy#SERVICE#Handle(param->read.handle)) < #SERVICE MAX INDEX# )
             *  {
             *      ESP_LOGI(TAG, "#SERVICE# READ");
             *      handle#SERVICE#ReadEvent(attrIndex, param, &rsp);
             *  }
             */

            if ((attrIndex = getAttributeIndexByGpsHandle(param->read.handle)) < GPS_SERV_NUM_ATTR)
            {
                ESP_LOGI(TAG, "GPS READ");
                handleGpsReadEvent(attrIndex, param, &rsp);
            }

            if ((attrIndex = getAttributeIndexByWifiHandle(param->read.handle)) < WIFI_SERV_NUM_ATTR)
            {
                ESP_LOGI(TAG, "WIFI READ");
                handleWifiReadEvent(attrIndex, param, &rsp);
            }

            if ((attrIndex = getAttributeIndexByThermHandle(param->read.handle)) < THERM_SERV_NUM_ATTR)
            {
                ESP_LOGI(TAG, "THERM READ");
                handleThermReadEvent(attrIndex, param, &rsp);
            }

            if ((attrIndex = getAttributeIndexByHeartrateHandle(param->read.handle)) < HEARTRATE_SERV_NUM_ATTR)
            {
                ESP_LOGI(TAG, "HEARTRATE READ");
                handleHeartrateReadEvent(attrIndex, param, &rsp);
            }

            /* END read handler calls for external services */

            // Helper function sends what it can (up to MTU size) and buffers the rest for later.
            gatts_proc_read(gatts_if, &prepare_read_env, param, rsp.attr_value.value, rsp.attr_value.len);
        }
        else // a continuation of a long read.
        {
            // Dont invoke the handle#SERVICE#ReadEvent again, just keep pumping out buffered data.
            gatts_proc_long_read(gatts_if, &prepare_read_env, param);
        }
    }
    break;
    case ESP_GATTS_WRITE_EVT:
    {
        esp_gatt_status_t status = ESP_GATT_WRITE_NOT_PERMIT;
        int attrIndex;

        if (!param->write.is_prep)
        {
            // This section handles writes where the length is less than or = MTU.  (not long writes)
            // (But the length of write data must always be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.)
            ESP_LOGD(TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, param->write.value, param->write.len, ESP_LOG_DEBUG);

            /*
             *  Add calls to handle write events for each external service here
             *  Template:
             *  if( (attrIndex = getAttributeIndexBy#SERVICE#Handle(param->read.handle)) < #SERVICE MAX INDEX# )
             *  {
             *      ESP_LOGI(TAG, "#SERVICE# WRITE");
             *      handle#SERVICE#WriteEvent(attrIndex, param->write.value, param->write.len);
             *      status = ESP_GATT_OK;
             *  }
             */

            if ((attrIndex = getAttributeIndexByWifiHandle(param->write.handle)) < WIFI_SERV_NUM_ATTR)
            {
                ESP_LOGI(TAG, "WIFI WRITE");
                handleWifiWriteEvent(attrIndex, param->write.value, param->write.len);
                status = ESP_GATT_OK;
            }

            /* END write handler calls for external services */

            /* send response when param->write.need_rsp is true*/
            if (param->write.need_rsp)
            {
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
            }
        }
        else
        {
            /* handle prepare write */
            prepare_write_env.handle = param->write.handle; // keep track of the handle for ESP_GATTS_EXEC_WRITE_EVT since it doesn't provide it.
                                                            // Note: if characteristic set to ESP_GATT_AUTO_RSP then long write is handled internally.
                                                            // Otherwise if ESP_GATT_RSP_BY_APP, then call helper function here (and later handle the final write in ESP_GATTS_EXEC_WRITE_EVT)
            // example_prepare_write_event_env(gatts_if, &prepare_write_env, param);
        }
    }

    break;
    case ESP_GATTS_EXEC_WRITE_EVT:
    {
        esp_gatt_status_t status = ESP_GATT_OK;
        int attrIndex;
        // This section handles long writes where the length is greater than MTU.
        // (But the length of write data must always be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.)
        ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");

        if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC) // Make sure long write was not aborted.
        {
            uint16_t char_val_len = 0;
            const uint8_t *char_val_p;
            if (prepare_write_env.prepare_buf == NULL)
            {
                // Note: if characteristic set to ESP_GATT_AUTO_RSP, long write is handled internally.  Get value from API.
                esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->write.handle, &char_val_len, &char_val_p);
                if (get_attr_ret != ESP_OK)
                {
                    ESP_LOGE(TAG, "ERROR: %d\n", get_attr_ret);
                }
            }
            else // Otherwise if ESP_GATT_RSP_BY_APP
            {
                char_val_p = prepare_write_env.prepare_buf;
                char_val_len = prepare_write_env.prepare_len;
            }

            /*
             *  Add calls to handle long-write events for each external service here
             *  Template:
             *  if( (attrIndex = getAttributeIndexBy#SERVICE#Handle(prepare_write_env.handle)) < #SERVICE MAX INDEX# )
             *  {
             *      ESP_LOGI(TAG, "#SERVICE# WRITE");
             *      handle#SERVICE#WriteEvent(attrIndex, char_val_p, char_val_len);
             *      status = ESP_GATT_OK;
             *  }
             */

            /* END write handler calls for external services */
        }
        else // not ESP_GATT_PREP_WRITE_EXEC
        {
            ESP_LOGW(TAG, "ESP_GATT_PREP_WRITE_CANCEL");
        }

        // example_exec_write_event_env(&prepare_write_env, param); // this cleans-up the long-write.
        /* Always send response for EXEC_WRITE */
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
    }
    break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);

        gatts_mtu = param->mtu.mtu;
        break;
    case ESP_GATTS_CONNECT_EVT:
        gatts_mtu = 23;
        profile.conn_id = param->connect.conn_id;
        profile.has_conn = true;
        ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, param->connect.remote_bda, 6, ESP_LOG_INFO);
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20; // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10; // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
        // start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        profile.has_conn = false;
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, reason = %d", param->disconnect.reason);
        ble_gap_startAdvertising();
        break;
    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
    {
        if (param->add_attr_tab.status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
        }

        /*
         *  Add calls to start external services
         *  Template:
         *  else if(param->add_attr_tab.svc_uuid.uuid.uuid16 == #SERVICE UUID#)
         *  {
         *      if( param->add_attr_tab.num_handle != #SERVICE MAX INDEX#)
         *      {
         *          ESP_LOGE(TAG,"create attribute table abnormall, num_handle (%d) isn't equal to #SERVICE MAX INDEX#(%d)", param->add_attr_tab.num_handle, #SERVICE MAX INDEX#);
         *      }
         *      else
         *      {
         *          ESP_LOGI(TAG,"create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
         *          memcpy(#SERVICE#_handle_table, param->add_attr_tab.handles, sizeof(#SERVICE#_handle_Table));
         *          esp_ble_gatts_start_service(#SERVICE#_handle_table[#SERVICE INDEX 0#]);
         *      }
         *  }
         *
         */
        else if (param->add_attr_tab.svc_uuid.uuid.uuid16 == GPS_SERV_uuid)
        {
            if (param->add_attr_tab.num_handle != GPS_SERV_NUM_ATTR)
            {
                ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) isn't equal to INFO_NB(%d)", param->add_attr_tab.num_handle, GPS_SERV_NUM_ATTR);
            }
            else
            {
                ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
                memcpy(gps_handle_table, param->add_attr_tab.handles, sizeof(gps_handle_table));
                esp_ble_gatts_start_service(gps_handle_table[GPS_SERV]);
            }
        }

        else if (param->add_attr_tab.svc_uuid.uuid.uuid16 == WIFI_SERV_uuid)
        {
            if (param->add_attr_tab.num_handle != WIFI_SERV_NUM_ATTR)
            {
                ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) isn't equal to INFO_NB(%d)", param->add_attr_tab.num_handle, WIFI_SERV_NUM_ATTR);
            }
            else
            {
                ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
                memcpy(wifi_handle_table, param->add_attr_tab.handles, sizeof(wifi_handle_table));
                // esp_ble_gatts_start_service(wifi_handle_table[WIFI_SERV]);
            }
        }

        else if (param->add_attr_tab.svc_uuid.uuid.uuid16 == THERM_SERV_uuid)
        {
            if (param->add_attr_tab.num_handle != THERM_SERV_NUM_ATTR)
            {
                ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) isn't equal to INFO_NB(%d)", param->add_attr_tab.num_handle, THERM_SERV_NUM_ATTR);
            }
            else
            {
                ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
                memcpy(therm_handle_table, param->add_attr_tab.handles, sizeof(therm_handle_table));
                esp_ble_gatts_start_service(therm_handle_table[THERM_SERV]);
            }
        }

        else if (param->add_attr_tab.svc_uuid.uuid.uuid16 == HEARTRATE_SERV_uuid)
        {
            if (param->add_attr_tab.num_handle != HEARTRATE_SERV_NUM_ATTR)
            {
                ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) isn't equal to INFO_NB(%d)", param->add_attr_tab.num_handle, HEARTRATE_SERV_NUM_ATTR);
            }
            else
            {
                ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
                memcpy(heartrate_handle_table, param->add_attr_tab.handles, sizeof(heartrate_handle_table));
                esp_ble_gatts_start_service(heartrate_handle_table[HEARTRATE_SERV]);
            }
        }
        /* END start external services */
        break;
    }
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            bt_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        }
        else
        {
            ESP_LOGE(TAG, "reg app failed, app_id %04x, status %d",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }
    do
    {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++)
        {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == bt_profile_tab[idx].gatts_if)
            {
                if (bt_profile_tab[idx].gatts_cb)
                {
                    bt_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

// Helper function for services which fills in the gatts_if and conn_id
esp_err_t ble_gatt_server_notify_indicate(uint16_t attr_handle, uint16_t value_len, uint8_t *value, bool need_confirm)
{
    if (profile.has_conn)
    {
        // the size of notify_data[] need less than MTU size
        return esp_ble_gatts_send_indicate(profile.gatts_if, profile.conn_id, attr_handle, value_len, value, need_confirm);
    }
    return ESP_FAIL;
}

void hide_service(uint16_t service_handle)
{
    esp_err_t stop_ret = esp_ble_gatts_stop_service(service_handle);
    if (stop_ret)
    {
        ESP_LOGE(TAG, "stop service failed, error code = %x", stop_ret);
    }
    else
    {
        ESP_LOGI(TAG, "stop service successfully");
    }
}

void show_service(uint16_t service_handle)
{
    esp_err_t start_ret = esp_ble_gatts_start_service(service_handle);
    if (start_ret)
    {
        ESP_LOGE(TAG, "start service failed, error code = %x", start_ret);
    }
    else
    {
        ESP_LOGI(TAG, "start service successfully");
    }
}

esp_err_t ble_gatt_server_init(void)
{
    esp_err_t err;

    err = esp_ble_gatts_register_callback(gatts_event_handler);
    if (err)
    {
        ESP_LOGE(TAG, "Gatts register failed with err: %x", err);
        return err;
    }

    err = esp_ble_gatts_app_register(ESP_APP_ID);
    if (err)
    {
        ESP_LOGE(TAG, "App register failed with err: %x", err);
        return err;
    }

    err = esp_ble_gatt_set_local_mtu(517);
    if (err)
    {
        ESP_LOGE(TAG, "set local MTU failed, error code = %x", err);
    }

    return err;
}