/* Same Fisheye62 ray map as tools/ir_calibration.py, evaluated on the headset. */
#include "calibration.h"
#include <math.h>

int ir_ray_map(const IrCamera *rgb, int width, int height, float *out) {
    if (width < 1 || height < 1 || !isfinite(rgb->projection[0]) || rgb->projection[0] <= 0) return 0;
    const double *k = rgb->distortion;
    for (int row = 0; row < height; row++) for (int col = 0; col < width; col++) {
        double tx = (((col + .5) / width) * 2328 - rgb->projection[1]) / rgb->projection[0];
        double ty = (((row + .5) / height) * 1748 - rgb->projection[2]) / rgb->projection[0];
        double x = tx, y = ty, p = k[6], q = k[7];
        for (int n = 0; n < 8; n++) {
            double r2 = x*x + y*y;
            double ex = tx - (x + p*(2*x*x+r2) + 2*q*x*y);
            double ey = ty - (y + q*(2*y*y+r2) + 2*p*x*y);
            double a = 1+6*p*x+2*q*y, b = 2*(q*x+p*y), d = 1+6*q*y+2*p*x;
            double det = a*d-b*b;
            if (!isfinite(det) || fabs(det) < 1e-12) return 0;
            x += (d*ex-b*ey)/det;
            y += (a*ey-b*ex)/det;
        }
        double radius = hypot(x, y), lo = 0, hi = 1.57079632679489661923;
        for (int n = 0; n < 45; n++) {
            double theta = (lo+hi)*.5, power = theta*theta, poly = 1;
            for (int i = 0; i < 6; i++) { poly += k[i]*power; power *= theta*theta; }
            if (theta*poly < radius) lo = theta; else hi = theta;
        }
        double theta = (lo+hi)*.5, z = cos(theta);
        double scale = sin(theta)/fmax(radius, 1e-12)/z;
        if (!isfinite(scale) || z <= 1e-6) return 0;
        int at = (row*width+col)*2;
        out[at] = (float)(x*scale); out[at+1] = (float)(y*scale);
    }
    return 1;
}

void ir_rotation(const IrCamera *ir, const IrCamera *rgb, double out[9]) {
    /* Column-major IR-from-RGB rotation, matching the GLSL upload. */
    for (int col = 0; col < 3; col++) for (int row = 0; row < 3; row++) {
        double sum = 0;
        for (int k = 0; k < 3; k++) sum += ir->pose[k*4+row]*rgb->pose[k*4+col];
        out[col*3+row] = sum;
    }
}

void ir_camera_pose(const IrCamera *rgb, double out[7]) {
    double r[9];
    for (int row = 0; row < 3; row++) for (int col = 0; col < 3; col++) r[row*3+col] = rgb->pose[col*4+row];
    double trace = r[0]+r[4]+r[8];
    if (trace > 0) {
        double s = sqrt(trace+1)*2;
        out[0]=(r[7]-r[5])/s; out[1]=(r[2]-r[6])/s; out[2]=(r[3]-r[1])/s; out[3]=s/4;
    } else {
        int i = 0;
        if (r[4]>r[i*3+i]) i=1;
        if (r[8]>r[i*3+i]) i=2;
        int j=(i+1)%3, k=(i+2)%3;
        double s = sqrt(1+r[i*3+i]-r[j*3+j]-r[k*3+k])*2;
        out[3]=(r[k*3+j]-r[j*3+k])/s; out[i]=s/4;
        out[j]=(r[j*3+i]+r[i*3+j])/s; out[k]=(r[k*3+i]+r[i*3+k])/s;
    }
    for (int row=0; row<3; row++) {
        out[4+row]=0;
        for (int k=0; k<3; k++) out[4+row]-=r[row*3+k]*rgb->pose[k*4+3];
    }
}
