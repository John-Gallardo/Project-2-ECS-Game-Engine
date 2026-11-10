#include "WindowManager.h"

namespace ECSEngine {

WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)) {}

std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

/**
 * Returns the rect associated with the window boundary in window coordinates
 */
Rect WindowManager::GetWindowBounds() const {
    sf::Vector2f windowBounds{mWindow->getSize()};
    return Rect{
        .topLeft = Point2D{0.0f, 0.0f},
        .width = windowBounds.x,
        .height = windowBounds.y
    };
}

/**
 * TODO:
 */
void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {

}

/**
 * TODO:
 */
void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {

}

/**
 * Converts the rectangle from window coordinates to world coordinates
 */
Rect WindowManager::WindowToWorld(const Rect& rect) const {
    // NOTE: topLeft is a POINT, width and height are VECTORS.
    // corresponding function overloads are called.
    Point2D newTopLeft{WindowToWorld(rect.topLeft)};
    float newWidth{WindowToWorld(rect.width)};
    float newHeight{WindowToWorld(rect.height)};
    return Rect{newTopLeft, newWidth, newHeight};
}

/**
 * Converts a point from window coordinates to world coordinates
 */
Point2D WindowManager::WindowToWorld(const Point2D& pt) const {
    // TODO: need other member variables
}

/**
 * TODO:
 */
Rect WindowManager::WorldToWindow(const Rect& rect) const {
    return Rect();
}

/**
 * TODO:
 */
Point2D WindowManager::WorldToWindow(const Point2D& pt) const {
    return Point2D();
}

/**
 * Sets the camera by making the given worldPt and screenPt correspond to each other
 */
void WindowManager::SetCamera(const Point2D& worldPt, const Point2D& screenPt) {
    // TODO:
}

/**
 * Returns the world center
 */
Point2D WindowManager::GetWorldCenter() const {
    return mWorldCenter;
}

/**
 * Returns the world scale
 */
float WindowManager::GetWorldScale() const {
    return mWorldUnitsPerPixel;
}

/**
 * Sets the world center
 */
void WindowManager::SetWorldCenter(const Point2D& worldCenter) {
    mWorldCenter = worldCenter;
}

/**
 * Sets the world scale
 */
void WindowManager::SetWorldScale(float worldUnitsPerPixel) {
    mWorldUnitsPerPixel = worldUnitsPerPixel;
}

/**
 * Sets the world rect
 */
void WindowManager::SetWorldRect(const Rect& worldCameraRect) {
    mWorldCameraRect = worldCameraRect;
}

/**
 * Converts the given window distance, x, into world coordinates
 * (windowToWorldDist in lab 9)
 */
float WindowManager::WindowToWorld(float x) const {
    // window units * (world units / window units) -> world units
    return x * mWorldUnitsPerPixel;
}

/**
 * Converts the given world distance, x, into window coordinates
 * (worldToWindowDist in lab 9)
 */
float WindowManager::WorldToWindow(float x) const {
    // world units / (world units / window units) -> window units
    return x / mWorldUnitsPerPixel;
}

}  // namespace ECSEngine
