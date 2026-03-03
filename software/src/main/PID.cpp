#include "PID.h"

void PIDController_Init(PIDController *pid) {

	pid->out = 0.0f;
    pid->sum_error = 0.0f;
    pid->prev_error = 0.0f;
    pid->d_error = 0.0f;
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
    pid->sum_error += error * pid->control_interval;

	/*
	* Derivative (band-limited differentiator)
	*/
    d_error = (error - pid->prev_error) / pid->control_interval;
    pid->d_error_filt = (1 - pid->beta)*pid->d_error_filt + (pid->beta) * d_error;

	/*
	* Compute output and apply limits
	*/
    integrator     = pid->Ki * pid->sum_error;
    differentiator = pid->Kd * pid->d_error_filt;

    // Stiction mangement

    pid->out = proportional + integrator + differentiator;

    if (pid->out > pid->limMax) {

        pid->out = pid->limMax;

    } else if (pid->out < pid->limMin) {

        pid->out = pid->limMin;

    }

    pid->prev_error       = error;

	/* Return controller output */
    return pid->out;
}