#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

// Adapted from https://github.com/pms67/PID
typedef struct {
	float Kp;
	float Ki;
	float Kd;
} K_GAIN;

typedef struct {

	// Controller gains
	float Kp;
	float Ki;
	float Kd;

	// Min and max regimes that the PID has been tuned for. Used for gain scheduling
	float min_val;
	float max_val;

	// Anti-windup gain
	float Kaw;

	// Feedforward term to handle stiction
	float stiction;

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
	float prev_measure;
	float d_error_filt;			
	float d_measured;  // to get speed measurements

	float proportional;
	float integrator;
	float differentiator;

	/* Controller output */
	float out;


} PIDController;

// void  PIDController_Init  (PIDController *pid);
// float PIDController_Update(PIDController *pid, float setpoint, float measurement);

void PIDController_Init(PIDController *pid) {

	pid->out = 0.0f;
    pid->sum_error = 0.0f;
    pid->prev_error = 0.0f;
    pid->d_error_filt = 0.0f;
}

float PIDController_Update(PIDController *pid, float setpoint, float measurement) {

	/*
	* Error signal
	*/
    float error = setpoint - measurement;
    float d_error;

	/*
	* Proportional
	*/
    float proportional = pid->Kp * error;
    float integrator;
    float differentiator;

    pid->error = error;
	/*
	* Integral w/ anti-windup
        u_integral = integral (Ki (err + Kaw*(sat(u) - u)) dt

        [sat(u) - u] acts as a feedback term to error accumulation 
	*/

    // pid->sum_error += (error + pid->Kaw * (pid->limMax - pid->out))* pid->control_interval;

		// trapezoidal integration. += 0.5 * (x[n] + x[n - 1]) * dT
    pid->sum_error = pid->sum_error + 0.5 * (error + pid->prev_error) * pid->control_interval;

	/*
	* Derivative (band-limited differentiator)
	*/
	d_error = (error - pid->prev_error) / pid->control_interval;
	// pid->d_error_filt = (1 - pid->beta)*pid->d_error_filt + (pid->beta) * d_error;
	pid->d_error_filt = d_error;


	/*
	* Compute output and apply limits
	*/
    integrator     = pid->Ki * pid->sum_error;
    differentiator = pid->Kd * pid->d_error_filt;

	// Integrator mangement:
	// if (integrator > pid->limMaxInt) {
	// 	pid->sum_error = pid->limMaxInt / pid->Ki;
	// } else if (integrator < pid->limMinInt) {
	// 	pid->sum_error = pid->limMinInt / pid->Ki;
	// }

	// derivative limits
	// if (differentiator > 0.2) {
	// 	differentiator = 0.2;
	// } else if (differentiator < -0.2) {
	// 	differentiator = -0.2;
	// }

	pid->out = proportional + integrator + differentiator;

	// Handling stiction if the PID output is too low. Idea is this acts as a feedforward path
	if (pid->out < pid->stiction && pid->out > 0) { // positive case
		pid->out = pid->out + pid->stiction;
	}
	else if (pid->out > -pid->stiction && pid->out < 0) { // negative case
		pid->out = pid->out - pid->stiction;
	}

	if (pid->out > pid->limMax) {
		pid->out = pid->limMax;

	} else if (pid->out < pid->limMin) {
		pid->out = pid->limMin;
	}

	pid->proportional     = proportional;
	pid->integrator       = integrator;
	pid->differentiator   = differentiator;

	pid->prev_error       = error;
	pid->prev_measure     = measurement;

	/* Return controller output */
	return pid->out;
}

void PIDController_Measure(PIDController *pid, float measurement) {
	pid->d_measured = (measurement - pid->prev_measure) / pid->control_interval;
	pid->prev_measure = measurement;
}


void PIDController_GainSchedule(
    PIDController *pid, K_GAIN *K_large,  K_GAIN *K_small, float curr_val 
) {
    // linearly interpolate between two sets of PID values tuned for large and small movements
	// Handle the cases where the input value is greater than the limits that we've tuned for 

	// greater than max
	if (curr_val > pid->max_val || curr_val < -pid->max_val) {
		pid->Kp = K_large->Kp;
		pid->Ki = K_large->Ki;
		pid->Kd = K_large->Kd;

	// smaller than min
	} else if (curr_val < pid->min_val || curr_val > -pid->min_val) {
		pid->Kp = K_small->Kp;
		pid->Ki = K_small->Ki;
		pid->Kd = K_small->Kd;

	// linearly interpolate between large and small based on curr_val
	} else {
		float del_x = curr_val - pid->min_val;
		
		// TODO: These are constants so not necessary to do this calculation all the time
		float slope_Kp = (K_large->Kp - K_small->Kp) / (pid->max_val - pid->min_val);
		float slope_Ki = (K_large->Ki - K_small->Ki) / (pid->max_val - pid->min_val);
		float slope_Kd = (K_large->Kd - K_small->Kd) / (pid->max_val - pid->min_val);

		pid->Kp = K_small->Kp + slope_Kp * del_x;
		pid->Ki = K_small->Ki + slope_Ki * del_x;
		pid->Kd = K_small->Kd + slope_Kd * del_x;
	}
}

#endif