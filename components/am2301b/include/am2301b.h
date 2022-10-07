
/* AM2301B sensor registers */
#define AM2301B_ADDR            0x38
#define AM2301B_STATUS_BYTE     0x71
#define AM2301B_STATUS_OK       0x18
#define AM2301B_TRIG_MEAS1      0xAC
#define AM2301B_TRIG_MEAS2      0x33
#define AM2301B_TRIG_MEAS3      0x00


/**
 * @brief Perform first time setup. This only needs to be run after a power on.
 * 
 * @return uint8_t 
 *      - I2C_OK if success
 *      - I2C_FAIL if not
 */
uint8_t am2301b_init(void);


/**
 * @brief Trigger sensor read and write payload strings to buffers.
 * 
 * @param rel_hum   Where to store the relative humidity signal
 * @param temp      Where to store the temperature output signal
 * @return uint8_t
 *      - I2C_OK if success
 *      - I2C_FAIL if not
 */
uint8_t am2301b_trigger_measurement(char *rel_hum_buf, char *temp_buf);
