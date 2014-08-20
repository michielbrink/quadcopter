#ifndef MOTOR
#define MOTOR
#define MIN_SERVO_TIME 1350
#define MAX_SERVO_TIME 1880
#define MAX_SLEW 20

/* Function prototypes -----------------------------------------------*/
void StartupMotors();
void SetMotorSpeeds(uint16_t *speeds);
void GetMotorSpeeds(uint16_t *speeds);

#endif
