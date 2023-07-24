#include <Python.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "pthread_barrier.c"
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
int rot_pos = -1, lin_pos = -1, inner_to_center = -1, outer_to_max = -1;
int steps_per_revolution = -1, steps_per_linear = -1;
volatile bool is_running = false;

#define CLK_SPEED 40
#define CLK_PULSE CLK_SPEED / 4

#define TIME_MULT CLK_SPEED * 1000

int is_zero(void){
  return 0;
}
int is_at_limit(void)
{

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

int is_not_at_limit(void){
  if(is_at_limit() == 0){
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

int is_not_at_rot_limit(void){
  if(is_at_rot_limit() == 0){
    return 1;
  }
  return 0;
}


int steps_with_speed(int rot_steps, int lin_steps, int delay, int (*checkLimit)() , int ramp_up, int ramp_down)
{

  int abs_rot_steps = abs(rot_steps);
  int abs_lin_steps = abs(lin_steps);

  int max_steps = max(abs_rot_steps, abs_lin_steps);
  bool lin_is_max = abs_lin_steps == max_steps;
  bool rot_is_max = abs_rot_steps == max_steps;
  
  int loop_time = delay * CLK_SPEED;
  int default_pulse_time = loop_time / 4;

  int rot_delay = abs_rot_steps == 0 ? 0 : (rot_is_max ? loop_time : (abs_lin_steps * loop_time) / abs_rot_steps);
  int lin_delay = abs_lin_steps == 0 ? 0 : (lin_is_max ? loop_time : (abs_rot_steps * loop_time) / abs_lin_steps);

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
  bcm2835_gpio_write(lin_dir_pin, lin_steps < 0 ? HIGH : LOW);
  
  int rot_count = 0;
  int lin_count = 0;

  int clk_count = 0;

  for (int i = 0; i < max_steps; i++)
  {

    if(!is_running){
      printf("Exiting steps_with_speed as is_running=false");
      break;

    }

    int pulse_time = default_pulse_time;

    if (i < ramp_up)
    {
      pulse_time *= (ramp_up - i);
    }
    else if (max_steps - i < ramp_down)
    {
      pulse_time *= (max_steps - i);
    }

    //printf("clk_count=%d rot_count=%d lin_count=%d \n", clk_count, rot_count, lin_count);

    if (checkLimit() != 0)
    {
      printf("checkLimit()=%d\n", checkLimit());
      return checkLimit();
    }

    if (clk_count >= rot_count && rot_steps != 0)
    {
      rot_count += rot_delay;
      bcm2835_gpio_write(rot_step_pin, HIGH);
      usleep(pulse_time);
      bcm2835_gpio_write(rot_step_pin, LOW);
      rot_pos += rot_steps > 0 ? 1 : -1;
      
    }
    else
    {
      usleep(pulse_time);
    }

    usleep(pulse_time);

    if (clk_count >= lin_count && lin_steps != 0)
    {
      //printf("lin_pulse_count=%d\n",++lin_pulse_count);
      lin_count += lin_delay;
      bcm2835_gpio_write(lin_step_pin, HIGH);
      usleep(pulse_time);
      bcm2835_gpio_write(lin_step_pin, LOW);
      lin_pos += lin_steps > 0 ? 1 : -1;
    }
    else
    {
      usleep(pulse_time);
    }

    usleep(pulse_time);

    clk_count += loop_time;
  }

  return 0;
}

void load_theta_rho(char *fname)
{

  is_running = true;

  FILE *fp = fopen(fname, "r");
  if (fp == NULL)
  {
    perror("Unable to open file!");
    return;
  }

    bcm2835_gpio_write(rot_en_pin, LOW);
    bcm2835_gpio_write(lin_en_pin, LOW);


  // Read lines using POSIX function getline
  // This code won't work on Windows
  char *line = NULL;
  size_t len = 0;

  // TODO - comment / uncomment
   int last_rho_coor = lin_pos;
   int last_theta_coor = abs(rot_pos % steps_per_revolution);

  // int last_theta_coor = 0;
  // int last_rho_coor = lin_pos;

  printf("Start\n");

  // printf("steps_per_revolution=%d steps_per_linear=%d\n", steps_per_revolution, steps_per_linear);
  int i = 0;
  while (getline(&line, &len, fp) != -1)
  {

    if(!is_running)
      break;

    i++;

    float theta = atof(strtok(line, " "));
    float rho = atof(strtok(NULL, " "));
    int theta_coor = (int)(steps_per_revolution * theta / 6.28318531);
    int rho_coor = (int)(steps_per_linear * rho);

    int theta_steps = theta_coor - rot_pos;
    int rho_steps = rho_coor - lin_pos;

    printf("theta_steps=%d \t rho_steps=%d last_theta_coor=%d \t last_rho_coor=%d \t theta=%f \t rho=%f \t theta_coor=%d \t rho_coor=%d \t rot_pos=%d \t lin_pos=%d \t \n",
        theta_steps, rho_steps, last_theta_coor, last_rho_coor, theta, rho, theta_coor, rho_coor, rot_pos, lin_pos);

    if(steps_with_speed(theta_steps, rho_steps, 1, &is_at_limit, 10, 10) != 0){
      printf("%s\n", line);
      printf("lin_pos=%d \t rot_pos=%d\n", lin_pos, rot_pos);
      break;
    }

    last_theta_coor = theta_coor;
    last_rho_coor = rho_coor;

  }

  bcm2835_gpio_write(rot_en_pin, HIGH);
  bcm2835_gpio_write(lin_en_pin, HIGH);


  // print_vec_movement(moves);
  printf("End\n");

  fclose(fp);
  free(line);

  return;
}

static PyObject *py_run_file(PyObject *self, PyObject *args)
{

  pthread_t drive_thread;
  char *filename;

  if (!PyArg_ParseTuple(args, "s", &filename))
  {
    printf("Parse error\n");
    return NULL;
  }

  printf("Start Thread\n");
  pthread_create(&drive_thread, NULL, load_theta_rho, (void *)filename);
  printf("Thread\n");
  return PyLong_FromLong(0L);

  //load_theta_rho(filename);

  return PyLong_FromLong(0L);
}

// static bool set_running(bool running){

//   if(running){
//     if (pthread_mutex_trylock(&running_mutex) != 0)
//     {
//       printf("Already running\n");
//       return false;
//     }
//     else
//     {
//       is_running = true;
//       return true;
//     }
//   } else {
    
//   }
// }

static PyObject *py_stopmotors(PyObject *self, PyObject *args)
{
  printf("py_stopmotors _is_running %d\n", is_running);
  is_running = false;
  printf("py_stopmotors _is_running %d\n", is_running);
  return PyLong_FromLong(0L);
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

  return PyLong_FromLong(0L);
}

static PyObject *py_steps(PyObject *self, PyObject *args)
{

  is_running=true;

  int lin_steps = 0, rot_steps = 0, delay = 0;

  if (!PyArg_ParseTuple(args, "iii",
                        &lin_steps,
                        &rot_steps,
                        &delay))
  {
    return NULL;
  }

  printf("lin_pos=%d rot_pos=%d\n", lin_pos, rot_pos);

  bcm2835_gpio_write(rot_en_pin, LOW);
  bcm2835_gpio_write(lin_en_pin, LOW);
  steps_with_speed(rot_steps, lin_steps, delay, &is_at_limit, 10, 10);
  bcm2835_gpio_write(rot_en_pin, HIGH);
  bcm2835_gpio_write(lin_en_pin, HIGH);

  printf("lin_pos=%d rot_pos=%d\n", lin_pos, rot_pos);

  return PyLong_FromLong(0L);
}

static PyObject *py_calibrate(PyObject *self, PyObject *args)
{

  bcm2835_gpio_write(rot_en_pin, LOW);
  bcm2835_gpio_write(lin_en_pin, LOW);

  printf("lin_pos %d", lin_pos);

  lin_pos = 0;

  char descriptions[4][12] = {"Find Switch",
                     "Back Away 1",
                     "Creep Up",
                     "Back Away 2"};

  int cali_moves[4][3] = {  
   {50000, 1, &is_at_limit} ,
   {-100, 10, &is_not_at_limit} ,
   {200, 100, &is_at_limit} ,
   {-100, 100, &is_not_at_limit}
  };

  printf("\n");

  is_running = true;

  steps_with_speed(500000, 0, 1, &is_not_at_rot_limit, 10, 10);  
  sleep(1);
  steps_with_speed(500000, 0, 1, &is_at_rot_limit, 10, 10);  
  sleep(1);
  rot_pos = 0;
  steps_with_speed(500000, 0, 1, &is_not_at_rot_limit, 10, 10);  
  sleep(1);
  steps_with_speed(500000, 0, 1, &is_at_rot_limit, 10, 10);  
  steps_per_revolution = abs(rot_pos);
  rot_pos = 0;
  
  for(int i=0 ; i<4 ; i++){
    printf("%s\n", descriptions[i]);
    steps_with_speed(0, cali_moves[i][0], cali_moves[i][1], cali_moves[i][2], 10, 10);  
    printf("lin_pos=%d\n", lin_pos);
  }

  lin_pos = 0;

  for(int i=0 ; i<4 ; i++){
    printf("%s\n", descriptions[i]);
    steps_with_speed(0, -cali_moves[i][0], cali_moves[i][1], cali_moves[i][2], 10, 10);  
    printf("lin_pos=%d\n", lin_pos);
  }

  steps_per_linear = abs(lin_pos);
  lin_pos = 0;

  bcm2835_gpio_write(rot_en_pin, HIGH);
  bcm2835_gpio_write(lin_en_pin, HIGH);

  is_running = false;

  printf("Calibrated, steps_per_linear=%d, steps_per_revolution=%d", steps_per_linear , steps_per_revolution);

  return PyLong_FromLong((long)steps_per_linear);
}

static PyMethodDef DrivingMethods[] = {
    {"init", py_init, METH_VARARGS, "Function for initialisation"},
    {"calibrate", py_calibrate, METH_VARARGS, "Function for calibration"},
    {"steps", py_steps, METH_VARARGS, "Function to move"},
    {"run_file", py_run_file, METH_VARARGS, "Function to move"},
    {"stopmotors", py_stopmotors, METH_VARARGS, "Function for driving motor"},
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
