#pragma once

#include "MathUtil.h"

namespace ECSEngine {

struct CameraComponent {
    Point2D position;  // [world coordinates] position in world coordinates
    float worldUnitsPerPixel;
    Rect hardExclusion;  // [screen coordinates] Area where the camera is not allowed to go
                         // (negative of this rect)
    Rect softExclusion;  // [screen coordinates] Area where the camera is allowed to go (but will
                         // scroll out)
    float speed;
};

}  // namespace ECSEngine
