#include "collect.h"
#include "button-sensor.h"
#include "batmon-sensor.h"
#include "board-peripherals.h"
#include "dev/leds.h"
#include "sys/energest.h"

#include "collect-sensor.h"
#include <stdio.h>
#include "sys/log.h"


#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO



#define DEBUG 0




sensor_data_type sensor_data;

/*---------------------------------------------------------------------------*/

void
collect_arch_read_sensors(struct collect_data_msg *msg)
{
  msg->sensors[CPU_TEMP_SENSOR] = sensor_data.bat_temp;
  msg->sensors[BATTERY_VOLTAGE_SENSOR] = sensor_data.bat_vol;
  msg->sensors[PRESSURE_SENSOR] = sensor_data.pressure;
  msg->sensors[AMB_TEMP_SENSOR] = sensor_data.amb_temp;
  msg->sensors[OBJ_TEMP_SENSOR] = sensor_data.obj_temp;
  msg->sensors[HDC_TEMP_SENSOR] = sensor_data.hdc_temp;
  msg->sensors[HDC_HUMIDITY_SENSOR] = sensor_data.hdc_humidity;
  msg->sensors[LIGHT_SENSOR] = sensor_data.light;
  msg->sensors[GYRO_X_SENSOR] = sensor_data.gyro.x;
  msg->sensors[GYRO_Y_SENSOR] = sensor_data.gyro.y;
  msg->sensors[GYRO_Z_SENSOR] = sensor_data.gyro.z;
  msg->sensors[ACC_X_SENSOR] = sensor_data.acc.x;
  msg->sensors[ACC_Y_SENSOR] = sensor_data.acc.y;
  msg->sensors[ACC_Z_SENSOR] = sensor_data.acc.z;

#if DEBUG
  uint8_t i;
  printf("sensor value: ");
  for(i = 0; i < SENSOR_NUM; i++){
    printf("%d ", msg->sensors[i]);
  }
  printf("\n");
#endif
}


void
read_sensors(struct sensor_data_struct *data)
{
  int value;

  ENERGEST_ON(ENERGEST_TYPE_SENSOR);
/*---------------------------------------------------------------------------*/
  data->bat_temp = batmon_sensor.value(BATMON_SENSOR_TYPE_TEMP);;                        //C
  data->bat_vol = batmon_sensor.value(BATMON_SENSOR_TYPE_VOLT);           //(value * 125) >> 5; in mV
/*---------------------------------------------------------------------------*/
  value = bmp_280_sensor.value(BMP_280_SENSOR_TYPE_PRESS);
  if(value != CC26XX_SENSOR_READING_ERROR) {
    data->pressure = value;          
  } 
/*---------------------------------------------------------------------------*/
  value = tmp_007_sensor.value(TMP_007_SENSOR_TYPE_ALL);

  if(value != CC26XX_SENSOR_READING_ERROR) {
    value = tmp_007_sensor.value(TMP_007_SENSOR_TYPE_AMBIENT);
    data->amb_temp = value;

    value = tmp_007_sensor.value(TMP_007_SENSOR_TYPE_OBJECT);
    data->obj_temp = value;
  }
/*---------------------------------------------------------------------------*/
  value = hdc_1000_sensor.value(HDC_1000_SENSOR_TYPE_TEMP);
  if(value != CC26XX_SENSOR_READING_ERROR) {
    data->hdc_temp = value;
  } 

  value = hdc_1000_sensor.value(HDC_1000_SENSOR_TYPE_HUMIDITY);
  if(value != CC26XX_SENSOR_READING_ERROR) {
    data->hdc_humidity = value;        
  } 
/*---------------------------------------------------------------------------*/
   value = opt_3001_sensor.value(0);
  if(value != CC26XX_SENSOR_READING_ERROR) {
    data->light =  value;
  } 
/*---------------------------------------------------------------------------*/
  value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
  if(value >= 0){
    data->gyro.x = value;        
  }

  value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Y);
  if(value >= 0){
    data->gyro.y = value;
  }

  value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Z);
  if(value >= 0){
    data->gyro.z = value;
  }

  value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
  if(value >= 0){
    data->acc.x = value;        
  }

  value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
  if(value >= 0){
    data->acc.y = value;
  }

  value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);
  if(value >= 0){
    data->acc.z = value;
  }

  ENERGEST_OFF(ENERGEST_TYPE_SENSOR);
/*---------------------------------------------------------------------------*/
}
/*---------------------------------------------------------------------------*/
void
active_sensors(void)
{
  SENSORS_ACTIVATE(hdc_1000_sensor);
  SENSORS_ACTIVATE(opt_3001_sensor);
  SENSORS_ACTIVATE(bmp_280_sensor);
}

static void keep_active_sensor_init(void)
{
  SENSORS_ACTIVATE(batmon_sensor);
  SENSORS_ACTIVATE(tmp_007_sensor);
  mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ALL);
}
/*---------------------------------------------------------------------------*/

void init_sensors(void)
{
  active_sensors();
  keep_active_sensor_init();
}
/*---------------------------------------------------------------------------*/

