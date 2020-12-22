/**
 *
 * https://github.com/espressif/esp-idf/blob/v2.0/examples/peripherals/i2c/main/i2c_test.c
 *
 *
 *
 *
 *
 */
#include <driver/i2c.h>

#include "i2c.h"
//peter



#define READ_BIT I2C_MASTER_READ /*!< I2C master read */
#define ACK_VAL 0x0 /*!< I2C ack value */
#define NACK_VAL 0x1 /*!< I2C nack value */

#define I2C_MASTER_NUM I2C_NUM_1   /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000 // I2C master clock frequency


/**
 * @brief i2c master initialization
 */
esp_err_t i2c_master_init(gpio_num_t sda, gpio_num_t scl)
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda;
    conf.scl_io_num = scl;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(I2C_MASTER_NUM, &conf);

	return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}


esp_err_t i2c_master_write_slave(uint8_t addr, uint8_t* data_wr, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( addr << 1 ) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data_wr, size, true);
    i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_PERIOD_MS);

    i2c_cmd_link_delete(cmd);
    return ret;
}


/**
 * @brief test code to read esp-i2c-slave
 *        We need to fill the buffer of esp slave device, then master can read them out.
 *
 * _______________________________________________________________________________________
 * | start | slave_addr + rd_bit +ack | read n-1 bytes + ack | read 1 byte + nack | stop |
 * --------|--------------------------|----------------------|--------------------|------|
 *
 */
esp_err_t i2c_master_read_slave(i2c_port_t addr, uint8_t* data_rd, size_t size)
{
	if (size == 0)
		return ESP_OK;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( addr << 1 ) | READ_BIT, true);
	if (size > 1) {
		i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
	}
	i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_PERIOD_MS);

	i2c_cmd_link_delete(cmd);

	return ret;
}


esp_err_t i2c_master_scan(i2c_port_t addr)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_PERIOD_MS);

	i2c_cmd_link_delete(cmd);

	return ret;
}
