#ifdef LOCAL_TEST_ENVIRONMENT
#include "own_test_math.h"
#include <stdio.h>
#define OUT_FILE "/home/reinder/tkkrlab/metingen/20140401/out.csv"
FILE *fp_out;
#else
#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "own_math.h"
#include "arm_math.h"
#include "helpers.h"
#endif
#include <math.h>
#include "control.h"
#include "motor.h"
#include <string.h>

// Measurement buffer

fVector3 acc_zk;
float acc_n = 0.0;
fVector3 mag_zk;
float mag_n = 0.0;
fVector3 gyro_zk;
float gyro_n = 0.0;

uint8_t kalman_initialized = 0;

uint32_t time_last_control_update;

fVector3 angle_target = {.x = 0.0, .y = 0.0, .z = 0.0, .t = 0.0};
int power_target = 0;

kalman_data kal_roll, kal_pitch;

PID_state PID_pitch = {.P = CONTROL_P, .D = CONTROL_D, .I = CONTROL_I, .err = 0.0, .dx = 0.0 , .int_I = 0.0, .max_I = CONTROL_MAX_I, .min_I = -CONTROL_MAX_I};
PID_state PID_roll =  {.P = CONTROL_P, .D = CONTROL_D, .I = CONTROL_I, .err = 0.0, .dx = 0.0 , .int_I = 0.0, .max_I = CONTROL_MAX_I, .min_I = -CONTROL_MAX_I};

// Measurements to be injected in the controller
void insert_acc_value(fVector3* acc_val) {
    acc_zk.x += acc_val->x;
    acc_zk.y += acc_val->y;
    acc_zk.z += acc_val->z;
    acc_n++;
}
void insert_mag_value(fVector3* mag_val) {
    mag_zk.x += mag_val->x;
    mag_zk.y += mag_val->y;
    mag_zk.z += mag_val->z;
    mag_n++;
}
void insert_gyro_value(fVector3* gyro_val) {
    gyro_zk.x += gyro_val->x;
    gyro_zk.y += gyro_val->y;
    gyro_zk.z += gyro_val->z;
    gyro_n++;
}

// Set the new targets for feedback
void insert_angle_target(fVector3* new_angle_target) {
    angle_target.x = new_angle_target->x;
    angle_target.y = new_angle_target->y;
    angle_target.z = new_angle_target->z;
    angle_target.t = new_angle_target->t;
}
void get_current_angle(fVector3* angle) {
    angle->t = time_last_control_update;
    angle->x = kal_pitch.x1 * DEGREES_PER_RAD;
    angle->y = kal_roll.x1  * DEGREES_PER_RAD; 
    angle->z = PID_pitch.int_I;
}
void insert_power_target(uint16_t new_power_target) {
    power_target = new_power_target;
}


// Nullify measurement-values
void nullify_measurements() {
    time_last_control_update = time_since_startup;
    acc_zk = (fVector3){.t = time_last_control_update, .x=0.0, .y=0.0, .z=0.0};
    acc_n = 0.0;
    mag_zk = (fVector3){.t = time_last_control_update, .x=0.0, .y=0.0, .z=0.0};
    mag_n = 0.0;
    gyro_zk = (fVector3){.t = time_last_control_update, .x=0.0, .y=0.0, .z=0.0};
    gyro_n = 0.0;
    angle_target = (fVector3){.t = time_last_control_update, .x=0.0, .y=0.0, .z=0.0};
}

// Start controller
void Init_Controller() {
    nullify_measurements();
    angle_target = (fVector3){.t = 0, .x=0.0, .y=0.0, .z=0.0};
    power_target = 0.0;
    kalman_init(&kal_pitch);
    kalman_init(&kal_roll);
    #ifdef LOCAL_TEST_ENVIRONMENT
    fp_out = fopen(OUT_FILE, "w");
    if (!fp_out) {
        printf("Could not open %s for writing\n", OUT_FILE);
    }
    #endif
}

void Close_Controller() {
    #ifdef LOCAL_TEST_ENVIRONMENT
    if (fp_out) {
        fclose(fp_out);
    }
    #endif
}

inline void avg_measurement(fVector3* val, float n) {
    val->x /= n;
    val->y /= n;
    val->z /= n;
}

inline int signum(int var) {
    return (0 < var) - (var < 0);
}

// Currently this only optimizes for pitch_torque
//
// 7 kN thrust max at 338 us speed-setting with about 25cm arms
// --> 5 Nm/unit torque
// for yaw unknow, assuming 1 Nm/unit for now
//
// Order in which targets will be tried to be met:
// max_power -> pitch -> roll -> yaw
void calculate_motor_power(uint16_t *speeds, float32_t Lx, float32_t Ly, float32_t Lz) {
    float32_t m1, m2, m3, m4;
    float32_t min_m = -0.45; // Gives a little margin compared to 0.0
    float32_t max_m = (float32_t) (MAX_SERVO_TIME - MIN_SERVO_TIME);
    float32_t F = (float32_t) power_target;

    // Calc PITCH-axis
    float32_t pitch_power = F / 2.0;
    float32_t diff_pitch_power = Lx * PITCH_TORQUE_PER_UNIT;
    if ( diff_pitch_power > pitch_power) {
        m2 = 0.0;
        m4 = pitch_power;
    } else if ( ( - diff_pitch_power ) > pitch_power ) {
        m2 = pitch_power;
        m4 = 0.0;
    } else {
        m2 = pitch_power - diff_pitch_power;
        m4 = pitch_power + diff_pitch_power;
    }

    // Calc ROLL-axis
    float32_t roll_power = F / 2.0;
    float32_t diff_roll_power = Ly * ROLL_TORQUE_PER_UNIT;
    if ( diff_roll_power > roll_power) {
        m1 = 0.0;
        m3 = roll_power;
    } else if ( ( - diff_roll_power ) > roll_power ) {
        m1 = roll_power;
        m3 = 0.0;
    } else {
        m1 = roll_power - diff_roll_power;
        m3 = roll_power + diff_roll_power;
    }

    speeds[0] = round(m1);
    speeds[1] = round(m2);
    speeds[2] = round(m3);
    speeds[3] = round(m4);
}

uint8_t update_motor_settings(uint16_t *speeds) {
    float32_t dt, current_time, z1, z2;
    float32_t pitch_err, pitch_torque;
    float32_t roll_err, roll_torque;

    // printf("%i / %i \n", time_since_startup, time_last_control_update);
    if (!kalman_initialized && (acc_n > KALMAN_NUM_INIT_MEASUREMENTS)) {
        avg_measurement(&acc_zk, acc_n);
        avg_measurement(&mag_zk, mag_n);
        avg_measurement(&gyro_zk, gyro_n);
        kal_pitch.x1 = atan2(acc_zk.y, acc_zk.z);
        kal_pitch.x2 = 0.0;
        kal_pitch.x3 = (RADS_PER_DEGREE*gyro_zk.y);
        kal_roll.x1 = atan2(acc_zk.x, acc_zk.z);
        kal_roll.x2 = 0.0;
        kal_roll.x3 = (RADS_PER_DEGREE*gyro_zk.x);

        kalman_initialized = 1;
        return 0;
    } else if (!kalman_initialized) {
        return 0;
    } else if ((time_last_control_update + CONTROL_SAMPLE_RATE < time_since_startup)
     && (acc_n>0) && (gyro_n>0)) {
        current_time = time_since_startup;
        dt = ( (float32_t) (current_time - time_last_control_update)) / 1000.0;
        time_last_control_update = current_time;

        // Per meetserie een estimate maken van de huidige waarde door het gemiddelde te nemen
        avg_measurement(&acc_zk, acc_n);
        avg_measurement(&mag_zk, mag_n);
        avg_measurement(&gyro_zk, gyro_n);

        z1 = atan2(acc_zk.y, acc_zk.z);
        z2 = gyro_zk.y*RADS_PER_DEGREE;
        kalman_innovate(&kal_pitch, z1, z2, dt);
#ifndef LOCAL_TEST_ENVIRONMENT
        LedAngle(kal_pitch.x1 * DEGREES_PER_RAD * 10);
#endif
        pitch_err = kal_pitch.x1 - angle_target.y;
        pitch_torque = do_pid(&PID_pitch, pitch_err, dt);

#ifdef LOCAL_TEST_ENVIRONMENT
        if (fp_out)
        {
            fprintf(fp_out, "%d\t%.7f\t%.7f\t%.7f\t%.7f\t%.7f\n",
                time_since_startup, kal_pitch.x1, kal_pitch.x2, kal_pitch.x3, z1, z2);
        }
        else {
            printf("%d\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\n",
                time_since_startup, kal_pitch.x1, kal_pitch.x2, kal_pitch.x3, z1, z2);
        }
#endif


        z1 = atan2(acc_zk.x, acc_zk.z);
        z2 = gyro_zk.x*RADS_PER_DEGREE;
        kalman_innovate(&kal_roll, z1, z2, dt);
        roll_err    = kal_roll.x1 - angle_target.x;
        roll_torque = do_pid(&PID_roll, roll_err, dt);

        calculate_motor_power(&(speeds[0]), pitch_torque, roll_torque, 0.0);
#ifdef LOCAL_TEST_ENVIRONMENT
        printf("%04i %04i %04i %04i %f %f\n", speeds[0], speeds[1], speeds[2], speeds[3], pitch_torque, roll_torque);
#endif
        nullify_measurements();
        return 1;
    }
    return 0;
}

// Setup the kalman data struct
void kalman_init(kalman_data *data)
{
    data->x1 = 0.0f;
    data->x2 = 0.0f;
    data->x3 = 0.0f;

    // Init P to diagonal matrix with large values since
    // the initial state is not known
    data->p11 = 1000.0f;
    data->p12 = 0.0f;
    data->p13 = 0.0f;
    data->p21 = 0.0f;
    data->p22 = 1000.0f;
    data->p23 = 0.0f;
    data->p31 = 0.0f;
    data->p32 = 0.0f;
    data->p33 = 1000.0f;

    data->q1 = Q1;
    data->q2 = Q2;
    data->q3 = Q3;
    data->r1 = R1;
    data->r2 = R2;
}

void kalman_innovate(kalman_data *data, float32_t z1, float32_t z2, float32_t dt) {
    float32_t y1, y2;
    float32_t a, b, c;
    float32_t sDet;
    float32_t s11, s12, s21, s22;
    float32_t k11, k12, k21, k22, k31, k32;
    float32_t p11, p12, p13, p21, p22, p23, p31, p32, p33;

    // Step 1
    // x(k) = Fx(k-1) + Bu + w:
    data->x1 = data->x1 + dt*data->x2 - dt*data->x3;
    //x2 = x2;
    //x3 = x3;

    // Step 2
    // P = FPF'+Q
    a = data->p11 + data->p21*dt - data->p31*dt;
    b = data->p12 + data->p22*dt - data->p32*dt;
    c = data->p13 + data->p23*dt - data->p33*dt;
    data->p11 = a + b*dt - c*dt + data->q1;
    data->p12 = b;
    data->p13 = c;
    data->p21 = data->p21 + data->p22*dt - data->p23*dt;
    data->p22 = data->p22 + data->q2;
    //p23 = p23;
    data->p31 = data->p31 + data->p32*dt - data->p33*dt;
    //p32 = p32;
    data->p33 = data->p33 + data->q3;

    // Step 3
    // y = z(k) - Hx(k)
    y1 = z1-data->x1;
    y2 = z2-data->x2;

    // Step 4
    // S = HPT' + R
    s11 = data->p11 + data->r1;
    s12 = data->p12;
    s21 = data->p21;
    s22 = data->p22 + data->r2;

    // Step 5
    // K = PH*inv(S)
    sDet = 1/(s11*s22 - s12*s21);
    k11 = (data->p11*s22 - data->p12*s21)*sDet;
    k12 = (data->p12*s11 - data->p11*s12)*sDet;
    k21 = (data->p21*s22 - data->p22*s21)*sDet;
    k22 = (data->p22*s11 - data->p21*s12)*sDet;
    k31 = (data->p31*s22 - data->p32*s21)*sDet;
    k32 = (data->p32*s11 - data->p31*s12)*sDet;

    // Step 6
    // x = x + Ky
    data->x1 = data->x1 + k11*y1 + k12*y2;
    data->x2 = data->x2 + k21*y1 + k22*y2;
    data->x3 = data->x3 + k31*y1 + k32*y2;

    // Step 7
    // P = (I-KH)P
    p11 = data->p11*(1.0f - k11) - data->p21*k12;
    p12 = data->p12*(1.0f - k11) - data->p22*k12;
    p13 = data->p13*(1.0f - k11) - data->p23*k12;
    p21 = data->p21*(1.0f - k22) - data->p11*k21;
    p22 = data->p22*(1.0f - k22) - data->p12*k21;
    p23 = data->p23*(1.0f - k22) - data->p13*k21;
    p31 = data->p31 - data->p21*k32 - data->p11*k31;
    p32 = data->p32 - data->p22*k32 - data->p12*k31;
    p33 = data->p33 - data->p22*k32 - data->p13*k31;
    data->p11 = p11; data->p12 = p12; data->p13 = p13;
    data->p21 = p21; data->p22 = p22; data->p23 = p23;
    data->p31 = p31; data->p32 = p32; data->p33 = p33;
}

/*
 * Returns the next control-value with this PID-controller based on the error
 * To smooth the D-value, a 3/4 moving avg filter is used
 */
float32_t do_pid(PID_state* cur_pid_state, float32_t err_value, float32_t dt) {
    // Smooth the dt
    float32_t new_err = ((3 * cur_pid_state->err) + err_value) / 4.0;
    cur_pid_state->dx = (new_err - cur_pid_state->err) / dt;
    cur_pid_state->err = new_err;
    cur_pid_state->int_I += ( err_value * dt );

    // Prevent windup of I
    if (cur_pid_state->int_I < cur_pid_state->min_I) {
        cur_pid_state->int_I = cur_pid_state->min_I;
    } else if (cur_pid_state->int_I > cur_pid_state->max_I) {
        cur_pid_state->int_I = cur_pid_state->max_I;
    }

    return (cur_pid_state->P * err_value)
        + (cur_pid_state->D * cur_pid_state->dx)
        + (cur_pid_state->I * cur_pid_state->int_I);
}
