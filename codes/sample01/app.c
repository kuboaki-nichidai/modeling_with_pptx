#include "app.h"
#include "util.h"

void main_task(intptr_t unused) { // mainä÷êîÇÃÇÊÇ§Ç»Ç‡ÇÃ
  init_f("sample01"); 
  ev3_motor_config(EV3_PORT_A, LARGE_MOTOR);
  ev3_motor_config(EV3_PORT_C, LARGE_MOTOR);
  ev3_sensor_config(EV3_PORT_1, TOUCH_SENSOR);
  
  ev3_motor_set_power(EV3_PORT_A, 40);
  ev3_motor_set_power(EV3_PORT_C, 40);

  while(!ev3_touch_sensor_is_pressed(EV3_PORT_1));

  ev3_motor_set_power(EV3_PORT_A, -40);
  ev3_motor_set_power(EV3_PORT_C, -40);

  tslp_tsk(2000); 

  ev3_motor_stop(EV3_PORT_A, false);
  ev3_motor_stop(EV3_PORT_C, false);

  ext_tsk();
}
