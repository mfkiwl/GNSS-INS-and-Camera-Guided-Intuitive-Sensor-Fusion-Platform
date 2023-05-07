#ifndef _VEHSTESTIMN_H_
#define _VEHSTESTIMN_H_

#include <cmath>
#include <iostream>
#include <vector>
#include <eigen3/Eigen/Dense>

class VehStEstimnClass
{
public:
    // External inputs (root inport signals with default storage)
    typedef struct
    {
        double CC_PosnGpsLgt;  // '<Root>/CC_PosnGpsLgt'
        double CC_PosnGpsLati; // '<Root>/CC_PosnGpsLati'
        double CC_PosnGpsAlti; // '<Root>/CC_PosnGpsAlti'
        double CC_VVehFromCan; // '<Root>/CC_VVehFromCan'
        double CC_RateYawIMU;  // '<Root>/CC_RateYawIMU'
        double CC_AVehIMULgt;  // '<Root>/CC_AVehIMULgt'
        double CC_AVehIMULat;  // '<Root>/CC_AVehIMULat'
    } ExtU_VehStEstimn_T;

    static double VeSE_PosnEastMeasd_prev, VeSE_PosnNorthMeasd_prev, VeSE_VVehMeasd_prev, VeSE_RateYawMeasd_prev, VeSE_AVehLgtMeasd_prev, VeSE_AVehLatMeasd_prev, Bearing_prev;
    void MeasurementUpdateChecker(double VeSE_PosnEastMeasd, double VeSE_PosnNorthMeasd, double VeSE_VVehMeasd, double VeSE_RateYawMeasd,
                                  double VeSE_AVehLgtMeasd, double VeSE_AVehLatMeasd, double Bearing, int VeSE_IdxSnsrUpd[7]);

    double VeSE_ValTs_P = 0.02;
    double e, n, psi, v, psi_dot, a_long, a_lat, a_y;

    int sgn(double);

    struct StateNCov
    {
        Eigen::MatrixXd State, Covariance;
    };

    typedef struct StateNCov Struct;

    Eigen::MatrixXd VeSE_ValMtrxUpdPrednCovari = Eigen::MatrixXd::Random(7, 7);
    // *** PROCESS MODEL FUNCTION *** //
    Eigen::MatrixXd VeSE_StPred = Eigen::MatrixXd::Random(7, 1);
    Eigen::MatrixXd ProcessModelfcn(Eigen::MatrixXd);
    Eigen::MatrixXd observationModel_h(Eigen::MatrixXd);
    Eigen::MatrixXd h_ = Eigen::MatrixXd::Random(7, 1);
    double E, N, Psi, V, Psi_dot, A_long, A_lat;
    // *** END OF PROCESS MODEL FUNCTION *** //
    Eigen::MatrixXd VeSE_ValMtrxDrtv = Eigen::MatrixXd::Random(7, 7);
    Eigen::MatrixXd ProcessModelJacobfcn(Eigen::MatrixXd);
    double F13, F14, F15, F16, F23, F24, F25, F26, F34, F46, F74;

    Eigen::MatrixXd VeSE_ValMtrxLnrzn = Eigen::MatrixXd::Random(7, 2);
    Eigen::MatrixXd LinearizedGammafcn(Eigen::MatrixXd);
    double G00, G01, G10, G11, G20, G31, G40, G51;

    Eigen::MatrixXd VeSE_ValMtrxPredCovari = Eigen::MatrixXd::Random(7, 7);
    Eigen::MatrixXd VeSE_ValMtrxProcNoiseCovari_P = Eigen::MatrixXd::Random(2, 2);

    Struct State_Prediction(Eigen::MatrixXd VeSE_ValMtrxUpdPrednCovari, Eigen::MatrixXd VeSE_StEstimn);

    // **** STATE UPDATE **** //
    // State Update Subsystem
    double VeSE_PosnEastMeasd, VeSE_PosnNorthMeasd, VeSE_VVehMeasd, VeSE_RateYawMeasd, VeSE_AVehLgtMeasd, VeSE_AVehLatMeasd;
    Eigen::MatrixXd VeSE_ValMtrxMeasCovariCombd_P = Eigen::MatrixXd::Random(7, 7);
    Eigen::MatrixXd VeSE_ValMtrxMeasCombd_P = Eigen::MatrixXd::Random(7, 7);
    Eigen::MatrixXd VeSE_ValMtrxMeas = Eigen::MatrixXd::Random(1, 7);
    double VeSE_ValMtrxMeasCovari;
    double VeSE_StMeasd_Idx;

    Eigen::MatrixXd VeSE_StMeasd = Eigen::MatrixXd::Random(7, 1);

    // State Update Subsystem -- StateAndCovarianceUpdate subsystem
    Eigen::MatrixXd VeSE_StUpd = Eigen::MatrixXd::Random(7, 1);            // first return element of pair
    Eigen::MatrixXd VeSE_ValMtrxUpdCovari = Eigen::MatrixXd::Random(7, 7); // second return element of pair
    Struct StateAndCovarianceUpdate(Eigen::MatrixXd, Eigen::MatrixXd, double, Eigen::MatrixXd, double, int);
    Struct State_Update(Eigen::MatrixXd, Eigen::MatrixXd, Eigen::MatrixXd, Eigen::MatrixXd);

    static Eigen::MatrixXd VeSE_StUpd_Prev;
    static Eigen::MatrixXd VeSE_ValMtrxPredCovari_Prev;
    static Eigen::MatrixXd VeSE_ValMtrxUpdCovari_Prev;

    // **** END OF STATE UPDATE **** //

    // *** Check NaN Function *** //
    double isNanCheck(double);
    static double checkedParam_prev;

    // *** HeadingAngleContinuityCheck Fınction *** //
    double HeadingAngleContinuityCheck(double);
    double VeSE_AgYawEstimd, conditional, subtrct;
    static double VeSE_AgYawGps_prev;
    static double conditional_prev;
    static int static_idx;

    // *** Heading Angle Calculation *** //
    double heading_angle_calculated = 0.0;
    double delta_lgt, y, x, bearing;
    static double lgt_prev, lat_prev;
    static bool initial_value;
    double HeadingAngleCalculation(double, double);
};

#endif