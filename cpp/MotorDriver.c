#include <Python.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "pthread_barrier.c"

#include "vec.c"

#ifdef __APPLE__
int bcm2835_init(void) { return 1; }
void bcm2835_gpio_write(uint8_t pin, uint8_t on) {}
void bcm2835_gpio_fsel(uint8_t pin, uint8_t mode) {}
uint8_t bcm2835_gpio_lev(uint8_t pin) {}
int BCM2835_GPIO_FSEL_OUTP = 0b001;
int BCM2835_GPIO_FSEL_INPT = 0b000;
#define HIGH 0x1
#define LOW 0x0

#else
#include "bcm2835.c"
#endif

pthread_mutex_t running_mutex;
volatile bool is_running = false;
int rot_dir_pin = 0, rot_step_pin = 0, rot_en_pin = 0, lin_dir_pin = 0, lin_step_pin = 0, lin_en_pin = 0, outer_limit_pin = 0, inner_limit_pin = 0;

int rot_pos = -1, lin_pos = -1, inner_to_center = -1, outer_to_max = -1;

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

    MotorMovement *rot = malloc(sizeof(MotorMovement));
    MotorMovement *lin = malloc(sizeof(MotorMovement));

    rot->steps = atoi(strtok(line, " "));
    lin->steps = atoi(strtok(NULL, " "));
    rot->delay = (int)(atof(strtok(NULL, " ")) * 1000000000);
    lin->delay = (int)(atof(strtok(NULL, " ")) * 1000000000);

    MotorMovements *this_move = malloc(sizeof(MotorMovements));

    this_move->rot = *rot;
    this_move->lin = *lin;

    vector_add(&moves, *this_move);
  }

  fclose(fp);
  free(line);

  return moves;
}

int is_at_limit(){
    if(bcm2835_gpio_lev(outer_limit_pin) == 1){
      return 1;
    }
    if(bcm2835_gpio_lev(inner_limit_pin) == 1){
      return -1;
    }
    return 0;
}

int step_with_delay(int steps, int delay, int step_pin, int dir_pin)
{

  struct timespec tim1, tim2;
  tim1.tv_sec = 0;
  tim1.tv_nsec = delay;

  bcm2835_gpio_write(step_pin, steps > 0 ? HIGH : LOW);

  for (int i = 0; i < abs(steps); i++)
  {
    bcm2835_gpio_write(step_pin, HIGH);
    nanosleep(&tim1, &tim2);
    bcm2835_gpio_write(step_pin, LOW);
    nanosleep(&tim1, &tim2);
    
    if(is_at_limit() != 0){
      return is_at_limit();
    }

  }
}

void *drive_motor(void *args)
{
  struct drive_motor_args *thread_args;
  thread_args = (drive_motor_args *)args;

  // do stuff

  for (int i = 0; i < (int)vector_size(thread_args->moves); i++)
  {
    pthread_barrier_wait(thread_args->barrier);

    printf("drive_motor _is_running %d\n", is_running);
    if (!is_running)
    {
      printf("Stopping drive_motor\n");
      return 0;
    }

    MotorMovements this_move = ((MotorMovements *)thread_args->moves)[i];
    if (thread_args->is_linear)
    {
      printf("LIN steps=%d delay=%d\n",
             this_move.lin.steps,
             this_move.lin.delay);
      step_with_delay(this_move.lin.steps, this_move.lin.delay, thread_args->step_pin, thread_args->dir_pin);
    }
    else
    {
      printf("ROT steps=%d delay=%d\n",
             this_move.rot.steps,
             this_move.rot.delay);
      step_with_delay(this_move.rot.steps, this_move.rot.delay, thread_args->step_pin, thread_args->dir_pin);
    }
  }

  return 0;
}

void *drive_motors(void *args)
{

  printf("drive_motors");
  char *filename;
  filename = (char *)args;

  printf("drive_motors %s", filename);

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

  pthread_barrier_t enter_barrier;
  pthread_barrier_init(&enter_barrier, NULL, 2);

  drive_motor_args *rot_args = malloc(sizeof(drive_motor_args));
  rot_args->dir_pin = rot_dir_pin;
  rot_args->step_pin = rot_step_pin;
  rot_args->moves = moves;
  rot_args->en_pin = rot_en_pin;
  rot_args->barrier = &enter_barrier;
  rot_args->is_linear = false;

  drive_motor_args *lin_args = malloc(sizeof(drive_motor_args));
  lin_args->dir_pin = lin_dir_pin;
  lin_args->step_pin = lin_step_pin;
  lin_args->moves = moves;
  lin_args->en_pin = lin_en_pin;
  lin_args->barrier = &enter_barrier;
  lin_args->is_linear = true;

  pthread_t rot_thread, lin_thread;
  int iret_lin, iret_rot;

  iret_rot = pthread_create(&rot_thread, NULL, drive_motor, (void *)rot_args);
  iret_lin = pthread_create(&lin_thread, NULL, drive_motor, (void *)lin_args);

  pthread_join(rot_thread, NULL);
  pthread_join(lin_thread, NULL);

  is_running = false;

  pthread_mutex_unlock(&running_mutex);

  return 0;
}

static PyObject *py_drivemotors(PyObject *self, PyObject *args)
{

  pthread_t drive_thread;
  int iret_drive;

  // Declare two pointers
  char *filename;

  if (!PyArg_ParseTuple(args, "s", &filename))
  {
    printf("Parse error\n");
    return NULL;
  }
  printf("Start Thread\n");
  iret_drive = pthread_create(&drive_thread, NULL, drive_motors, (void *)filename);
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

  if (!bcm2835_init())
    return 0;

  bcm2835_gpio_fsel(rot_dir_pin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(rot_step_pin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(rot_en_pin, BCM2835_GPIO_FSEL_OUTP);

  bcm2835_gpio_fsel(lin_dir_pin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(lin_step_pin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(lin_en_pin, BCM2835_GPIO_FSEL_OUTP);

  bcm2835_gpio_fsel(inner_limit_pin, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_fsel(outer_limit_pin, BCM2835_GPIO_FSEL_INPT);

  return PyLong_FromLong(0L);
}

static PyMethodDef DrivingMethods[] = {
    {"drivemotors", py_drivemotors, METH_VARARGS, "Function for driving motor"},
    {"stopmotors", py_stopmotors, METH_VARARGS, "Function for driving motor"},
    {"init", py_init, METH_VARARGS, "Function for initialisation"},
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
