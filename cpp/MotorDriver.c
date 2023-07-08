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
volatile bool is_running = false;
int rot_dir_pin = 0, rot_step_pin = 0, rot_en_pin = 0, lin_dir_pin = 0, lin_step_pin = 0, lin_en_pin = 0, outer_limit_pin = 0, inner_limit_pin = 0;
int rot_pos = -1, lin_pos = -1, inner_to_center = -1, outer_to_max = -1;
int ramp_steps = 10;

#define CLK_SPEED 80
#define CLK_PULSE CLK_SPEED / 4

#define TIME_MULT CLK_SPEED * 1000

struct Movement
{
  int lin_steps;
  int rot_steps;
};

typedef struct Movement vec_movement;

struct MotorMovement
{
  int steps;
  int delay;
};

struct MotorMovements
{
  struct MotorMovement rot;
  struct MotorMovement lin;
};

typedef struct MotorMovement MotorMovement;
typedef struct MotorMovements MotorMovements;

struct drive_motor_args
{
  MotorMovements *moves;
  int step_pin;
  int dir_pin;
  int en_pin;
  pthread_barrier_t *barrier;
  bool is_linear;
};

typedef struct drive_motor_args drive_motor_args;

MotorMovements *create_move(int lin_steps, int lin_delay, int rot_steps, int rot_delay)
{

  MotorMovement *rot = malloc(sizeof(MotorMovement));
  MotorMovement *lin = malloc(sizeof(MotorMovement));
  rot->steps = rot_steps;
  rot->delay = rot_delay;
  lin->steps = lin_steps;
  lin->delay = lin_delay;
  MotorMovements *this_move = malloc(sizeof(MotorMovements));
  this_move->rot = *rot;
  this_move->lin = *lin;

  return this_move;
}

void free_move(MotorMovements *move)
{
  // free(&move->rot);
  // free(&move->lin);
  free(move);
}

void print_movements(MotorMovements *moves)
{
  for (int i = 0; i < (int)vector_size(moves); i++)
  {
    MotorMovements this_move = ((MotorMovements *)moves)[i];
    printf("m.rot.steps=%d m.rot.delay=%d m.lin.steps=%d m.lin.delay=%d\n",
           this_move.rot.steps,
           this_move.rot.delay,
           this_move.lin.steps,
           this_move.lin.delay);
  }
}

void delayMs(unsigned int howLong)
{
  struct timespec sleeper;

  unsigned int uSecs = howLong % 1000000;
  unsigned int wSecs = howLong / 1000000;

  /**/ if (howLong == 0)
    return;
  else if (howLong < 100)
  {

    struct timeval tNow, tLong, tEnd;

    gettimeofday(&tNow, NULL);
    tLong.tv_sec = howLong / 1000000;
    tLong.tv_usec = howLong % 1000000;
    timeradd(&tNow, &tLong, &tEnd);

    while (timercmp(&tNow, &tEnd, <))
      gettimeofday(&tNow, NULL);
  }
  else
  {
    sleeper.tv_sec = wSecs;
    sleeper.tv_nsec = (long)(uSecs * 1000L);
    nanosleep(&sleeper, NULL);
  }
}

MotorMovements *load_file(char *fname)
{

  MotorMovements *moves = vector_create();

  FILE *fp = fopen(fname, "r");
  if (fp == NULL)
  {
    perror("Unable to open file!");
    exit(1);
  }

  // Read lines using POSIX function getline
  // This code won't work on Windows
  char *line = NULL;
  size_t len = 0;

  while (getline(&line, &len, fp) != -1)
  {

    int rot_steps = atoi(strtok(line, " "));
    int lin_steps = atoi(strtok(NULL, " "));
    int rot_delay = (int)(atof(strtok(NULL, " ")) * TIME_MULT);
    int lin_delay = (int)(atof(strtok(NULL, " ")) * TIME_MULT);

    MotorMovements *this_move = create_move(lin_steps, lin_delay, rot_steps, rot_delay);

    vector_add(&moves, *this_move);
  }

  fclose(fp);
  free(line);

  return moves;
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

int theta_rho(int theta, int rho)
{

  return 0;
}

int steps_with_speed(int rot_steps, int lin_steps, int delay, bool checkLimit)
{

  int max_steps = max(rot_steps, lin_steps);
  bool lin_is_max = lin_steps == max_steps;
  bool rot_is_max = rot_steps == max_steps;

  int loop_time = delay * CLK_SPEED;
  int default_pulse_time = loop_time / 4;

  int lin_delay = rot_steps == 0 ? 0 : (lin_is_max ? loop_time : (rot_steps * loop_time) / lin_steps);
  int rot_delay = rot_steps == 0 ? 0 : (rot_is_max ? loop_time : (lin_steps * loop_time) / rot_steps);

  printf("rot_steps=%d\n", rot_steps);
  printf("lin_steps=%d\n", lin_steps);
  printf("lin_is_max=%d\n", lin_is_max);
  printf("rot_is_max=%d\n", rot_is_max);
  printf("max_steps=%d\n", max_steps);
  printf("loop_time=%d\n", loop_time);
  printf("default_pulse_time=%d\n", default_pulse_time);
  printf("lin_delay=%d\n", lin_delay);
  printf("rot_delay=%d\n", rot_delay);
  printf("default_pulse_time=%d\n", default_pulse_time);

  int rot_count = 0;
  int lin_count = 0;

  int clk_count = 0;

  for (int i = 0; i <= max_steps; i++)
  {

    int pulse_time = default_pulse_time;

    if (i < ramp_steps)
    {
      pulse_time *= (ramp_steps - i);
    }
    else if (max_steps - i < ramp_steps)
    {
      pulse_time *= (max_steps - i);
    }

    printf("clk_count=%d rot_count=%d lin_count=%d \n", clk_count, rot_count, lin_count);

    if (checkLimit && is_at_limit() != 0)
    {
      printf("is_at_limit()=%d\n", is_at_limit());
      return is_at_limit();
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

void *drive_motors(void *args)
{

  bcm2835_gpio_write(rot_en_pin, LOW);
  bcm2835_gpio_write(lin_en_pin, LOW);

  printf("drive_motors\n");
  char *filename;
  filename = (char *)args;

  printf("drive_motors %s\n", filename);

  if (pthread_mutex_trylock(&running_mutex) != 0)
  {
    printf("Already running\n");
    return 0;
  }
  else
  {
    is_running = true;
  }

  MotorMovements *moves = load_file(filename);

  // print_movements(moves);

  for (int i = 0; i < (int)vector_size(moves); i++)
  {

    MotorMovements this_move = moves[i];

    // steps_with_delay(this_move, true, 50, 50);
    steps_with_speed(this_move.rot.steps, this_move.lin.steps, 10, true);
  }

  is_running = false;

  pthread_mutex_unlock(&running_mutex);

  bcm2835_gpio_write(rot_en_pin, HIGH);
  bcm2835_gpio_write(lin_en_pin, HIGH);

  return 0;
}

static PyObject *py_drivemotors(PyObject *self, PyObject *args)
{

  pthread_t drive_thread;

  // Declare two pointers
  char *filename;

  if (!PyArg_ParseTuple(args, "s", &filename))
  {
    printf("Parse error\n");
    return NULL;
  }
  printf("Start Thread\n");
  pthread_create(&drive_thread, NULL, drive_motors, (void *)filename);
  printf("Thread\n");
  return PyLong_FromLong(0L);
}

static PyObject *py_stopmotors(PyObject *self, PyObject *args)
{
  printf("py_stopmotors _is_running %d\n", is_running);
  is_running = false;
  printf("py_stopmotors _is_running %d\n", is_running);
  return PyLong_FromLong(0L);
}

static PyObject *py_init(PyObject *self, PyObject *args)
{
  if (pthread_mutex_init(&running_mutex, NULL) != 0)
  {
    printf("pthread_mutex_init() error\n");
    return 0;
  }

  if (!PyArg_ParseTuple(args, "iiiiiiii",
                        &rot_dir_pin,
                        &rot_step_pin,
                        &rot_en_pin,
                        &lin_dir_pin,
                        &lin_step_pin,
                        &lin_en_pin,
                        &outer_limit_pin,
                        &inner_limit_pin))
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

  printf("rot_step_pin=%d, rot_dir_pin=%d, rot_en_pin=%d\n", rot_step_pin, rot_dir_pin, rot_en_pin);

  return PyLong_FromLong(0L);
}

static PyObject *py_steps(PyObject *self, PyObject *args)
{

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
  steps_with_speed(rot_steps, lin_steps, delay, true);
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

  printf("Finding switch\n");
  steps_with_speed(0, 50000, 1, true);

  printf("lin_pos %d\n", lin_pos);

  printf("Move back away from switch\n");
  steps_with_speed(0, -100, 10, false);

  printf("lin_pos %d\n", lin_pos);

  printf("Creep up to switch\n");
  steps_with_speed(0, 200, 100, true);

  printf("lin_pos %d\n", lin_pos);

  printf("Move back away from switch\n");
  steps_with_speed(0, -100, 100, false);

  printf("lin_pos %d\n", lin_pos);

  lin_pos = 0;

  printf("Finding switch\n");
  steps_with_speed(0, -50000, 1, true);

  printf("lin_pos %d\n", lin_pos);

  printf("Move back away from switch\n");
  steps_with_speed(0, 100, 10, false);

  printf("lin_pos %d\n", lin_pos);

  printf("Creep up to switch\n");
  steps_with_speed(0, -200, 100, true);

  printf("lin_pos %d\n", lin_pos);

  printf("Move back away from switch\n");
  steps_with_speed(0, 100, 1, false);

  printf("lin_pos %d\n", lin_pos);

  bcm2835_gpio_write(rot_en_pin, HIGH);
  bcm2835_gpio_write(lin_en_pin, HIGH);

  return PyLong_FromLong((long)lin_pos);
}

static PyMethodDef DrivingMethods[] = {
    {"drivemotors", py_drivemotors, METH_VARARGS, "Function for driving motor"},
    {"stopmotors", py_stopmotors, METH_VARARGS, "Function for driving motor"},
    {"init", py_init, METH_VARARGS, "Function for initialisation"},
    {"calibrate", py_calibrate, METH_VARARGS, "Function for calibration"},
    {"steps", py_steps, METH_VARARGS, "Function to move"},
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
