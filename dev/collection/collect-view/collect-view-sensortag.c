#include "collect-view.h"
#include "button-sensor.h"
#include "batmon-sensor.h"
#include "board-peripherals.h"

#include "collect-view-sensortag.h"

enum {
  SENSOR1,
  SENSOR2,
};

/*---------------------------------------------------------------------------*/
void
collect_view_arch_read_sensors(struct collect_view_data_msg *msg)
{
	SENSORS_ACTIVATE(batmon_sensor);
	SENSORS_ACTIVATE(bmp_280_sensor);
	SENSORS_ACTIVATE(opt_3001_sensor);
	SENSORS_ACTIVATE(hdc_1000_sensor);
	SENSORS_ACTIVATE(tmp_007_sensor);
	mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ALL);

	msg->sensors[BATTERY_VOLTAGE_SENSOR] = batmon_sensor.value(BATMON_SENSOR_TYPE_VOLT);
  msg->sensors[BATTERY_INDICATOR] = 0;
  msg->sensors[LIGHT1_SENSOR] = opt_3001_sensor.value(0);
  msg->sensors[LIGHT2_SENSOR] = 0;
  msg->sensors[TEMP_SENSOR] = hdc_1000_sensor.value(HDC_1000_SENSOR_TYPE_TEMP);;
  msg->sensors[HUMIDITY_SENSOR] = hdc_1000_sensor.value(HDC_1000_SENSOR_TYPE_HUMIDITY);


  msg->sensors[SENSOR1] = 0;
  msg->sensors[SENSOR2] = 0;
}
/*---------------------------------------------------------------------------*/
