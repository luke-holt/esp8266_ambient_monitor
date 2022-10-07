#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"

#include "esp_log.h"

#include "i2c_helpers.h"


uint8_t i2c_write_buf(uint8_t address, uint8_t *tx_buf, size_t buf_len)
{
    int ret_val;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, tx_buf, buf_len, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    ret_val = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, 1000 / portTICK_RATE_MS);

    i2c_cmd_link_delete(cmd);

    return ret_val;
}


uint8_t i2c_write_byte(uint8_t addr, uint8_t reg_addr, uint8_t reg_cmd)
{
    int ret_val;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_cmd, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret_val = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret_val;
}


uint8_t i2c_read_byte(uint8_t address, uint8_t reg_addr, uint8_t *rx_reg)
{
    int ret_val;

    /* Create command link */
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    /* Start signal */
    i2c_master_start(cmd);

    /* Device address with Write bit followed by register address */
    i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);

    /* Repeated start bit */
    i2c_master_start(cmd);

    /* Device address with Read bit followed by read command */
    i2c_master_write_byte(cmd, address << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, rx_reg, LAST_NACK_VAL);

    /* Stop signal */
    i2c_master_stop(cmd);

    /* Send queued commands */
    ret_val = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, 1000 / portTICK_RATE_MS);

    /* Free command link */
    i2c_cmd_link_delete(cmd);

    return ret_val;
}


void dbl2str(const double d, char *buf)
{
    int whole, dec;
    
    whole = (int)d;
    dec = (d - whole) * 1000000;

    sprintf(buf, "%d.%06d", whole, dec);
}
