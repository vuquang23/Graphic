#include "transforms.h"

#include "CGL/matrix3x3.h"
#include "CGL/vector2D.h"
#include "CGL/vector3D.h"

namespace CGL {

    Vector2D operator*(const Matrix3x3 &m, const Vector2D &v) {
        Vector3D mv = m * Vector3D(v.x, v.y, 1);
        return Vector2D(mv.x / mv.z, mv.y / mv.z);
    }

    Matrix3x3 translate(float dx, float dy) {
        // Part 3: Fill this in.
        Matrix3x3 ret = Matrix3x3();
        ret[0][0] = ret[1][1] = ret[2][2] = 1;
        ret[2][0] = dx;
        ret[2][1] = dy;
        return ret;
    }

    Matrix3x3 scale(float sx, float sy) {
        // Part 3: Fill this in.
        Matrix3x3 ret = Matrix3x3();
        ret[0][0] = sx;
        ret[1][1] = sy;
        ret[2][2] = 1;
        return ret;
    }

    // The input argument is in degrees counterclockwise
    Matrix3x3 rotate(float deg) {
        // Part 3: Fill this in.
        Matrix3x3 ret = Matrix3x3();
        ret[0][0] = cos(PI * deg / 180);
        ret[0][1] = sin(PI * deg / 180);
        ret[1][0] = -sin(PI * deg / 180);
        ret[1][1] = cos(PI * deg / 180);
        ret[2][2] = 1;
        return ret;
    }

}
