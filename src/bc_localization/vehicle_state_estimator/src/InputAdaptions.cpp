#include "InputAdaptions.h"

/**
 * This function takes in a latitude, longitude, and height and returns the ECEF coordinates of that
 * point
 *
 * @param lat Latitude of the vehicle
 * @param lon Longitude of the vehicle
 * @param h Altitude of the vehicle
 * @param arr The array that will be returned with the ECEF coordinates
 */
void Transforms::LLA2ECEF1(double lat, double lon, double h, double arr[])
{
    if (lat == 0.0 && lon == 0.0 && h == 0.0)
        return;

    if (isnanf(iv_lat) || iv_lat == 0.0)
    {
        iv_lat = lat;
        iv_lon = lon;
        iv_h = h;
    }
    else
    {
        IvLat = iv_lat;
        IvLon = iv_lon;
        IvUp = iv_h;
    }
    lam = IvLon * M_PI / 180;
    psi = IvLat * M_PI / 180;

    REb = REa * (1 - f);
    e = sqrt(pow(REa, 2) - pow(REb, 2)) / REa;
    NE = REa / (sqrt(1 - pow(e, 2) * (pow(sin(psi), 2))));

    x = (NE + IvUp) * cos(psi) * cos(lam);
    y = (NE + IvUp) * cos(psi) * sin(lam);
    z = (NE * (1 - pow(e, 2)) + IvUp) * sin(psi);

    arr[0] = x, arr[1] = y, arr[2] = z;
    // std::cout << "gps ECEF1:" << lat << " " << lon << " " << h << std::endl;
    // std::cout << "gps_iv ECEF1:" << iv_lat << " " << iv_lon << " " << iv_h << std::endl;
    // std::cout << "ECEF1:" << arr[0] << " " << arr[1] << " " << arr[2] << std::endl;
}
double Transforms::iv_lat = NAN;
double Transforms::iv_lon = NAN;
double Transforms::iv_h = NAN;

/**
 * It takes in the latitude, longitude, and altitude of the vehicle and returns the ECEF coordinates of
 * the vehicle
 *
 * @param VeSE_PosnGpsLati Latitude of the vehicle
 * @param VeSE_PosnGpsLgt Longitude of the vehicle
 * @param VeSE_PosnGpsAlti Altitude of the vehicle
 * @param arr array to store the ECEF coordinates
 */
void Transforms::LLA2ECEF(double VeSE_PosnGpsLati, double VeSE_PosnGpsLgt, double VeSE_PosnGpsAlti, double arr[])
{
    if (VeSE_PosnGpsLati == 0.0 && VeSE_PosnGpsLgt == 0.0 && VeSE_PosnGpsAlti == 0.0)
        return;

    lam = VeSE_PosnGpsLgt * M_PI / 180;
    psi = VeSE_PosnGpsLati * M_PI / 180;

    REb = REa * (1 - f);
    e = sqrt(pow(REa, 2) - pow(REb, 2)) / REa;
    NE = REa / (sqrt(1 - pow(e, 2) * (pow(sin(psi), 2))));

    VeSE_CoordsTrfon1 = (NE + VeSE_PosnGpsAlti) * cos(psi) * cos(lam);
    VeSE_CoordsTrfon2 = (NE + VeSE_PosnGpsAlti) * cos(psi) * sin(lam);
    VeSE_CoordsTrfon3 = (NE * (1 - pow(e, 2)) + VeSE_PosnGpsAlti) * sin(psi);

    arr[0] = VeSE_CoordsTrfon1, arr[1] = VeSE_CoordsTrfon2, arr[2] = VeSE_CoordsTrfon3;
}

/**
 * The function takes in the ECEF coordinates of the target and the origin, and returns the ENU
 * coordinates of the target.
 *
 * The function is called in the main function as follows:
 *
 * @param VeSE_CoordsTrfon1 X coordinate of the point to be transformed
 * @param VeSE_CoordsTrfon2 ECEF X coordinate
 * @param VeSE_CoordsTrfon3 The Z-coordinate of the point to be transformed.
 * @param VeSE_CoordsOrgn_P The origin of the ENU coordinate system.
 * @param ENU The output of the function, which is the ENU coordinates of the point.
 */
void Transforms::ECEF2ENU(double VeSE_CoordsTrfon1, double VeSE_CoordsTrfon2, double VeSE_CoordsTrfon3, double VeSE_CoordsOrgn_P[3], double ENU[1][3])
{
    lam = iv_lon * M_PI / 180;
    psi = iv_lat * M_PI / 180;

    double Coords_Arr[1][3]{VeSE_CoordsTrfon1, VeSE_CoordsTrfon2, VeSE_CoordsTrfon3};
    for (int i{0}; i < 3; i++)
    {
        d_xyz_t[i][0] = Coords_Arr[0][i] - VeSE_CoordsOrgn_P[i];
    }

    double R[3][3] = {
        {-sin(lam), cos(lam), 0},
        {-sin(psi) * cos(lam), -sin(psi) * sin(lam), cos(psi)},
        {cos(psi) * cos(lam), cos(psi) * sin(lam), sin(psi)}};

    for (int i{0}; i < 3; i++)
    {
        ENU[0][i] = 0.0;
        for (int j{0}; j < 3; j++)
        {
            ENU[0][i] += R[i][j] * d_xyz_t[j][0];
        }
    }
    VeSE_PosnEastMeasd = ENU[0][0], VeSE_PosnNorthMeasd = ENU[0][1], VeSE_PosnUpMeasd = ENU[0][2];
    // std::cout << "VeSE_PosnEastMeasd: " << VeSE_PosnEastMeasd << " VeSE_PosnNorthMeasd: " << VeSE_PosnNorthMeasd << " VeSE_PosnUpMeasd: " << VeSE_PosnUpMeasd << std::endl;
}

/**
 * This function takes in the raw data from the CAN bus and IMU and transforms it into the correct units for
 * the Kalman Filter
 *
 * @param CC_VVehFromCan Vehicle speed from CAN (km/h -> m/s)
 * @param CC_RateYawIMU Yaw rate from the IMU (degree -> radian)
 * @param CC_AVehIMULgt longitudinal acceleration from IMU (g -> m/s2)
 * @param CC_AVehIMULat lateral acceleration from IMU (g -> m/s2)
 * @param IMU_meas_added the output of the function, which is a 4-element array containing the
 * transformed measurements.
 */
void Transforms::IMUtransform(double CC_VVehFromCan, double CC_RateYawIMU, double CC_AVehIMULgt, double CC_AVehIMULat, double IMU_meas_added[4])
{
    VeSE_VVehMeasd = (CC_VVehFromCan * VeSE_SclTchgf_P) * 1000 / 3600;
    VeSE_RateYawMeasd = CC_RateYawIMU * M_PI / 180;
    VeSE_AVehLgtMeasd = CC_AVehIMULgt * g;
    VeSE_AVehLatMeasd = CC_AVehIMULat * g;

    IMU_meas_added[0] = VeSE_VVehMeasd, IMU_meas_added[1] = VeSE_RateYawMeasd, IMU_meas_added[2] = VeSE_AVehLgtMeasd, IMU_meas_added[3] = VeSE_AVehLatMeasd;
}