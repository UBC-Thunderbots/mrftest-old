#include "geom/kalman/kalman.h"
#include <cmath>
#include "geom/angle.h"


Kalman::Kalman(
    bool angle, double measure_std, double accel_std,
    Timediff decay_time_constant)
    : sigma_m(measure_std),
      sigma_a(accel_std),
      time_constant(std::chrono::duration_cast<std::chrono::duration<double>>(
                        decay_time_constant)
                        .count()),
      h(1, 2),
      p(2, 2, Matrix::InitFlag::IDENTITY),
      state_estimate(2, 1, Matrix::InitFlag::ZEROES),
      is_angle(angle)
{
    // %the state measurement operator
    // H=[1 0];(;
    h(0, 0) = 1.0;
    h(0, 1) = 0.0;
}


Matrix Kalman::gen_q_mat(double timestep) const
{
    // %the acceleration to state vector (given an acceleration, what is the
    // state
    // %update)
    // G=[timestep.^2/2; timestep];
    Matrix G(2, 1);
    G(0, 0) = timestep * timestep / 2;
    G(1, 0) = timestep;

    // %The amount of uncertainty gained per step
    const Matrix &q = G * ~G * sigma_a * sigma_a;
    return q;
}

Matrix Kalman::gen_f_mat(double timestep) const
{
    // %the state update matrix (get next state given current one
    // F=[1 timestep;0 decay_constant];
    Matrix f(2, 2);
    f(0, 0) = 1.0;
    f(0, 1) = timestep;
    f(1, 0) = 0.0;
    f(1, 1) = std::exp(-timestep / time_constant);
    return f;
}

// predict forward one step
void Kalman::predict_step(
    Timediff timestep, Matrix &state_predict,
    Matrix &p_predict) const
{
    double timestep_double =
        std::chrono::duration_cast<std::chrono::duration<double>>(timestep)
            .count();
    const Matrix &f = gen_f_mat(timestep_double);
    const Matrix &q = gen_q_mat(timestep_double);
    state_predict   = f * state_predict;
    p_predict       = f * p_predict * ~f + q;
}

// get an estimate of the state and covariance at prediction_time, outputs
// passed by reference
void Kalman::predict(
    Timestamp prediction_time, Matrix &state_predict, Matrix &p_predict) const
{
    state_predict          = state_estimate;
    p_predict              = p;
    Timestamp current_time = last_measurement_time;

    predict_step(prediction_time - current_time, state_predict, p_predict);

    if (is_angle)
    {
        state_predict(0, 0) = Angle::of_radians(state_predict(0, 0)).angle_mod().to_radians();
    }
}

// this should generate an updated state
void Kalman::update(double measurement, Timestamp measurement_time)
{
    Matrix state_priori;
    Matrix p_priori;
    predict(measurement_time, state_priori, p_priori);

    // %how much does the guess differ from the measurement
    double residual = measurement - (h * state_priori)(0, 0);
    if (is_angle)
    {
        residual = Angle::of_radians(residual).angle_mod().to_radians();
    }

    // %The kalman update calculations
    const Matrix &kalman_gain =
        (p_priori * ~h) / (((h * p_priori * ~h)(0, 0)) + sigma_m * sigma_m);
    state_estimate = state_priori + kalman_gain * residual;

    if (is_angle)
    {
        state_estimate(0, 0) =
            Angle::of_radians(state_estimate(0, 0)).angle_mod().to_radians();
    }
    p = (Matrix(2, 2, Matrix::InitFlag::IDENTITY) - kalman_gain * h) * p_priori;

    last_measurement_time = measurement_time;

}

