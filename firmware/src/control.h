#ifndef FEEDBACK
#define FEEDBACK
#include "helpers.h"
#ifdef LOCAL_TEST_ENVIRONMENT
#include "own_test_math.h"
#else
#ifndef ARM_MATH_MATRIX_CHECK
#define ARM_MATH_MATRIX_CHECK
#endif
#include "arm_math.h"
#endif

// The minimum amount of ms between two updates
// The control-loop will always wait till at least one gyro and one acc measurement has been made
#define CONTROL_SAMPLE_RATE 25.0

// The PID-values for the PID-control
#define CONTROL_P 1.0
#define CONTROL_I 0.1
#define CONTROL_D 0.05
#define CONTROL_MAX_I 0.8

// The PID-values for the PID-control
#define CONTROL_RATE_P 0.1
#define CONTROL_RATE_I 0.01
#define CONTROL_RATE_D 0.005
#define CONTROL_RATE_MAX_I 0.08

// Q diagonal 3x3 with these elements on diagonal
#define Q1 5.0f
#define Q2 100.0f
#define Q3 0.01f

// R diagonal 2x2 with these elements on diagonal
#define R1 1000.0f
#define R2 1000.0f

#define KALMAN_NUM_INIT_MEASUREMENTS 1024

#define DEGREES_PER_RAD 57.296
#define RADS_PER_DEGREE 0.017453

// 7 kN thrust max at 338 us speed-setting with about 25cm arms
// --> 5 Nm/unit torque
// for yaw unknow, assuming 1 Nm/unit for now
#define PITCH_TORQUE_PER_UNIT 70
#define ROLL_TORQUE_PER_UNIT  70
#define YAW_TORQUE_PER_UNIT   0.01


typedef struct {
    float32_t x1, x2, x3;
    float32_t p11, p12, p13, p21, p22, p23, p31, p32, p33;
    float32_t q1, q2, q3, r1, r2;
} kalman_data;

typedef struct {
    float32_t P, I, D, err, dx, int_I, max_I, min_I
} PID_state;

void kalman_innovate(kalman_data *data, float32_t z1, float32_t z2, float32_t dt);
void kalman_init(kalman_data *data);
void calculate_motor_power(uint16_t *speeds, float32_t pitch_torque, float32_t roll_torque, float32_t yaw_torque);

uint8_t do_kalman(kalman_data* kalman_state, arm_matrix_instance_f32* zk);
float32_t do_pid(PID_state* cur_pid_state, float32_t err_value, float32_t dt);

// Measurements to be injected in the controller
void insert_acc_value(fVector3* acc_val);
void insert_mag_value(fVector3* mag_val);
void insert_gyro_value(fVector3* gyro_val);

// Set the new targets for feedback
void insert_angle_target(fVector3* new_angle_target);
void insert_power_target(uint16_t new_power_target);

// Expose some states
void get_current_angle(fVector3* angle);

// Start controller
void Init_Controller();
void Init_PitchStruct();
uint8_t update_motor_settings(uint16_t *speeds);
void Close_Controller();

#endif
