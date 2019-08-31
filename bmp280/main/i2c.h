#ifndef PROJECT_I2C_H
#define PROJECT_I2C_H



#include <driver/i2c.h>


esp_err_t i2c_master_init(gpio_num_t sda, gpio_num_t scl);
esp_err_t i2c_master_write_slave(uint8_t addr, uint8_t* data_wr, size_t size);
esp_err_t i2c_master_read_slave(uint8_t addr, uint8_t* data_rd, size_t size);
esp_err_t i2c_master_scan(uint8_t addr);

uint8_t m_i2c_read8(uint8_t addr, uint8_t reg);
uint16_t m_i2c_read16(uint8_t addr, uint8_t reg);
uint32_t m_i2c_read24(uint8_t addr, uint8_t reg);


#endif
