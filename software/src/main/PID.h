#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

// Adapted from https://github.com/pms67/PID
typedef struct {

	// Controller gains
	float Kp;
	float Ki;
	float Kd;

	// Anti-windup gain
	float Kaw;

	// 1st order IIR derivative filter coeff.
	float beta;

	/* Output limits */
	float limMin;
	float limMax;
	
	// Integrator limits
	float limMinInt;
	float limMaxInt;

	float control_interval;

	/* Controller state */
	float error;
	float sum_error;
	float prev_error;			
	float d_error;			

	/* Controller output */
	float out;

} PIDController;

void  PIDController_Init  (PIDController *pid);
float PIDController_Update(PIDController *pid, float setpoint, float measurement);

#endif