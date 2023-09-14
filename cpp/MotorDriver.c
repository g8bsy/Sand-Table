#include <Python.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "vec.c"
#include "bcm2835.h"

#ifdef __APPLE__
int bcm2835_init(void) { return 1; }
void bcm2835_gpio_write(uint8_t pin, uint8_t on) {}
void bcm2835_gpio_fsel(uint8_t pin, uint8_t mode) {}
uint8_t bcm2835_gpio_lev(uint8_t pin) { return 0; }
void bcm2835_gpio_set_pud(uint8_t pin, uint8_t pud) {}
#define HIGH 0x1
#define LOW 0x0
#endif

#define FOREACH_CB_TYPE(CB_TYPE) \
  CB_TYPE(TASK_START)            \
  CB_TYPE(TASK_COMPLETE)         \
  CB_TYPE(TASK_ERROR)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

enum CB_TYPE_ENUM
{
  FOREACH_CB_TYPE(GENERATE_ENUM)
};
typedef enum CB_TYPE_ENUM CB_TYPE;

static const char *CB_TYPE_STRING[] = {
    FOREACH_CB_TYPE(GENERATE_STRING)};

#define max(a, b) \
  ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a, b) \
  ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

pthread_mutex_t running_mutex;
int rot_dir_pin = 0, rot_step_pin = 0, rot_en_pin = 0, lin_dir_pin = 0, lin_step_pin = 0, lin_en_pin = 0, outer_limit_pin = 0, inner_limit_pin = 0, rotation_limit_pin = 0;
int rot_pos = 0, lin_pos = 0;
int steps_per_revolution = 0, steps_per_linear = 0;
volatile bool is_running = false;
volatile int speed = 100;

int lin_dir = 0, rot_dir = 0, rot_at_limit = 0, lin_at_limit = 0;

static PyObject *my_callback = NULL;

void * calibrate(void *t_id);

void save_pos(){

  FILE *fptr;
  fptr = fopen("integers", "w");

  if (fptr == NULL) {
    printf("Failed to create the file.\n");
    return;
  }

  putw(lin_pos, fptr);
  putw(steps_per_linear, fptr);
  putw(rot_pos, fptr);
  putw(steps_per_revolution, fptr);
  putw(speed, fptr);

  //printf("saved lin_pos=%d rot_pos=%d", lin_pos, rot_pos);

  fclose(fptr);

}

void load_pos(){
  FILE *fptr;
  fptr = fopen("integers", "r");

  if (fptr != NULL) {
    printf("File opened successfully!\n");
  }
  else {
    printf("Failed to open the file.\n");
    char* t_id = "init";
    calibrate(t_id);
    return;
  }
  
  lin_pos = getw(fptr);
  steps_per_linear = getw(fptr);
  rot_pos = getw(fptr);
  steps_per_revolution = getw(fptr);
  speed = getw(fptr);
  if(speed < 100 || speed > 500){
    speed = 250;
  }
  
  fclose(fptr);
  
  printf("loaded lin_pos=%d steps_per_linear=%d rot_pos=%d steps_per_revolution=%d speed=%d\n", lin_pos, steps_per_linear, rot_pos, steps_per_revolution, speed);
}

void callback_message(CB_TYPE type, const char *task_id, const char *msg)
{
  PyGILState_STATE gstate = PyGILState_Ensure();
  PyObject *arglist = Py_BuildValue("(sss)", CB_TYPE_STRING[type], task_id, msg);
  PyObject *result = PyObject_CallObject(my_callback, arglist);
  Py_DECREF(arglist);
  if (result != NULL)
    Py_DECREF(result);
  PyGILState_Release(gstate);
}

int is_zero(void)
{
  return 0;
}

int is_one(void)
{
  return 1;
}

int is_at_limit(void)
{

  int rot_limit_is_on_now = bcm2835_gpio_lev(rotation_limit_pin) == 0 ;

  if(!rot_at_limit && rot_limit_is_on_now)
  {
    //we've hit the rot limit pin
    rot_at_limit = 1;
  } else if(rot_at_limit && !rot_limit_is_on_now)
  {
    //we've released the rot limit pin
    rot_at_limit = 0;
  }

  if (bcm2835_gpio_lev(outer_limit_pin) == 0)
  {
    return 1;
  }
  if (bcm2835_gpio_lev(inner_limit_pin) == 0)
  {
    return -1;
  }
  return 0;
}

int is_at_inner_limit(void)
{

  if (bcm2835_gpio_lev(inner_limit_pin) == 0)
  {
    return -1;
  }
  return 0;
}

int is_at_outer_limit(void)
{

  if (bcm2835_gpio_lev(outer_limit_pin) == 0)
  {
    return 1;
  }  
  return 0;
}

int is_not_at_limit(void)
{
  if (is_at_limit() == 0)
  {
    return 1;
  }
  return 0;
}

int is_at_rot_limit(void)
{
  if (bcm2835_gpio_lev(rotation_limit_pin) == 0)
  {
    return 1;
  }
  return 0;
}

int is_not_at_rot_limit(void)
{
  if (is_at_rot_limit() == 0)
  {
    return 1;
  }
  return 0;
}

void delayMicrosHard(unsigned int howLong)
{
  struct timeval tNow, tLong, tEnd;

  gettimeofday(&tNow, NULL);
  tLong.tv_sec = howLong / 1000000;
  tLong.tv_usec = howLong % 1000000;
  timeradd(&tNow, &tLong, &tEnd);

  while (timercmp(&tNow, &tEnd, <))
    gettimeofday(&tNow, NULL);
}

void delayMicros(unsigned int howLong)
{
  struct timespec sleeper;
  unsigned int uSecs = howLong % 1000000;
  unsigned int wSecs = howLong / 1000000;

  /**/ if (howLong == 0)
    return;
  else if (howLong < 100)
    delayMicrosHard(howLong);
  else
  {
    sleeper.tv_sec = wSecs;
    sleeper.tv_nsec = (long)(uSecs * 1000L);
    nanosleep(&sleeper, NULL);
  }
}

bool start_task(const char *task_id, const char *msg)
{
  if (pthread_mutex_trylock(&running_mutex) != 0)
  {
    callback_message(TASK_ERROR, task_id, msg);
    return false;
  }
  else
  {
    is_running = true;
    callback_message(TASK_START, task_id, msg);
    return true;
  }
}

void stop_task(const char *task_id, const char *msg)
{
  pthread_mutex_unlock(&running_mutex);
  is_running = false;
  callback_message(TASK_COMPLETE, task_id, msg);
}

int steps_with_speed(int rot_steps, int lin_steps, int delay, int (*checkLimit)(), int ramp_up, int ramp_down)
{

  int64_t abs_rot_steps = abs(rot_steps);
  int64_t abs_lin_steps = abs(lin_steps);

  int64_t max_steps = max(abs_rot_steps, abs_lin_steps);
  bool lin_is_max = abs_lin_steps == max_steps;
  bool rot_is_max = abs_rot_steps == max_steps;

  int64_t loop_time = delay * speed * 4;
  
  int64_t rot_delay = abs_rot_steps == 0 ? 0 : (rot_is_max ? loop_time : (abs_lin_steps * loop_time) / abs_rot_steps);
  int64_t lin_delay = abs_lin_steps == 0 ? 0 : (lin_is_max ? loop_time : (abs_rot_steps * loop_time) / abs_lin_steps);

  // printf("rot_steps=%d\n", rot_steps);
  // printf("lin_steps=%d\n", lin_steps);
  // printf("lin_is_max=%d\n", lin_is_max);
  // printf("rot_is_max=%d\n", rot_is_max);
  // printf("max_steps=%d\n", max_steps);
  // printf("loop_time=%d\n", loop_time);
  // printf("default_pulse_time=%d\n", default_pulse_time);
  // printf("lin_delay=%d\n", lin_delay);
  // printf("rot_delay=%d\n", rot_delay);

  bcm2835_gpio_write(rot_dir_pin, rot_steps < 0 ? HIGH : LOW);
  bcm2835_gpio_write(lin_dir_pin, lin_steps < 0 ? LOW : HIGH);

  uint64_t rot_count = 0;
  uint64_t lin_count = 0;

  uint64_t clk_count = 0;

  for (int i = 0; i < max_steps; i++)
  {

    if (!is_running)
    {
      printf("Exiting steps_with_speed as is_running=false\n");
      break;
    }

    int pulse_time = delay * speed;

    if(speed < 250) {
      if (i < ramp_up)
      {
        pulse_time *= (ramp_up - i);
      }
      else if (max_steps - i < ramp_down)
      {
        pulse_time *= (max_steps - i);
      }
    }

    // printf("clk_count=%d rot_count=%d lin_count=%d \n", clk_count, rot_count, lin_count);

    if (checkLimit() != 0)
    {
      delayMicros(100);
      if (checkLimit() != 0)
      {
        printf("checkLimit()=%d\n", checkLimit());
        break;
      }
    }
    

    if (clk_count >= rot_count && rot_steps != 0)
    {
      rot_count += rot_delay;
      bcm2835_gpio_write(rot_step_pin, HIGH);
      delayMicros(pulse_time);
      bcm2835_gpio_write(rot_step_pin, LOW);
      rot_pos += rot_steps > 0 ? 1 : -1;
    }
    else
    {
      delayMicros(pulse_time);
    }

    delayMicros(pulse_time);

    if (clk_count >= lin_count && lin_steps != 0)
    {
      // printf("lin_pulse_count=%d\n",++lin_pulse_count);
      lin_count += lin_delay;
      bcm2835_gpio_write(lin_step_pin, HIGH);
      delayMicros(pulse_time);
      bcm2835_gpio_write(lin_step_pin, LOW);
      lin_pos += lin_steps > 0 ? 1 : -1;
    }
    else
    {
      delayMicros(pulse_time);
    }

    delayMicros(pulse_time);

    clk_count += loop_time;
  }

  save_pos();

  return 0;
}

int planner_last_rot_steps;
int planner_last_lin_steps;
int planner_has_ramped_down = false;

//Run the previous line, but work out what it needs to do once it's done
//ie, ramp down or up. 
// Ramp up if the previous move ramped down
// ramp down if the next move is a reverseal
int plan_move(int rot_steps, int lin_steps){

    int ramp = 20;

    //Ramp down if either axis are gonna change direction
    bool should_ramp_down = ((rot_steps ^ planner_last_rot_steps) < 0)
                              || ((lin_steps ^ planner_last_rot_steps) < 0);

    // //If the rotation is much faster then the linear, don't care about linear direction changes
    if(abs(planner_last_rot_steps) / 2 > abs(planner_last_lin_steps)){
      should_ramp_down = (rot_steps ^ planner_last_rot_steps) < 0;
    }

    // //If the linear is much faster then the rotation, don't care about rotation direction changes
    if(abs(planner_last_lin_steps) / 2 > abs(planner_last_rot_steps)){
      should_ramp_down = (lin_steps ^ planner_last_lin_steps) < 0;
    }

    
    

  // bool should_ramp_down = (abs(rot_steps) + abs(planner_last_rot_steps) > rot_steps)
  //                         || (abs(lin_steps) + abs(planner_last_lin_steps) > lin_steps);
  int ret = steps_with_speed(planner_last_rot_steps, planner_last_lin_steps, 1, &is_at_limit, planner_has_ramped_down ? ramp : 0, should_ramp_down ? ramp : 0);

  planner_last_rot_steps = rot_steps;
  planner_last_lin_steps = lin_steps;
  planner_has_ramped_down = should_ramp_down;                        
  return ret;
}



struct steps_with_speed_locked_thread_args
{
  char *task_id;
  int rot_steps;
  int lin_steps;
  int delay;
  int (*checkLimit)();
  int ramp_up;
  int ramp_down;
};

void * steps_with_speed_locked(void *_args)
{
  struct steps_with_speed_locked_thread_args *args = (struct steps_with_speed_locked_thread_args *)_args;

  if (!start_task(args->task_id, "steps_with_speed_locked"))
  {
    free(args);
    return NULL;
  }

  printf("lin_pos=%d rot_pos=%d\n", lin_pos, rot_pos);

  bcm2835_gpio_write(rot_en_pin, LOW);
  bcm2835_gpio_write(lin_en_pin, LOW);
  steps_with_speed(args->rot_steps, args->lin_steps, args->delay, args->checkLimit, args->ramp_up, args->ramp_down);
  bcm2835_gpio_write(rot_en_pin, HIGH);
  bcm2835_gpio_write(lin_en_pin, HIGH);

  printf("lin_pos=%d rot_pos=%d\n", lin_pos, rot_pos);
  stop_task(args->task_id, "steps_with_speed_locked");
  free(args);
  pthread_exit(NULL);
  return NULL;
}

struct load_theta_rho_thread_args
{
  char *task_id;
  char *fname;
};

void * load_theta_rho(void *_args)
{

  struct load_theta_rho_thread_args *args = (struct load_theta_rho_thread_args *)_args;

  if (!start_task(args->task_id, "load_theta_rho"))
  {
    free(args);
    return NULL;
  }

  FILE *fp = fopen(args->fname, "r");
  if (fp == NULL)
  {
    perror("Unable to open file!");
    return  NULL;
  }

  bcm2835_gpio_write(rot_en_pin, LOW);
  bcm2835_gpio_write(lin_en_pin, LOW);

  delayMicros(1000);

  // Read lines using POSIX function getline
  // This code won't work on Windows
  char *line = NULL;
  size_t len = 0;

  rot_pos = abs(rot_pos % steps_per_revolution);
  
  // TODO - comment / uncomment
  int last_lin_pos = lin_pos;
  int last_rot_pos = abs(rot_pos % steps_per_revolution);

  printf("Start\n");

  // printf("steps_per_revolution=%d steps_per_linear=%d\n", steps_per_revolution, steps_per_linear);
  int i = 0;

  while (getline(&line, &len, fp) != -1)
  {

    
    // printf("%s\n", line);

    if (!is_running)
      break;

    i++;

    char *first_tok = strtok(line, " ");
    char *second_tok = strtok(NULL, " ");

    if (first_tok == NULL || second_tok == NULL)
    {
      printf("Skipping %s\n", line);
      continue;
    } 
    
    char *first_endptr;
    char *second_endptr;

    float theta = strtof(first_tok, &first_endptr);
    float rho = strtof(second_tok, &second_endptr);

    if (first_endptr == first_tok || second_endptr == second_tok)
    {
      printf("Skipping %s", line);
      continue;
    }

    int theta_coor = (int)(steps_per_revolution * theta / 6.28318531);
    int rho_coor = (int)(steps_per_linear * rho);

    int theta_steps = theta_coor - last_rot_pos;
    int rho_steps = rho_coor - last_lin_pos;

    
    printf("theta_steps=%d \t rho_steps=%d \t theta=%f \t rho=%f \t theta_coor=%d \t rho_coor=%d \t rot_pos=%d \t lin_pos=%d \t \n",
           theta_steps, rho_steps, theta, rho, theta_coor, rho_coor, rot_pos, lin_pos);

    if (plan_move(theta_steps, rho_steps) != 0)
//    if (steps_with_speed(theta_steps, rho_steps, 1, &is_at_limit, 0, 0) != 0)
    {
      printf("%s\n", line);
      printf("lin_pos=%d \t rot_pos=%d\n", lin_pos, rot_pos);
      break;
    }

    last_lin_pos += rho_steps;
    last_rot_pos += theta_steps;

  }

  //Clear the planner 
  plan_move(0, 0);

  bcm2835_gpio_write(rot_en_pin, HIGH);
  bcm2835_gpio_write(lin_en_pin, HIGH);

  // print_vec_movement(moves);
  printf("End\n");

  fclose(fp);
  free(line);
  stop_task(args->task_id, "load_theta_rho");
  free(args);

  rot_pos = abs(rot_pos % steps_per_revolution);

  return NULL;
}

void * calibrate(void *t_id)
{

  char *task_id = (char*)t_id;

  if (!start_task(task_id, "calibrate"))
  {
    free(task_id);
    return NULL;
  }

  int step_ramp = 20;

  bcm2835_gpio_write(rot_en_pin, LOW);
  bcm2835_gpio_write(lin_en_pin, LOW);

  delayMicros(1000);

  printf("lin_pos %d\n", lin_pos);

  lin_pos = 0;

  char descriptions[4][12] = {"Find Switch",
                              "Back Away 1",
                              "Creep Up",
                              "Back Away 2"};

  
  if(is_at_limit() == -1){
    printf("Moving away from inner switch");
    steps_with_speed(0, 1000, 2, &is_zero, step_ramp, step_ramp);
  } else if(is_at_limit() == 1){
    printf("Moving away from inner switch");
    steps_with_speed(0, -1000, 2, &is_zero, step_ramp, step_ramp);
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
  
  int cali_moves[5][3] = {
      {50000, 2, &is_at_limit},
      {-300, 20, &is_not_at_limit},
      {500, 20, &is_at_limit},
      {-300, 20, &is_not_at_limit},
      {-35, 20, &is_zero}};

  for (int i = 0; i < 4; i++)
  {
    if (is_running)
      printf("%s\n", descriptions[i]);
    steps_with_speed(0, cali_moves[i][0], cali_moves[i][1], cali_moves[i][2], step_ramp, step_ramp);
    if (is_running)
      printf("lin_pos=%d\n", lin_pos);
  }

  lin_pos = 0;

  for (int i = 0; i < 5; i++)
  {
    if (is_running)
      printf("%s\n", descriptions[i]);
    steps_with_speed(0, -cali_moves[i][0], cali_moves[i][1], cali_moves[i][2], step_ramp, step_ramp);
    if (is_running)
      printf("lin_pos=%d\n", lin_pos);
  }

#pragma GCC diagnostic pop

  steps_per_linear = abs(lin_pos);
  lin_pos = 0;

  if (is_running)
    printf("Calibrated, steps_per_linear=%d, steps_per_revolution=%d\n", steps_per_linear, steps_per_revolution);

  printf("Calibrating Rotation 1\n");
  steps_with_speed(-100000, 0, 20, &is_not_at_rot_limit, step_ramp, step_ramp);
  
  printf("Calibrating Rotation 2\n");
  steps_with_speed(100000, 0, 2, &is_at_rot_limit, step_ramp, step_ramp);
  if (is_running)
    delayMicros(100000);
  
  
  printf("Calibrating Rotation 2.1\n");
  steps_with_speed(-200, 0, 20, &is_zero, step_ramp, step_ramp);
  if (is_running)
    delayMicros(100000);

  printf("Calibrating Rotation 2.2\n");
  steps_with_speed(400, 0, 20, &is_at_rot_limit, step_ramp, step_ramp);
  
  rot_pos = 0;

  printf("Calibrating Rotation 3\n");
  steps_with_speed(100000, 0, 2, &is_not_at_rot_limit, step_ramp, 0);
  
  printf("Calibrating Rotation 4\n");
  steps_with_speed(100000, 0, 2, &is_at_rot_limit, 0, step_ramp);
  if (is_running)
    delayMicros(100000);

  printf("Calibrating Rotation 4.1\n");
  steps_with_speed(-200, 0, 20, &is_zero, step_ramp, step_ramp);
  if (is_running)
    delayMicros(100000);

  printf("Calibrating Rotation 4.2\n");
  steps_with_speed(400, 0, 20, &is_at_rot_limit, step_ramp, step_ramp);
  if (is_running)
    delayMicros(100000);

  steps_per_revolution = abs(rot_pos);
  
  rot_pos = 0;

  if (is_running)
    printf("Calibrated steps_per_revolution=%d\n", steps_per_revolution);

  bcm2835_gpio_write(rot_en_pin, HIGH);
  bcm2835_gpio_write(lin_en_pin, HIGH);

  stop_task(task_id, "calibrate");

  return  NULL;
}

//#pragma region PyMethod
static PyObject *py_run_file(PyObject *self, PyObject *args)
{

  if (pthread_mutex_trylock(&running_mutex) != 0)
  {
    printf("Already running\n");
    return PyLong_FromLong(1L);
  }
  else
  {
    pthread_mutex_unlock(&running_mutex);
  }

  pthread_t drive_thread;
  char *task_id;
  char *filename;

  if (!PyArg_ParseTuple(args, "ss", &task_id, &filename))
  {
    printf("Parse error\n");
    return NULL;
  }

  printf("Running file %s\n", filename);

  struct load_theta_rho_thread_args *thread_args = malloc(sizeof(struct load_theta_rho_thread_args));
  thread_args->fname = filename;
  thread_args->task_id = task_id;

  printf("Start Thread\n");
  pthread_create(&drive_thread, NULL, load_theta_rho, (void *)thread_args);
  printf("Thread\n");
  return PyLong_FromLong(0L);

  return PyLong_FromLong(0L);
}

static PyObject *py_stopmotors(PyObject *self, PyObject *args)
{
  printf("py_stopmotors _is_running %d\n", is_running);
  is_running = false;
  printf("py_stopmotors _is_running %d\n", is_running);
  return PyLong_FromLong(0L);
}

static PyObject *py_set_speed(PyObject *self, PyObject *args)
{
  PyArg_ParseTuple(args, "i", &speed);
  return PyLong_FromLong(0L);
}

static PyObject *py_get_speed(PyObject *self, PyObject *args)
{
  return PyLong_FromLong(speed);
}

static PyObject *py_init(PyObject *self, PyObject *args)
{

  if (!PyArg_ParseTuple(args, "iiiiiiiii",
                        &rot_dir_pin,
                        &rot_step_pin,
                        &rot_en_pin,
                        &lin_dir_pin,
                        &lin_step_pin,
                        &lin_en_pin,
                        &outer_limit_pin,
                        &inner_limit_pin,
                        &rotation_limit_pin))
  {
    return NULL;
  } 

  printf("rot_dir_pin=%d \nrot_step_pin=%d \nrot_en_pin=%d \nlin_dir_pin=%d \nlin_step_pin=%d \nlin_en_pin=%d \nouter_limit_pin=%d \ninner_limit_pin=%d\n",
         rot_dir_pin, rot_step_pin, rot_en_pin, lin_dir_pin, lin_step_pin, lin_en_pin, outer_limit_pin, inner_limit_pin);

  if (!bcm2835_init())
    return 0;

  bcm2835_gpio_write(rot_en_pin, HIGH);
  bcm2835_gpio_write(lin_en_pin, HIGH);

  bcm2835_gpio_fsel(rot_dir_pin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(rot_step_pin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(rot_en_pin, BCM2835_GPIO_FSEL_OUTP);

  bcm2835_gpio_fsel(lin_dir_pin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(lin_step_pin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(lin_en_pin, BCM2835_GPIO_FSEL_OUTP);

  bcm2835_gpio_fsel(inner_limit_pin, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_set_pud(inner_limit_pin, BCM2835_GPIO_PUD_UP);

  bcm2835_gpio_fsel(outer_limit_pin, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_set_pud(outer_limit_pin, BCM2835_GPIO_PUD_UP);
  
  bcm2835_gpio_fsel(rotation_limit_pin, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_set_pud(rotation_limit_pin, BCM2835_GPIO_PUD_UP);

  printf("rot_step_pin=%d, rot_dir_pin=%d, rot_en_pin=%d\n", rot_step_pin, rot_dir_pin, rot_en_pin);

  load_pos();

  return PyLong_FromLong(0L);
}

static PyObject *py_steps(PyObject *self, PyObject *args)
{

  if (pthread_mutex_trylock(&running_mutex) != 0)
  {
    printf("Already running\n");
    return PyLong_FromLong(1L);
  }
  else
  {
    pthread_mutex_unlock(&running_mutex);
  }

  int lin_steps = 0, rot_steps = 0, delay = 0;
  char *task_id;

  if (!PyArg_ParseTuple(args, "siii",
                        &task_id,
                        &lin_steps,
                        &rot_steps,
                        &delay))
  {
    return NULL;
  }

  struct steps_with_speed_locked_thread_args *thread_args = malloc(sizeof(struct steps_with_speed_locked_thread_args));
  thread_args->task_id = task_id;
  thread_args->rot_steps = rot_steps;
  thread_args->lin_steps = lin_steps;
  thread_args->delay = delay;
  thread_args->checkLimit = &is_at_limit;
  thread_args->ramp_up = 20;
  thread_args->ramp_down = 20;

  pthread_t drive_thread;
  printf("Start Thread\n");
  pthread_create(&drive_thread, NULL, steps_with_speed_locked, thread_args);

  return PyLong_FromLong(0L);
}

static PyObject *py_calibrate(PyObject *self, PyObject *args)
{

  if (pthread_mutex_trylock(&running_mutex) != 0)
  {
    printf("Already running\n");
    return PyLong_FromLong(1L);
  }
  else
  {
    pthread_mutex_unlock(&running_mutex);
  }

  char *task_id;

  if (!PyArg_ParseTuple(args, "s",
                        &task_id))
  {
    return NULL;
  }

  pthread_t drive_thread;
  printf("Start Thread\n");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
  pthread_create(&drive_thread, NULL, calibrate, task_id);
#pragma GCC diagnostic pop  
  return PyLong_FromLong(0L);
}

static PyObject *py_set_callback(PyObject *dummy, PyObject *args)
{
  PyObject *result = NULL;
  PyObject *temp;

  if (PyArg_ParseTuple(args, "O:set_callback", &temp))
  {
    if (!PyCallable_Check(temp))
    {
      PyErr_SetString(PyExc_TypeError, "parameter must be callable");
      return NULL;
    }
    Py_XINCREF(temp);        /* Add a reference to new callback */
    Py_XDECREF(my_callback); /* Dispose of previous callback */
    my_callback = temp;      /* Remember new callback */
    /* Boilerplate to return "None" */
    Py_INCREF(Py_None);
    result = Py_None;
  }
  return result;
}
//#pragma end region PyMethod
static PyMethodDef DrivingMethods[] = {
    {"init", py_init, METH_VARARGS, "Function for initialisation"},
    {"calibrate", py_calibrate, METH_VARARGS, "Function for calibration"},
    {"steps", py_steps, METH_VARARGS, "Function to move"},
    {"run_file", py_run_file, METH_VARARGS, "Function to move"},
    {"stopmotors", py_stopmotors, METH_VARARGS, "Function for driving motor"},
    {"set_speed", py_set_speed, METH_VARARGS, "Function for Setting speed"},
    {"get_speed", py_get_speed, METH_VARARGS, "Function for Setting speed"},
    {"set_callback", py_set_callback, METH_VARARGS, "Function for Setting completion callback"},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef motordrivermodule = {
    PyModuleDef_HEAD_INIT,
    "MotorDriver", // module name
    "C library for driving motors",
    -1,
    DrivingMethods};

PyMODINIT_FUNC PyInit_MotorDriver(void)
{
  return PyModule_Create(&motordrivermodule);
};
