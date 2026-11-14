#include "WindowManager.h"

namespace ECSEngine {

// Owns the game window and converts between world and window coordinates.
//
// How it works
//   - The camera is a world point (mWorldCenter) shown at the window's center,
//     plus a zoom (mWorldUnitsPerPixel = world units per window pixel).
//   - WorldToWindow / WindowToWorld use those two values to convert points,
//     rects and distances (distances only need the zoom).
//   - DebugDrawRect / DebugFrameRect draw in window coordinates, so convert
//     world rects with WorldToWindow first.
//
// Lifetimes
//   - The window is a shared_ptr: anyone holding a copy from GetWindow keeps
//     it alive, even after the WindowManager is gone.
//   - Converted values use the current camera. Convert again after the camera
//     moves or zooms; don't keep them across frames.

WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)) {}

std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

// The whole window as a rect at (0, 0), in window pixels.
Rect WindowManager::GetWindowBounds() const {
    sf::Vector2f windowBounds{mWindow->getSize()};
    return Rect{
        .topLeft = Point2D{0.0f, 0.0f},
        .width = windowBounds.x,
        .height = windowBounds.y
    };
}

// Fills `r` (window coordinates) with the given colour.
void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {
    sf::RectangleShape rectangle{sf::Vector2f{r.width, r.height}};
    rectangle.setPosition({r.topLeft.x, r.topLeft.y});
    rectangle.setFillColor(sf::Color{
        static_cast<uint8_t>(red),
        static_cast<uint8_t>(green),
        static_cast<uint8_t>(blue),
        static_cast<uint8_t>(alpha)
    });
    mWindow->draw(rectangle);
}

// Draws a 1-pixel outline of `r` (window coordinates).
void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {
    sf::RectangleShape rectangle{sf::Vector2f{r.width, r.height}};
    rectangle.setPosition({r.topLeft.x, r.topLeft.y});
    rectangle.setFillColor(sf::Color::Transparent);
    rectangle.setOutlineColor(sf::Color{
        static_cast<uint8_t>(red),
        static_cast<uint8_t>(green),
        static_cast<uint8_t>(blue),
        static_cast<uint8_t>(alpha)
    });
    rectangle.setOutlineThickness(-1);  // negative draws the outline inside the rect
    mWindow->draw(rectangle);
}

// Rects: the corner converts as a point, the width and height as distances.
Rect WindowManager::WindowToWorld(const Rect& rect) const {
    Point2D newTopLeft{WindowToWorld(rect.topLeft)};
    float newWidth{WindowToWorld(rect.width)};
    float newHeight{WindowToWorld(rect.height)};
    return Rect{newTopLeft, newWidth, newHeight};
}

// Same formulas as lab 9: world = view's left/top edge + pixels * zoom.
// Point2D has no operator+, so each axis is done separately.
Point2D WindowManager::WindowToWorld(const Point2D& pt) const {
    float newX{mWorldCenter.x - ((mWindow->getSize().x * 0.5f) * mWorldUnitsPerPixel) + (pt.x * mWorldUnitsPerPixel)};
    float newY{mWorldCenter.y - ((mWindow->getSize().y * 0.5f) * mWorldUnitsPerPixel) + (pt.y * mWorldUnitsPerPixel)};
    return Point2D{newX, newY};
}

Rect WindowManager::WorldToWindow(const Rect& rect) const {
    Point2D newTopLeft{WorldToWindow(rect.topLeft)};
    float newWidth{WorldToWindow(rect.width)};
    float newHeight{WorldToWindow(rect.height)};
    return Rect{newTopLeft, newWidth, newHeight};
}

// The reverse of WindowToWorld.
Point2D WindowManager::WorldToWindow(const Point2D& pt) const {
    float newX{(pt.x - (mWorldCenter.x - ((mWindow->getSize().x * 0.5f) * mWorldUnitsPerPixel))) / mWorldUnitsPerPixel};
    float newY{(pt.y - (mWorldCenter.y - ((mWindow->getSize().y * 0.5f) * mWorldUnitsPerPixel))) / mWorldUnitsPerPixel};
    return Point2D{newX, newY};
}

// Moves the camera so `worldPt` shows up at window position `screenPt`.
void WindowManager::SetCamera(const Point2D& worldPt, const Point2D& screenPt) {
    float worldCenterX = worldPt.x - screenPt.x * mWorldUnitsPerPixel + mWindow->getSize().x * 0.5f * mWorldUnitsPerPixel;
    float worldCenterY = worldPt.y - screenPt.y * mWorldUnitsPerPixel + mWindow->getSize().y * 0.5f * mWorldUnitsPerPixel;
    mWorldCenter = Point2D{worldCenterX, worldCenterY};
}

Point2D WindowManager::GetWorldCenter() const {
    return mWorldCenter;
}

float WindowManager::GetWorldScale() const {
    return mWorldUnitsPerPixel;
}

void WindowManager::SetWorldCenter(const Point2D& worldCenter) {
    mWorldCenter = worldCenter;
}

void WindowManager::SetWorldScale(float worldUnitsPerPixel) {
    mWorldUnitsPerPixel = worldUnitsPerPixel;
}

// Centers the camera on the rect. The zoom is left unchanged.
void WindowManager::SetWorldRect(const Rect& worldCameraRect) {
    float rectMidX{worldCameraRect.topLeft.x + worldCameraRect.width / 2};
    float rectMidY{worldCameraRect.topLeft.y + worldCameraRect.height / 2};
    mWorldCenter = Point2D{rectMidX, rectMidY};
}

// Distances only need the zoom (windowToWorldDist / worldToWindowDist in lab 9).
float WindowManager::WindowToWorld(float x) const {
    return x * mWorldUnitsPerPixel;
}

float WindowManager::WorldToWindow(float x) const {
    return x / mWorldUnitsPerPixel;
}

}  // namespace ECSEngine
