#include "VehStEstimn2.h"

/**
 * This function checks if the current measurement is different from the previous measurement. If it
 * is, then the measurement is used in the measurement update step of the Kalman filter
 *
 * @param VeSE_PosnEastMeasd Measured East position of the vehicle
 * @param VeSE_PosnNorthMeasd Measured North position of the vehicle
 * @param VeSE_VVehMeasd Measured longitudinal velocity
 * @param VeSE_RateYawMeasd Measured yaw rate
 * @param VeSE_AVehLgtMeasd Measured longitudinal acceleration of the vehicle
 * @param VeSE_AVehLatMeasd Measured lateral acceleration of the vehicle
 * @param VeSE_IdxSnsrUpd Index of sensors that are updated.
 */
void VehStEstimnClass::MeasurementUpdateChecker(double VeSE_PosnEastMeasd, double VeSE_PosnNorthMeasd, double VeSE_VVehMeasd, double VeSE_RateYawMeasd, double VeSE_AVehLgtMeasd, double VeSE_AVehLatMeasd, double Bearing, int VeSE_IdxSnsrUpd[7])
{

    if (fabs(VeSE_PosnEastMeasd - VeSE_PosnEastMeasd_prev) > 0.0001 || fabs(VeSE_PosnNorthMeasd - VeSE_PosnNorthMeasd_prev) > 0.0001)
        VeSE_IdxSnsrUpd[0] = 1, VeSE_IdxSnsrUpd[1] = 1;
    else
        VeSE_IdxSnsrUpd[0] = 0, VeSE_IdxSnsrUpd[1] = 0;

    if (fabs(VeSE_VVehMeasd - VeSE_VVehMeasd_prev) > 0.00001 || VeSE_VVehMeasd < 0.0001)
        VeSE_IdxSnsrUpd[2] = 1;
    else
        VeSE_IdxSnsrUpd[2] = 0;

    if (fabs(VeSE_RateYawMeasd - VeSE_RateYawMeasd_prev) > 0.00001)
        VeSE_IdxSnsrUpd[3] = 1;
    else
        VeSE_IdxSnsrUpd[3] = 0;

    if (fabs(VeSE_AVehLgtMeasd - VeSE_AVehLgtMeasd_prev) > 0.00001)
        VeSE_IdxSnsrUpd[4] = 1;
    else
        VeSE_IdxSnsrUpd[4] = 0;

    if (fabs(VeSE_AVehLatMeasd - VeSE_AVehLatMeasd_prev) > 0.00001)
        VeSE_IdxSnsrUpd[5] = 1;
    else
        VeSE_IdxSnsrUpd[5] = 0;

    if (fabs(Bearing - Bearing_prev) > 0.00001)
        VeSE_IdxSnsrUpd[6] = 1;
    else
        VeSE_IdxSnsrUpd[6] = 0;

    VeSE_PosnEastMeasd_prev = VeSE_PosnEastMeasd;
    VeSE_PosnNorthMeasd_prev = VeSE_PosnNorthMeasd;
    VeSE_VVehMeasd_prev = VeSE_VVehMeasd;
    VeSE_RateYawMeasd_prev = VeSE_RateYawMeasd;
    VeSE_AVehLgtMeasd_prev = VeSE_AVehLgtMeasd;
    VeSE_AVehLatMeasd_prev = VeSE_AVehLatMeasd;
    Bearing_prev = Bearing;
}

double VehStEstimnClass::VeSE_PosnEastMeasd_prev = 0;
double VehStEstimnClass::VeSE_PosnNorthMeasd_prev = 0;
double VehStEstimnClass::VeSE_VVehMeasd_prev = 0;
double VehStEstimnClass::VeSE_RateYawMeasd_prev = 0;
double VehStEstimnClass::VeSE_AVehLgtMeasd_prev = 0;
double VehStEstimnClass::VeSE_AVehLatMeasd_prev = 0;
double VehStEstimnClass::Bearing_prev = 0;

/**
 * The function takes in the state vector and returns the predicted state vector
 *
 * @param StEstimn State Estimate Vector
 *
 * @return The predicted state vector.
 */
Eigen::MatrixXd VehStEstimnClass::ProcessModelfcn(Eigen::MatrixXd StEstimn)
{
    e = StEstimn.coeff(0, 0);
    n = StEstimn.coeff(1, 0);
    psi = StEstimn.coeff(2, 0);
    v = StEstimn.coeff(3, 0);
    psi_dot = StEstimn.coeff(4, 0);
    a_long = StEstimn.coeff(5, 0);
    a_lat = StEstimn.coeff(6, 0);

    double psi_ddot = 0;
    double a_dot = 0;

    E = e + cos(psi) * (v * VeSE_ValTs_P + a_long * (pow(VeSE_ValTs_P, 2) / 2)) + a_dot * (pow(VeSE_ValTs_P, 3) / 3) - sin(psi) * (v * psi_dot * (pow(VeSE_ValTs_P, 2) / 2)) + (v * psi_ddot + 2 * a_long * psi_dot) * (pow(VeSE_ValTs_P, 3) / 6) + (a_dot * psi_dot + a_long * psi_ddot) * (pow(VeSE_ValTs_P, 4) / 8) + a_dot * psi_ddot * (pow(VeSE_ValTs_P, 5) / 20);

    N = n + sin(psi) * (v * VeSE_ValTs_P + a_long * (pow(VeSE_ValTs_P, 2) / 2)) + a_dot * (pow(VeSE_ValTs_P, 3) / 3) + cos(psi) * (v * psi_dot * (pow(VeSE_ValTs_P, 2) / 2)) + (v * psi_ddot + 2 * a_long * psi_dot) * (pow(VeSE_ValTs_P, 3) / 6) + (a_dot * psi_dot + a_long * psi_ddot) * (pow(VeSE_ValTs_P, 4) / 8) + a_dot * psi_ddot * (pow(VeSE_ValTs_P, 5) / 20);

    Psi = psi + sgn(v) * (psi_dot * VeSE_ValTs_P) + psi_ddot * (pow(VeSE_ValTs_P, 2) / 2);

    V = v + a_long * VeSE_ValTs_P + a_dot * (pow(VeSE_ValTs_P, 2) / 2);

    Psi_dot = psi_dot + psi_ddot * VeSE_ValTs_P;

    A_long = a_long + a_dot * VeSE_ValTs_P;
    A_lat = V * Psi_dot;

    VeSE_StPred << E, N, Psi, V, Psi_dot, A_long, A_lat;

    return VeSE_StPred;
}

/**
 * This function calculates the observation model which is a function of the state vector
 *
 * @param VeSE_StPred The predicted state vector
 *
 * @return The observation model is being returned.
 */
Eigen::MatrixXd VehStEstimnClass::observationModel_h(Eigen::MatrixXd VeSE_StPred)
{

    e = VeSE_StPred.coeff(0, 0);
    n = VeSE_StPred.coeff(1, 0);
    psi = VeSE_StPred.coeff(2, 0);
    v = VeSE_StPred.coeff(3, 0);
    psi_dot = VeSE_StPred.coeff(4, 0);
    a_long = VeSE_StPred.coeff(5, 0);
    a_lat = VeSE_StPred.coeff(6, 0);

    h_ << e, n, psi, v, psi_dot, a_long, a_lat;

    return h_;
}

/**
 * The function takes in the state vector and returns the Jacobian of the process model
 *
 * @param StEstimn State Estimate Vector
 *
 * @return The derivative of the process model with respect to the state vector.
 */
Eigen::MatrixXd VehStEstimnClass::ProcessModelJacobfcn(Eigen::MatrixXd StEstimn)
{

    e = StEstimn.coeff(0, 0);
    n = StEstimn.coeff(1, 0);
    psi = StEstimn.coeff(2, 0);
    v = StEstimn.coeff(3, 0);
    psi_dot = StEstimn.coeff(4, 0);
    a_long = StEstimn.coeff(5, 0);
    a_lat = StEstimn.coeff(6, 0);

    F13 = (-sin(psi) * ((a_long * pow(VeSE_ValTs_P, 2)) / 2 + v * VeSE_ValTs_P) - cos(psi) * ((a_long * psi_dot * pow(VeSE_ValTs_P, 3)) / 3 + (psi_dot * v * pow(VeSE_ValTs_P, 2)) / 2));
    F14 = (VeSE_ValTs_P * cos(psi) - (pow(VeSE_ValTs_P, 2) * psi_dot * sin(psi)) / 2);
    F15 = (-sin(psi) * ((a_long * pow(VeSE_ValTs_P, 3)) / 3 + (v * pow(VeSE_ValTs_P, 2)) / 2));
    F16 = (-(psi_dot * sin(psi) * pow(VeSE_ValTs_P, 3)) / 3 + (cos(psi) * pow(VeSE_ValTs_P, 2)) / 2);

    F23 = (cos(psi) * ((a_long * pow(VeSE_ValTs_P, 2)) / 2 + v * VeSE_ValTs_P) - sin(psi) * ((a_long * psi_dot * pow(VeSE_ValTs_P, 3)) / 3 + (psi_dot * v * pow(VeSE_ValTs_P, 2)) / 2));
    F24 = ((psi_dot * cos(psi) * pow(VeSE_ValTs_P, 2)) / 2 + sin(psi) * VeSE_ValTs_P);
    F25 = (cos(psi) * ((a_long * pow(VeSE_ValTs_P, 3)) / 3 + (v * pow(VeSE_ValTs_P, 2)) / 2));
    F26 = ((psi_dot * cos(psi) * pow(VeSE_ValTs_P, 3)) / 3 + (sin(psi) * pow(VeSE_ValTs_P, 2)) / 2);

    F34 = VeSE_ValTs_P * sgn(v);
    F46 = VeSE_ValTs_P;
    F74 = psi_dot;

    VeSE_ValMtrxDrtv << 1.0, 0.0, F13, F14, F15, F16, 0.0,
        0.0, 1.0, F23, F24, F25, F26, 0.0,
        0.0, 0.0, 1.0, 0.0, F34, 0.0, 0.0,
        0.0, 0.0, 0.0, 1.0, 0.0, F46, 0.0,
        0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, F74, 0.0, 0.0, 1.0;

    return VeSE_ValMtrxDrtv;
}

/**
 * The function `LinearizedGammafcn` takes in the state estimate vector and returns the linearized
 * Gamma matrix
 *
 * @param StEstimn State Estimate Vector
 *
 * @return The linearized matrix of the state transition function.
 */
Eigen::MatrixXd VehStEstimnClass::LinearizedGammafcn(Eigen::MatrixXd StEstimn)
{

    e = StEstimn.coeff(0, 0);
    n = StEstimn.coeff(1, 0);
    psi = StEstimn.coeff(2, 0);
    v = StEstimn.coeff(3, 0);
    psi_dot = StEstimn.coeff(4, 0);
    a_long = StEstimn.coeff(5, 0);
    a_y = StEstimn.coeff(6, 0);

    double psi_ddot = 0;
    double a_dot = 0;

    G00 = -sin(psi) * (a_dot * pow(VeSE_ValTs_P, 5) / 20) + (a_long * pow(VeSE_ValTs_P, 4) / 8) + (v * pow(VeSE_ValTs_P, 3) / 6);
    G01 = (cos(psi) * pow(VeSE_ValTs_P, 3) / 3) - sin(psi) * (psi_ddot * pow(VeSE_ValTs_P, 5) / 20) + (psi_dot * pow(VeSE_ValTs_P, 4) / 8);
    G10 = cos(psi) * (a_dot * pow(VeSE_ValTs_P, 5) / 20) + (a_long * pow(VeSE_ValTs_P, 4) / 8) + (v * pow(VeSE_ValTs_P, 3) / 6);
    G11 = cos(psi) * (psi_ddot * pow(VeSE_ValTs_P, 5) / 20) + (psi_dot * pow(VeSE_ValTs_P, 4) / 8) + (pow(VeSE_ValTs_P, 3) * sin(psi) / 3);
    G20 = pow(VeSE_ValTs_P, 2) * sgn(v) / 2;
    G31 = pow(VeSE_ValTs_P, 2) / 2;
    G40 = VeSE_ValTs_P;
    G51 = VeSE_ValTs_P;

    VeSE_ValMtrxLnrzn << G00, G01,
        G10, G11,
        G20, 0.0,
        0.0, G31,
        G40, 0.0,
        0.0, G51,
        0.0, 0.0;

    return VeSE_ValMtrxLnrzn;
}

/**
 * The function takes in the current state estimate and the current covariance matrix and returns the
 * predicted state and the predicted covariance matrix
 *
 * @param VeSE_ValMtrxUpdPrednCovari The covariance matrix of the state estimate at the previous time
 * step.
 * @param VeSE_StEstimn The state estimate vector
 *
 * @return a pair of values. The first value is the predicted state and the second value is the
 * predicted covariance matrix.
 */
VehStEstimnClass::Struct VehStEstimnClass::State_Prediction(Eigen::MatrixXd VeSE_ValMtrxUpdPrednCovari, Eigen::MatrixXd VeSE_StEstimn)
{
    VehStEstimnClass::Struct PredStCov;

    VeSE_ValMtrxProcNoiseCovari_P << 1.0, 0.0,
        0.0, 100.0; // Q trial

    VeSE_ValMtrxPredCovari = ((ProcessModelJacobfcn(VeSE_StEstimn) * VeSE_ValMtrxUpdPrednCovari) * (ProcessModelJacobfcn(VeSE_StEstimn).transpose()))   // F * P * F'
                             + ((LinearizedGammafcn(VeSE_StEstimn) * VeSE_ValMtrxProcNoiseCovari_P) * (LinearizedGammafcn(VeSE_StEstimn).transpose())); // L * Q * L'

    PredStCov.Covariance = VeSE_ValMtrxPredCovari;
    PredStCov.State = ProcessModelfcn(VeSE_StEstimn);

    return PredStCov;
}

/**
 * The function takes in the predicted state and covariance matrix, the measurement matrix, the
 * measurement covariance matrix, and the measured state, and returns the updated state and covariance
 * matrix
 *
 * @param VeSE_StPred The predicted state vector
 * @param VeSE_ValMtrxPredCovari The covariance matrix of the predicted state
 * @param VeSE_StMeasd The measured state vector
 * @param VeSE_ValMtrxMeas The measurement matrix.
 * @param VeSE_ValMtrxMeasCovari Measurement covariance matrix
 * @param rowIndex Index of the current calculated row
 *
 * @return a pair of Eigen::VectorXd and Eigen::MatrixXd.
 */
VehStEstimnClass::Struct VehStEstimnClass::StateAndCovarianceUpdate(
    Eigen::MatrixXd VeSE_StPred, Eigen::MatrixXd VeSE_ValMtrxPredCovari, double VeSE_StMeasd, Eigen::MatrixXd VeSE_ValMtrxMeas,
    double VeSE_ValMtrxMeasCovari, int rowIndex)
{
    VehStEstimnClass::Struct UpdStCov;

    Eigen::MatrixXd P = Eigen::MatrixXd::Random(7, 7);

    P = VeSE_ValMtrxPredCovari; // Prediction Covariance

    Eigen::MatrixXd H = Eigen::MatrixXd::Random(1, 7);
    H = VeSE_ValMtrxMeas;

    Eigen::MatrixXd R = Eigen::MatrixXd::Random(1, 1);
    R << VeSE_ValMtrxMeasCovari;

    Eigen::MatrixXd S = Eigen::MatrixXd::Random(1, 1);
    S << ((H * P * H.transpose()) + (R));

    Eigen::MatrixXd K = (P * (H.transpose())) * S.inverse(); // Kalman Gain

    VeSE_StUpd = ((VeSE_StPred) + ((K) * (VeSE_StMeasd - (observationModel_h(VeSE_StPred).coeff(rowIndex)))));

    VeSE_ValMtrxUpdCovari = P - (((K * H)) * (P)); // (I - KH)P

    UpdStCov.State = VeSE_StUpd;
    UpdStCov.Covariance = VeSE_ValMtrxUpdCovari;
    return UpdStCov;
}

/**
 * The function takes in the previous state and covariance matrix, and the current state and covariance
 * matrix, and returns the updated state and covariance matrix
 *
 * @param VeSE_StPred Predicted state
 * @param VeSE_ValMtrxPredCovari The covariance matrix of the predicted state.
 * @param VeSE_IdxSnsrUpd This is the index of the sensor that is being updated.
 * @param VeSE_StMeasd The measured state vector.
 *
 * @return the updated state and covariance matrix.
 */
VehStEstimnClass::Struct VehStEstimnClass::State_Update(Eigen::MatrixXd VeSE_StPred, Eigen::MatrixXd VeSE_ValMtrxPredCovari,
                                                        Eigen::MatrixXd VeSE_IdxSnsrUpd, Eigen::MatrixXd VeSE_StMeasd)
{
    VehStEstimnClass::Struct Pred, Updt, result;
    VeSE_ValMtrxMeasCovariCombd_P << 1.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, // R MATRIX
        0.00, 1.00, 0.00, 0.00, 0.00, 0.00, 0.00,
        0.00, 0.00, 1.00, 0.00, 0.00, 0.00, 0.00,
        0.00, 0.00, 0.00, 0.10, 0.00, 0.00, 0.00,
        0.00, 0.00, 0.00, 0.00, 5.00, 0.00, 0.00,
        0.00, 0.00, 0.00, 0.00, 0.00, 0.50, 0.00,
        0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.50;

    VeSE_ValMtrxMeasCombd_P << 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // H MATRIX 6*7
        0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0;

    Pred = State_Prediction(VeSE_ValMtrxUpdCovari_Prev, VeSE_StUpd_Prev);
    VeSE_ValMtrxPredCovari = Pred.Covariance;
    VeSE_StPred = Pred.State;

    for (int i = 0; i < 7; i++)
    {

        VeSE_StPred = (i > 0) ? VeSE_StUpd_Prev : VeSE_StPred;
        VeSE_ValMtrxPredCovari = (i > 0) ? VeSE_ValMtrxPredCovari_Prev : VeSE_ValMtrxPredCovari;
        VeSE_StMeasd_Idx = VeSE_StMeasd.coeff(i);
        VeSE_ValMtrxMeas = VeSE_ValMtrxMeasCombd_P.row(i);
        VeSE_ValMtrxMeasCovari = VeSE_ValMtrxMeasCovariCombd_P.coeff(i, i);
        Updt = StateAndCovarianceUpdate(VeSE_StPred, VeSE_ValMtrxPredCovari,
                                        VeSE_StMeasd_Idx, VeSE_ValMtrxMeas, VeSE_ValMtrxMeasCovari, i);
        VeSE_ValMtrxUpdCovari = Updt.Covariance;
        VeSE_StUpd = Updt.State;

        if (VeSE_IdxSnsrUpd(i) != 0)
        {
            VeSE_ValMtrxUpdCovari_Prev = VeSE_ValMtrxUpdCovari;
            VeSE_StUpd_Prev = VeSE_StUpd;
        }
        else
        {
            if (i == 0)
            {
                VeSE_ValMtrxUpdCovari_Prev = VeSE_ValMtrxPredCovari;
                VeSE_StUpd_Prev = VeSE_StPred;
            }
            else
            {
                VeSE_ValMtrxUpdCovari_Prev = VeSE_ValMtrxUpdCovari_Prev;
                VeSE_StUpd_Prev = VeSE_StUpd_Prev;
            }
        }
    }
    result.Covariance = VeSE_ValMtrxUpdCovari;
    result.State = VeSE_StUpd;
    return result;
}
Eigen::MatrixXd VehStEstimnClass::VeSE_ValMtrxUpdCovari_Prev = Eigen::MatrixXd::Identity(7, 7);
Eigen::MatrixXd VehStEstimnClass::VeSE_ValMtrxPredCovari_Prev = Eigen::MatrixXd::Identity(7, 7);
Eigen::MatrixXd VehStEstimnClass::VeSE_StUpd_Prev = Eigen::MatrixXd::Ones(7, 1);

/**
 * If the input parameter is not a number, then return the previous value of the parameter. If the
 * previous value is also not a number, then return 0
 *
 * @param checkedParam The parameter that is being checked for NaN
 *
 * @return the value of the checked parameter.
 */
double VehStEstimnClass::isNanCheck(double checkedParam)
{

    if (std::isnan(checkedParam))
    {
        checkedParam = checkedParam_prev;

        if (std::isnan(checkedParam))
            return checkedParam = 0;
        else
            return checkedParam;
    }
    else
    {
        checkedParam_prev = checkedParam;

        if (std::isnan(checkedParam))
        {
            return checkedParam = 0;
        }
        else
        {
            return checkedParam;
        }
    }
}
double VehStEstimnClass::checkedParam_prev = 0;

/**
 * If the difference between the current and previous GPS heading angle is greater than 180 degrees,
 * then the sign of the difference is reversed and added to the previous conditional value.
 *
 * The conditional value is then added to the current GPS heading angle to get the estimated heading
 * angle.
 *
 * The current GPS heading angle is then stored as the previous GPS heading angle.
 *
 * The conditional value is also stored as the previous conditional value.
 *
 * If the difference between the current and previous GPS heading angle is less than 180 degrees, then
 * the previous conditional value is added to the current GPS heading angle to get the estimated
 * heading angle.
 *
 * The current GPS heading angle is then stored as the previous GPS heading angle.
 *
 * The previous conditional value is also stored as the previous conditional value.
 *
 * The estimated heading angle is then returned.
 *
 * The conditional value is used to keep track of the number of times the GPS heading angle has crossed
 * the 180
 *
 * @param VeSE_AgYawGps The current heading angle
 *
 * @return The heading angle of the vehicle.
 */
double VehStEstimnClass::HeadingAngleContinuityCheck(double VeSE_AgYawGps)
{

    if (VeSE_AgYawGps > 0)
        return VeSE_AgYawGps * 57.2957795;
    else
        return (VeSE_AgYawGps + (2 * M_PI)) * 57.2957795;

    /*
    VeSE_AgYawGps = (90 - (VeSE_AgYawGps * 57.2957795)) * 0.017453292519943;

    subtrct     = VeSE_AgYawGps - VeSE_AgYawGps_prev;

    if (fabs(subtrct) >= 3.141592653589793)
    {
        conditional = -1*sgn(subtrct) + conditional_prev;
        VeSE_AgYawEstimd = VeSE_AgYawGps + (6.283185307179586 * conditional);
        VeSE_AgYawGps_prev = VeSE_AgYawGps;

        return VeSE_AgYawEstimd;
    }
    else
    {
        VeSE_AgYawEstimd = VeSE_AgYawGps + (6.283185307179586 * conditional_prev);
        VeSE_AgYawGps_prev = VeSE_AgYawGps;

        return VeSE_AgYawEstimd;
    } */
}
double VehStEstimnClass::VeSE_AgYawGps_prev = 0;
double VehStEstimnClass::conditional_prev = 0;

/**
 * If the value of the input is less than zero, return -1. If the value of the input is greater than
 * zero, return 1. If the value of the input is equal to zero, return 0.
 *
 * @param v the velocity of the vehicle
 *
 * @return The sign of the value.
 */
int VehStEstimnClass::sgn(double v)
{
    return (v < 0.0) ? -1 : ((v > 0.0) ? 1 : 0); // if v == 0 ise 0
    // return (v < 0) ? -1 : 1;
}

/**
 * It calculates the heading angle of the vehicle based on the current and previous GPS coordinates
 *
 * @param lgt_curr Current longitude
 * @param lat_curr Current latitude
 *
 * @return The heading angle of the vehicle.
 */
double VehStEstimnClass::HeadingAngleCalculation(double lgt_curr, double lat_curr)
{
    if (lgt_curr == 0.0 && lat_curr == 0.0)
        return 0.0;

    if (initial_value)
    {
        delta_lgt = (lgt_curr - lgt_prev);
        y = sin(delta_lgt) * cos(lat_curr);
        x = cos(lat_prev) * sin(lat_curr) - sin(lat_prev) * cos(lat_curr) * cos(delta_lgt);
        bearing = atan2(y, x);

        bearing = bearing * 57.2957795;
        bearing = fmod((bearing + 360.0), 360.0);
        bearing = 360.0 - bearing;

        return bearing;
    }
    else
    {
        lgt_prev = lgt_curr;
        lat_prev = lat_curr;
        initial_value = true;

        return 0.0;
    }
}
double VehStEstimnClass::lgt_prev = 0.0;
double VehStEstimnClass::lat_prev = 0.0;
bool VehStEstimnClass::initial_value = false;
