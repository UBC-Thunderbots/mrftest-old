#ifndef GEOM_KALMAN_KALMAN_H
#define GEOM_KALMAN_KALMAN_H

#include <chrono>
#include <deque>
#include "util/matrix.h"

/**
 * \brief Implements the basic mathematics of a Kalman filter.
 */
class Kalman final
{
   public:
    /**
     * \brief The type of a timestamp used for measurements, control inputs, and
     * estimates.
     */
    typedef std::chrono::steady_clock::time_point Timestamp;

    /**
     * \brief The type of the difference between two timestamps.
     */
    typedef std::chrono::steady_clock::duration Timediff;

    /**
     * \brief Constructs a new Kalman filter.
     *
     * \param[in] angle \c true for angular semantics (which imply 2π=0), or \c
     * false for linear semantics.
     *
     * \param[in] measure_std the standard deviation of measurements fed to
     * update(double, std::chrono::steady_clock::time_point).
     *
     * \param[in] accel_std expected standard deviation of the random
     * acceleration imposed on the object (jostling).
     *
     * \param[in] decay_time_constant rate of velocity decay.
     */
    explicit Kalman(
        bool angle, double measure_std, double accel_std,
        Timediff decay_time_constant);

    /**
     * \brief Predicts values at a given time.
     *
     * \param[in] prediction_time the time at which values should be extracted.
     *
     * \param[out] state_predict the matrix of predicted values.
     *
     * \param[out] p_predict the matrix of predicted covariances.
     */
    void predict(
        Timestamp prediction_time, Matrix &state_predict,
        Matrix &p_predict) const;

    /**
     * \brief Adds a measurement to the filter.
     *
     * \param[in] measurement the measured value.
     *
     * \param[in] measurement_time the time at which the measurement was taken.
     */
    void update(double measurement, Timestamp measurement_time);

    /**
     * \brief Adds a system control input to the filter.
     *
     * \param[in] input the control input, which must be a second derivative of
     * the system's value.
     *
     * \param[in] input_time the time at which the control input was delivered
     * to the system.
     */
    void add_control(double input, Timestamp input_time);

   private:
    struct ControlInput final
    {
        explicit ControlInput(Timestamp t, double v);

        Timestamp time;
        double value;
    };

    Timestamp last_measurement_time;
    double last_control;
    double sigma_m;
    double sigma_a;
    double time_constant;
    std::deque<ControlInput> inputs;
    Matrix h;
    Matrix p;
    Matrix state_estimate;
    bool is_angle;

    void predict_step(
        Timediff timestep, double control, Matrix &state_predict,
        Matrix &p_predict) const;
    Matrix gen_f_mat(double timestep) const;
    Matrix gen_q_mat(double timestep) const;
    Matrix gen_b_mat(double timestep) const;
};

#endif
