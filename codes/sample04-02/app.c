#include "app.h"
#include "util.h"

const int touch_sensor = EV3_PORT_1, color_sensor = EV3_PORT_3,
touch_sensor2 = EV3_PORT_2, sonar_sensor = EV3_PORT_4;
const int left_motor = EV3_PORT_A, right_motor = EV3_PORT_C;

void cyc0(intptr_t exinf) {
  act_tsk(MAIN_TASK);
}

// SimpleTimer
SYSTIM timer_start_count;
SYSTIM timer_timedout_count;
SYSTIM timer_current_count;

void timer_start(int delay_ms) {
  get_tim(&timer_start_count);
  timer_timedout_count
    = timer_start_count + (SYSTIM)delay_ms;
}

void timer_stop(void) {
  timer_start_count = (SYSTIM)0;
}

int timer_is_started(void) {
  return ( timer_start_count > (SYSTIM)0 );
}

int timer_is_timedout(void) {
  if( timer_start_count <= 0 ) {
    return 0;
  }

  get_tim(&timer_current_count);
  if( timer_current_count
      >= timer_timedout_count ) {
    return 1;
  } else {
    return 0;
  }
}

void horn_warning(void) {
  const int duration = 100;
  ev3_speaker_set_volume(10);

  for(int i = 0; i < 4; i++) {
    ev3_speaker_play_tone(NOTE_E4, duration); /* ミ */
    dly_tsk(duration);
    ev3_speaker_play_tone(NOTE_C4, duration); /* ド */
    dly_tsk(duration);
  }
}

void horn_confirmation(void) {
  const int duration = 100;
  ev3_speaker_set_volume(10);

  for(int i = 0; i < 2; i++) {
    ev3_speaker_play_tone(NOTE_C4, duration); /* ド */
    dly_tsk(duration);
    ev3_speaker_play_tone(NOTE_F4, duration); /* ファ */
    dly_tsk(duration);
  }
}

void horn_arrived(void) {
  const int duration = 100;
  ev3_speaker_set_volume(10);

  ev3_speaker_play_tone(NOTE_F4, duration); /* ファ */
  dly_tsk(duration * 3);
  ev3_speaker_play_tone(NOTE_F4, duration * 3 ); /* ファ */
  dly_tsk(duration);
}

#define DR_POWER 20
int dr_power = DR_POWER;

void driver_turn_left(void) {
  ev3_motor_set_power(left_motor, 0);
  ev3_motor_set_power(right_motor, dr_power);
}

void driver_turn_right(void) {
  ev3_motor_set_power(left_motor, dr_power);
  ev3_motor_set_power(right_motor, 0);
}

void driver_stop(void) {
  ev3_motor_set_power(left_motor, 0);
  ev3_motor_set_power(right_motor, 0);
}

#define LINEMON_THRESHOLD 20
int lm_threshold = LINEMON_THRESHOLD;

int linemon_is_online(void) {
  return( ev3_color_sensor_get_reflect(color_sensor)
    < LINEMON_THRESHOLD );
}

void tracer_stop(void) {
  driver_stop();  
}

void tracer_run(void) {  
  if( linemon_is_online() ) {
    driver_turn_left();
  } else {
    driver_turn_right();
  }
}

int bumper_is_pushed(void) {
  return ev3_touch_sensor_is_pressed(touch_sensor);
}

int carrier_cargo_is_loaded(void) {
  return ev3_touch_sensor_is_pressed(touch_sensor2);
}

#define WD_THRESHOLD 10
int wd_threshold = WD_THRESHOLD;
int walldetector_is_detected(void) { // cyc0 を100msにした
  return ev3_ultrasonic_sensor_get_distance(sonar_sensor) < wd_threshold;
}

typedef enum { 
  P_WAIT_FOR_LOADING, P_TRANSPORTING,
  P_TIMED_OUT, P_CARGO_SHIFTING,
  P_WAIT_FOR_UNLOADING, P_RETURNING, P_ARRIVED
} porter_state;

int p_state = P_WAIT_FOR_LOADING;

void porter_transport(void) {
  char buf[2] = " ";
  buf[0] ='0' + p_state; 
  msg_f( buf, 2 );
  switch(p_state) {
  case P_WAIT_FOR_LOADING:
    if(! timer_is_started() ) {
      timer_start(10000);
    }
    if (carrier_cargo_is_loaded()) {
      p_state = P_TRANSPORTING;
    }
    if (timer_is_timedout()) {
      p_state = P_TIMED_OUT;
    }
    if( p_state != P_WAIT_FOR_LOADING ) {
      timer_stop();
    }
    break;
  case P_TIMED_OUT:
    horn_confirmation();
    p_state = P_WAIT_FOR_LOADING;
    break;
  case P_TRANSPORTING:
    tracer_run();
    if (walldetector_is_detected()) {
      p_state = P_WAIT_FOR_UNLOADING;
      tracer_stop();
      horn_arrived();
    }
    if (! carrier_cargo_is_loaded()) {
      p_state = P_CARGO_SHIFTING;
    }
    if( p_state != P_TRANSPORTING ) {
      tracer_stop();
    }
    break;
  case P_CARGO_SHIFTING:
    if(! timer_is_started() ) {
      horn_warning();
      timer_start(5000);
    }
    if (carrier_cargo_is_loaded()) {
      p_state = P_TRANSPORTING;
      timer_stop();
    }
    if (timer_is_timedout()) {
      p_state = P_CARGO_SHIFTING;
      timer_stop();
    }
    break;
  case P_WAIT_FOR_UNLOADING:
    if (! carrier_cargo_is_loaded()) {
      p_state = P_RETURNING;
    }
    break;
  case P_RETURNING:
    tracer_run();
    if (bumper_is_pushed()) {
      p_state = P_ARRIVED;
      horn_arrived();
    }
    if( p_state != P_RETURNING ) {
      tracer_stop();
    }
    break;
  case P_ARRIVED:
    tracer_stop();
    break;
  default:
    break;
  } 
}

void main_task(intptr_t unused) {
  static int is_initialized = false;
  if(! is_initialized ) {
    is_initialized = true;
    init_f("sample04-02"); 
    ev3_motor_config(left_motor, LARGE_MOTOR);
    ev3_motor_config(right_motor, LARGE_MOTOR);
    ev3_sensor_config(touch_sensor, TOUCH_SENSOR);
    ev3_sensor_config(touch_sensor2, TOUCH_SENSOR);    
    ev3_sensor_config(color_sensor, COLOR_SENSOR);
    ev3_sensor_config(sonar_sensor, ULTRASONIC_SENSOR);    
  }
  
  porter_transport();

  ext_tsk();
}
