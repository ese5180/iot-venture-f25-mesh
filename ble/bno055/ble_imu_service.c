/**
 * @file ble_imu_service.c
 * @brief BLE GATT service implementation for IMU data
 * Fixed: Auto-restart advertising after disconnect
 */
#include "ble_imu_service.h"
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/uuid.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// LOG_MODULE_REGISTER(ble_imu_service, LOG_LEVEL_DBG);
LOG_MODULE_REGISTER(ble_service, LOG_LEVEL_DBG);

/* Connection handle */
static struct bt_conn *current_conn = NULL;//Single BLE Conncetion

/* Notification flags */
static bool quaternion_notify_enabled = false;
static bool euler_notify_enabled = false;
static bool notify_enabled = false; //BME280 & pressure sensor

/* Control command callback */
static ble_imu_control_callback_t control_callback = NULL;

/* Advertising parameters (saved for restart) */
static struct bt_le_adv_param adv_param = {
    .id = BT_ID_DEFAULT,
    .options = BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_NAME,//can be connected and broadcast automatically
    .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,//FAST Broadcast mode
    .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
};

static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_IMU_SERVICE_VAL),
};

/* Forward declarations */
static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
static void quaternion_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value);
static void euler_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value);
static ssize_t control_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static int restart_advertising(void);
static void restart_adv_work_handler(struct k_work *work);

/* Work queue for delayed advertising restart */
static K_WORK_DELAYABLE_DEFINE(restart_adv_work, restart_adv_work_handler);

/* GATT Service Definition */
BT_GATT_SERVICE_DEFINE(imu_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_IMU_SERVICE),
    
    /* Quaternion Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_IMU_QUATERNION,
                          BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_NONE,
                          NULL, NULL, NULL),
    BT_GATT_CCC(quaternion_ccc_changed,
               BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),//client character configuration
    
    /* Euler Angles Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_IMU_EULER,
                          BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_NONE,
                          NULL, NULL, NULL),
    BT_GATT_CCC(euler_ccc_changed,
               BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    /* Temperature Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_TMP,
                          BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_NONE,
                          NULL, NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed,
               BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    /* Humidity Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY,
                          BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_NONE,
                          NULL, NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed,
               BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    /* Pressure Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_PRESSURE,
                          BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_NONE,
                          NULL, NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed,
               BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    
    /* Control Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_IMU_CONTROL,
                          BT_GATT_CHRC_WRITE,
                          BT_GATT_PERM_WRITE,
                          NULL, control_write, NULL),
);

/**
 * @brief Restart BLE advertising
 */
static int restart_advertising(void)
{
    int err;

    LOG_INF("Attempting to restart advertising...");

    /* Stop advertising first (in case it's still running) */
    err = bt_le_adv_stop();
    if (err && err != -EALREADY) {
        LOG_DBG("Stop advertising returned: %d (may be already stopped)", err);
    }

    /* Longer delay to ensure complete cleanup of BLE stack resources */
    k_msleep(500);

    /* Start advertising with retry mechanism */
    for (int retry = 0; retry < 3; retry++) {
        err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
        
        if (err == 0) {
            LOG_INF("✓ BLE advertising restarted successfully");
            return 0;
        }
        
        if (err == -EALREADY) {
            LOG_WRN("Advertising already active, stopping first...");
            bt_le_adv_stop();
            k_msleep(500);
            continue;
        }
        
        LOG_WRN("Advertising restart attempt %d failed: %d", retry + 1, err);
        
        if (retry < 2) {
            k_msleep(1000);  // Wait longer between retries
        }
    }

    LOG_ERR("Failed to restart advertising after 3 attempts: %d", err);
    return err;
}

/**
 * @brief Work handler for restarting advertising
 */
static void restart_adv_work_handler(struct k_work *work)
{
    LOG_INF("Workqueue: Restarting advertising after disconnect...");
    restart_advertising();
}

/**
 * @brief Connection callback
 */
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection failed: %d", err);
        
        /* Restart advertising on connection failure */
        restart_advertising();
        return;
    }

    current_conn = bt_conn_ref(conn);
    LOG_INF("✓ BLE Connected");
}

/**
 * @brief Disconnection callback
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
    LOG_INF("BLE Disconnected from %s, reason: 0x%02x", addr, reason);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }

    quaternion_notify_enabled = false;
    euler_notify_enabled = false;
    notify_enabled = false;


    /* Use workqueue to restart advertising to avoid blocking BLE callback */
    k_work_schedule(&restart_adv_work, K_MSEC(500));
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/**
 * @brief Quaternion CCC changed callback
 */

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}
 
static void quaternion_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    quaternion_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Quaternion notifications %s", 
            quaternion_notify_enabled ? "enabled" : "disabled");
}

/**
 * @brief Euler angles CCC changed callback
 */
static void euler_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    euler_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Euler notifications %s", 
            euler_notify_enabled ? "enabled" : "disabled");
}

/**
 * @brief Control characteristic write callback
 */
static ssize_t control_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    if (offset != 0 || len != 1) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    uint8_t cmd = *(uint8_t *)buf;
    
    switch (cmd) {
    case BLE_IMU_CMD_START:
        LOG_INF("Received START command");
        break;
        
    case BLE_IMU_CMD_STOP:
        LOG_INF("Received STOP command");
        break;
        
    case BLE_IMU_CMD_RESET:
        LOG_INF("Received RESET command");
        break;
        
    case BLE_IMU_CMD_CALIBRATE:
        LOG_INF("Received CALIBRATE command");
        break;
        
    case BLE_IMU_CMD_SET_ZERO:
        LOG_INF("🎯 Received SET ZERO POINT command");
        break;
        
    default:
        LOG_WRN("Unknown control command: 0x%02x", cmd);
        return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
    }

    /* Call registered callback if available */
    if (control_callback) {
        control_callback(cmd);
    }

    return len;
}

/**
 * @brief Initialize BLE IMU service
 */
int ble_imu_service_init(void)
{
    int err;
    LOG_INF("Initializing BLE IMU service");

    /* Enable Bluetooth */
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed: %d", err);
        return err;
    }
    LOG_INF("Bluetooth initialized");

    /* Start advertising */
    err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start: %d", err);
        return err;
    }
    LOG_INF("BLE advertising started");
    return 0;
}

/**
 * @brief Send attitude data via BLE
 */
int ble_imu_service_send_attitude(const attitude_t *attitude)
{
    int err;
    char msg_qua[64],msg_euler[96];

    if (!current_conn) {
        return -ENOTCONN;
    }

    /* Send quaternion data if subscribed */
    if (quaternion_notify_enabled) {
        // ble_quaternion_packet_t quat_packet = {
        //     .w = attitude->quaternion.w,
        //     .x = attitude->quaternion.x,
        //     .y = attitude->quaternion.y,
        //     .z = attitude->quaternion.z,
        //     .timestamp = attitude->timestamp
        // };
        
        int len = snprintf(msg_qua, sizeof(msg_qua), "W=%.3f, X=%.3f, Y=%.3f, Z=%.3f", attitude->quaternion.w,attitude->quaternion.x,attitude->quaternion.y,attitude->quaternion.z);
        bt_gatt_notify(current_conn, &imu_svc.attrs[1], msg_qua, len);
    }

    /* Send Euler angles if subscribed */
    if (euler_notify_enabled) {
        // ble_euler_packet_t euler_packet = {
        //     .roll = attitude->euler.roll,
        //     .pitch = attitude->euler.pitch,
        //     .yaw = attitude->euler.yaw,
        //     .timestamp = attitude->timestamp
        // };
        float roll_deg = attitude->euler.roll * 180.0f / M_PI;
        float pitch_deg = attitude->euler.pitch * 180.0f / M_PI;
        float yaw_deg = attitude->euler.yaw * 180.0f / M_PI;

        int len = snprintf(msg_euler, sizeof(msg_euler), "Roll=%.1f, Pitch=%.1f, Yaw=%.1f", roll_deg, pitch_deg, yaw_deg);
        bt_gatt_notify(current_conn, &imu_svc.attrs[4], msg_euler, len);
    }

    return 0;
}

/**
 * @brief Send temperature attitude data via BLE
 */
int ble_env_notify_temperature(double temp_c)
{
    if (!ble_env_notify_enabled()) {
        return -ENOTCONN;
    }
    char str_temp[16];
    int len = snprintf(str_temp, sizeof(str_temp), "T=%.2f", temp_c);
    return bt_gatt_notify(current_conn, &imu_svc.attrs[7],
                          str_temp, len);
}

/**
 * @brief Send humidity attitude data via BLE
 */
int ble_env_notify_humidity(double rh)
{
    if (!ble_env_notify_enabled()) {
        return -ENOTCONN;
    }
    char str_hum[16];
    int len = snprintf(str_hum, sizeof(str_hum), "RH=%.2f", rh);
    return bt_gatt_notify(current_conn, &imu_svc.attrs[10],
                          str_hum, len);
}

int ble_env_notify_bruxism(int episodes_left, int episodes_right, int pressure_left, int pressure_right, int total_count)
{
    if (!ble_env_notify_enabled()) {
        return -ENOTCONN;
    }
    char str_press[32];
    int len = snprintf(str_press, sizeof(str_press), "left=%d,right=%d", episodes_left, episodes_right);
    return bt_gatt_notify(current_conn, &imu_svc.attrs[13],
                          str_press, len);
}


/**
 * @brief Check if imu notifications are enabled
 */
bool ble_imu_service_is_subscribed(void)
{
    return (quaternion_notify_enabled || euler_notify_enabled);
}

/**
 * @brief Check if temp & humidity & pressure notifications are enabled
 */
bool ble_env_notify_enabled(void)
{
    return notify_enabled && current_conn != NULL;
}

/**
 * @brief Check connection status
 */
bool ble_imu_service_is_connected(void)
{
    return (current_conn != NULL);
}

/**
 * @brief Register control command callback
 */
void ble_imu_service_register_control_callback(ble_imu_control_callback_t callback)
{
    control_callback = callback;
    LOG_INF("Control command callback registered");
}

/**
 * @brief Manually restart advertising (for debug/testing)
 */
int ble_imu_service_restart_advertising(void)
{
    return restart_advertising();
}