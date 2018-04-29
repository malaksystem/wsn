#ifndef COLLECT_SENSOR_H
#define COLLECT_SENSOR_H

#include "collect.h"


enum {
  CPU_TEMP_SENSOR,
  BATTERY_VOLTAGE_SENSOR,
  PRESSURE_SENSOR,
  AMB_TEMP_SENSOR,
  OBJ_TEMP_SENSOR,
  HDC_TEMP_SENSOR,
  HDC_HUMIDITY_SENSOR,
  LIGHT_SENSOR,
  GYRO_X_SENSOR,
  GYRO_Y_SENSOR,
  GYRO_Z_SENSOR,
  ACC_X_SENSOR,
  ACC_Y_SENSOR,
  ACC_Z_SENSOR,
  SENSOR_NUM,
};

typedef struct data_xyz
{
  int16_t x;
  int16_t y;
  int16_t z;
}data_xyz;


typedef struct sensor_data_struct
{
  int16_t bat_temp;
  int16_t bat_vol;

  int16_t pressure;
  int16_t bar_temp;

  int16_t amb_temp;
  int16_t obj_temp;

  int16_t hdc_temp;
  int16_t hdc_humidity;

  int16_t light;

  data_xyz gyro;
  data_xyz acc;
}sensor_data_type;



void init_sensors(void);
void active_sensors(void);
void read_sensors(struct sensor_data_struct *data);
void collect_arch_read_sensors(struct collect_data_msg *msg);

#endif /* COLLECT_SENSOR_H */
