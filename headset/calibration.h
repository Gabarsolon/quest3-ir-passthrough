#ifndef IR_HEADSET_CALIBRATION_H
#define IR_HEADSET_CALIBRATION_H
typedef struct { double projection[3], distortion[8], pose[16]; } IrCamera;
#ifdef _WIN32
#define IR_EXPORT __declspec(dllexport)
#else
#define IR_EXPORT
#endif
IR_EXPORT int ir_ray_map(const IrCamera *rgb, int width, int height, float *out);
IR_EXPORT void ir_camera_pose(const IrCamera *rgb, double out[7]);
IR_EXPORT void ir_rotation(const IrCamera *ir, const IrCamera *rgb, double out[9]);
#endif
