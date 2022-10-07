#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "driver/i2c.h"

#include "i2c_helpers.h"
#include "am2301b.h"


static const char *TAG = "am2301b i2c sensor";


uint8_t am2301b_init(void)
{
    int ret_val;
    uint8_t status_byte = 0;
    
    /* Wait 100ms before communicating with sensor */
    vTaskDelay(100 / portTICK_PERIOD_MS);


    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, AM2301B_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, AM2301B_STATUS_BYTE, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret_val = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if (ret_val != ESP_OK)
    {
        ESP_LOGI(TAG, "error in %s: %d. Check sensor connection.", __func__, ret_val);
        return I2C_FAIL;
    }

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, AM2301B_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, &status_byte, 1, LAST_NACK_VAL);
    i2c_master_stop(cmd);
    ret_val = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if ((status_byte & AM2301B_STATUS_OK) != AM2301B_STATUS_OK)
    {
        ESP_LOGI(TAG, "AM2301B sensor problem");
        return I2C_FAIL;
    }
    else
    {
        ESP_LOGI(TAG, "AM2301B status OK.");
    }

    return I2C_OK;
}


uint8_t am2301b_trigger_measurement(char *rel_hum_buf, char *temp_buf)
{
    int ret_val;
    uint8_t data[7];
    uint8_t trigger[3] = {
        AM2301B_TRIG_MEAS1,
        AM2301B_TRIG_MEAS2,
        AM2301B_TRIG_MEAS3
        };

    /* Wait 10ms before sending another command */
    vTaskDelay(10 / portTICK_PERIOD_MS);

    /* Send the trigger measurement command */
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, AM2301B_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, trigger, 3, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret_val = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if (ret_val != ESP_OK)
    {
        ESP_LOGI(TAG, "Data request error");
        return I2C_FAIL;
    }

    vTaskDelay(80 / portTICK_PERIOD_MS);

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, AM2301B_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, data, 7, LAST_NACK_VAL);
    i2c_master_stop(cmd);
    ret_val = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if (ret_val != ESP_OK)
    {
        ESP_LOGI(TAG, "Data retrieval error");
        return I2C_FAIL;
    }

    /* Extract bytes from the buffer in correct order */
    uint32_t rhs_bytes = (data[1] << 12) | (data[2] << 4) | (data[3] >> 4);
    uint32_t tos_bytes = ((data[3] & 0xf) << 16) | (data[4] << 8) | data[5];

    /* Raw bytes from sensor -> type long */
    long long_rhs = (long)rhs_bytes;
    long long_tos = (long)tos_bytes;

    double rhs_value = long_rhs / 1048576.0 * 100.0;        // %
    double tos_value = long_tos / 1048576.0 * 200.0 - 50;   // Degrees C

    dbl2str(rhs_value, rel_hum_buf);
    dbl2str(tos_value, temp_buf);

    return I2C_OK;
}


