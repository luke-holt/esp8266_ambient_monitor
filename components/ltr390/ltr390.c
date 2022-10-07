#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "esp_log.h"

#include "driver/i2c.h"

#include "i2c_helpers.h"
#include "ltr390.h"


static const char *TAG = "ltr390 i2c sensor";


uint8_t ltr390_trigger_measurement(char *als_buf, char *uvs_buf)
{
    int ret_val;

    uint8_t als_data0 = 0;
    uint8_t als_data1 = 0;
    uint8_t als_data2 = 0;
    uint8_t uvs_data0 = 0;
    uint8_t uvs_data1 = 0;
    uint8_t uvs_data2 = 0;

    /* Enable sensor in ALS mode */
    ret_val = i2c_write_byte(LTR390_ADDR, LTR390_MAIN_CTRL, MAIN_CTRL_EN | MAIN_CTRL_MODE_ALS);
    if (ret_val != I2C_OK)
    {
        ESP_LOGI(TAG, "error in %s: %d. Check sensor connection.", __func__, ret_val);
        return I2C_FAIL;
    }

    /* Wait for sensor */
    vTaskDelay(200 / portTICK_PERIOD_MS);

    /* Read values from ALS data registers */
    ret_val = i2c_read_byte(LTR390_ADDR, LTR390_ALS_DATA0, &als_data0);
    if (ret_val != I2C_OK)
    {
        ESP_LOGI(TAG, "error in %s: %d. Check sensor connection.", __func__, ret_val);
        return I2C_FAIL;
    }

    ret_val = i2c_read_byte(LTR390_ADDR, LTR390_ALS_DATA1, &als_data1);
    if (ret_val != I2C_OK)
    {
        ESP_LOGI(TAG, "error in %s: %d. Check sensor connection.", __func__, ret_val);
        return I2C_FAIL;
    }

    ret_val = i2c_read_byte(LTR390_ADDR, LTR390_ALS_DATA2, &als_data2);
    if (ret_val != I2C_OK)
    {
        ESP_LOGI(TAG, "error in %s: %d. Check sensor connection.", __func__, ret_val);
        return I2C_FAIL;
    }


    /* Change sensor mode to UVS */
    ret_val = i2c_write_byte(LTR390_ADDR, LTR390_MAIN_CTRL, MAIN_CTRL_EN | MAIN_CTRL_MODE_UVS);
    if (ret_val != I2C_OK)
    {
        ESP_LOGI(TAG, "error in %s: %d. Check sensor connection.", __func__, ret_val);
        return I2C_FAIL;
    }

    /* Wait for sensor */
    vTaskDelay(200 / portTICK_PERIOD_MS);

    /* Read values from UVS data registers */
    ret_val = i2c_read_byte(LTR390_ADDR, LTR390_UVS_DATA0, &uvs_data0);
    if (ret_val != I2C_OK)
    {
        ESP_LOGI(TAG, "error in %s: %d. Check sensor connection.", __func__, ret_val);
        return I2C_FAIL;
    }

    ret_val = i2c_read_byte(LTR390_ADDR, LTR390_UVS_DATA1, &uvs_data1);
    if (ret_val != I2C_OK)
    {
        ESP_LOGI(TAG, "error in %s: %d. Check sensor connection.", __func__, ret_val);
        return I2C_FAIL;
    }

    ret_val = i2c_read_byte(LTR390_ADDR, LTR390_UVS_DATA2, &uvs_data2);
    if (ret_val != I2C_OK)
    {
        ESP_LOGI(TAG, "error in %s: %d. Check sensor connection.", __func__, ret_val);
        return I2C_FAIL;
    }

    /* Assemble bytes together */
    uint32_t als_bytes = als_data0 | (als_data1 << 8) | ((als_data2 & 0xf) << 16);
    uint32_t uvs_bytes = uvs_data0 | (uvs_data1 << 8) | ((uvs_data2 & 0xf) << 16);

    /* Convert bytes to double values */
    int gain = 1; // ALS gain
    float integration_val = 1.0; // 18-bit resolution, 100ms integration time
    int window_factor = 1; // No window
    int uv_sensitivity = 2300;

    long als_up = (long)(als_bytes * 1000000); // uint32_t -> long
    double als_value = (0.6 * als_up / (gain * integration_val) * window_factor) / 1000000;

    long uvs_up = (long)(uvs_bytes * 1000000); // uint32_t -> long
    double uvs_value = (uvs_up / uv_sensitivity * window_factor) / 1000000;
    
    /* Write payload strings */
    dbl2str(als_value, als_buf);
    dbl2str(uvs_value, uvs_buf);

    return I2C_OK;
}

