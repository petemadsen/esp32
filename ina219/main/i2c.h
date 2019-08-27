#ifndef PROJECT_I2C_H
#define PROJECT_I2C_H


#include <driver/i2c.h>


esp_err_t i2c_master_init(gpio_num_t sda, gpio_num_t scl);


esp_err_t i2c_master_write_slave(uint8_t addr, uint8_t* data_wr, size_t size);


esp_err_t i2c_master_read_slave(i2c_port_t addr, uint8_t* data_rd, size_t size);


#endif
