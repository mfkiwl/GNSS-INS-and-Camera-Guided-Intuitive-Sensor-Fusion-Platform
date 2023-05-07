#ifndef _INPUTADAPTIONS_H_
#define _INPUTADAPTIONS_H_

const double REa = 6378137.0;
const double f = 1 / 298.257223563;
const double g = 9.81;
#include <cmath>
#include <iostream>

class Transforms
{
public:
    double lam{0.0}, psi{0.0}, REb{0.0}, x, y, z, VeSE_CoordsTrfon1, VeSE_CoordsTrfon2, VeSE_CoordsTrfon3, VeSE_PosnEastMeasd, VeSE_PosnNorthMeasd, VeSE_PosnUpMeasd;
    double e{0.0}, NE{0.0}, IvLat, IvLon, IvUp, d_xyz_t[3][1];
    double VeSE_VVehMeasd, VeSE_RateYawMeasd, VeSE_AVehLgtMeasd, VeSE_AVehLatMeasd, VeSE_SclTchgf_P{1.0};
    static double iv_lat, iv_lon, iv_h;

    Transforms() = default;
    ~Transforms() = default;

    void LLA2ECEF1(double lat, double lon, double h, double arr[]);
    void LLA2ECEF(double VeSE_PosnGpsLati, double VeSE_PosnGpsLgt, double VeSE_PosnGpsAlti, double arr[]);
    void ECEF2ENU(double VeSE_CoordsTrfon1, double VeSE_CoordsTrfon2, double VeSE_CoordsTrfon3, double VeSE_CoordsOrgn_P[3], double ENU[1][3]);
    void IMUtransform(double CC_VVehFromCan, double CC_RateYawIMU, double CC_AVehIMULgt, double CC_AVehIMULat, double IMU_meas_added[4]);
};

#endif