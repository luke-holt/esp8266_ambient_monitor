
#define I2C_MASTER_PORT         0
#define I2C_MASTER_SDA_IO       CONFIG_I2C_MASTER_SDA_IO
#define I2C_MASTER_SCL_IO       CONFIG_I2C_MASTER_SCL_IO
#define I2C_MASTER_TIMEOUT_MS   1000

#define WRITE_BIT               I2C_MASTER_WRITE
#define READ_BIT                I2C_MASTER_READ
#define ACK_CHECK_EN            0x1
#define ACK_CHECK_DIS           0x0
#define ACK_VAL                 0x0
#define NACK_VAL                0x1
#define LAST_NACK_VAL           0x2

#define I2C_OK                  0
#define I2C_FAIL                -1

#define MAX_RETURN_BUF_SIZE     100


uint8_t i2c_write_buf(uint8_t address, uint8_t *tx_buf, size_t buf_len);
uint8_t i2c_write_byte(uint8_t addr, uint8_t reg_addr, uint8_t reg_cmd);
uint8_t i2c_read_byte(uint8_t address, uint8_t reg_addr, uint8_t *rx_reg);
void dbl2str(const double d, char *buf);