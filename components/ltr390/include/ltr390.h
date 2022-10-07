
/* LTR390 sensor registers */
#define LTR390_ADDR             0x53
#define LTR390_MAIN_CTRL        0x00
#define LTR390_PART_ID          0x06
#define LTR390_MAIN_STATUS      0x07
#define LTR390_UVS_GAIN         0x05
#define LTR390_ALS_DATA0        0x0d
#define LTR390_ALS_DATA1        0x0e
#define LTR390_ALS_DATA2        0x0f
#define LTR390_UVS_DATA0        0x10
#define LTR390_UVS_DATA1        0x11
#define LTR390_UVS_DATA2        0x12

/* MAIN_CTRL register bits */
#define MAIN_CTRL_STBY          0x0 << 1
#define MAIN_CTRL_EN            0x1 << 1
#define MAIN_CTRL_MODE_ALS      0x0 << 3
#define MAIN_CTRL_MODE_UVS      0x1 << 3


/**
 * @brief Perform first time setup. 
 *      This only needs to run after a power cycle.
 * 
 * @return uint8_t 
 */
// uint8_t ltr390_init(void);
// TODO: Don't think we need this


/**
 * @brief Trigger sensor read and write payload strings to buffers.
 * 
 * @param als_buf Ambient light value buffer
 * @param uvs_buf UV value buffer
 * @return uint8_t 
 *      - I2C_OK if success
 *      - I2C_FAIL if not
 */
uint8_t ltr390_trigger_measurement(char *als_buf, char *uvs_buf);