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
 * Draws the rectangle in window coordinates onto the screen
 */
void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {
    // 1. Create rectangle and set position + color accordingly
    sf::RectangleShape rectangle{sf::Vector2f{r.width, r.height}};
    rectangle.setPosition({r.topLeft.x, r.topLeft.y});
    rectangle.setFillColor(sf::Color{
        static_cast<uint8_t>(red),
        static_cast<uint8_t>(green),
        static_cast<uint8_t>(blue),
        static_cast<uint8_t>(alpha)
    });
    
    // 2. Draw the rectangle
    mWindow->draw(rectangle);
}

/**
 * Draws the rectangle's outline in window coordinates onto the screen
 */
void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {
    // 1. Create rectangle and set position, color, outline color, and outline thickness accordingly
    sf::RectangleShape rectangle{sf::Vector2f{r.width, r.height}};
    rectangle.setPosition({r.topLeft.x, r.topLeft.y});
    rectangle.setFillColor(sf::Color::Transparent);
    rectangle.setOutlineColor(sf::Color{
        static_cast<uint8_t>(red),
        static_cast<uint8_t>(green),
        static_cast<uint8_t>(blue),
        static_cast<uint8_t>(alpha)
    });
    rectangle.setOutlineThickness(-1);  // negative values expand towards center of shape

    // 2. Draw the rectangle
    mWindow->draw(rectangle);
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
    // Same formula as lab 9 exercise
    // NOTE: applied on each axis separately since MathUtil.h doesn't supply operator+, etc... for Point2D
    float newX{mWorldCenter.x - ((mWindow->getSize().x * 0.5f) * mWorldUnitsPerPixel) + (pt.x * mWorldUnitsPerPixel)};
    float newY{mWorldCenter.y - ((mWindow->getSize().y * 0.5f) * mWorldUnitsPerPixel) + (pt.y * mWorldUnitsPerPixel)};
    return Point2D{newX, newY};
}

/**
 * Converts the rectangle from world coordinates to window coordinates
 */
Rect WindowManager::WorldToWindow(const Rect& rect) const {
    Point2D newTopLeft{WorldToWindow(rect.topLeft)};
    float newWidth{WorldToWindow(rect.width)};
    float newHeight{WorldToWindow(rect.height)};
    return Rect{newTopLeft, newWidth, newHeight};
}

/**
 * Converts a point from world coordinates to window coordinates
 */
Point2D WindowManager::WorldToWindow(const Point2D& pt) const {
    // Same formula as lab 9 exercise
    // NOTE: applied on each axis separately since MathUtil.h doesn't supply operator+, etc... for Point2D
    float newX{(pt.x - (mWorldCenter.x - ((mWindow->getSize().x * 0.5f) * mWorldUnitsPerPixel))) / mWorldUnitsPerPixel};
    float newY{(pt.y - (mWorldCenter.y - ((mWindow->getSize().y * 0.5f) * mWorldUnitsPerPixel))) / mWorldUnitsPerPixel};
    return Point2D{newX, newY};
}

/**
 * Sets the camera by making the given worldPt and screenPt correspond to each other
 */
void WindowManager::SetCamera(const Point2D& worldPt, const Point2D& screenPt) {
    // Same formula as lab 9 exercise
    // NOTE: applied on each axis separately since MathUtil.h doesn't supply operator+, etc... for Point2D
    float worldCenterX = worldPt.x - screenPt.x * mWorldUnitsPerPixel + mWindow->getSize().x * 0.5f * mWorldUnitsPerPixel;
    float worldCenterY = worldPt.y - screenPt.y * mWorldUnitsPerPixel + mWindow->getSize().y * 0.5f * mWorldUnitsPerPixel;
    mWorldCenter = Point2D{worldCenterX, worldCenterY};
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
 * Sets the world rect by
 * 1. setting mWorldCenter to the center of the rectangle
 * NOTE: scale isn't set, not sure if that's what's expected
 */
void WindowManager::SetWorldRect(const Rect& worldCameraRect) {
    // 1. grab center of rectangle in WORLD coordinates & then set
    // NOTE: operator+, etc... isn't supported by MathUtil.h for Point2D
    float rectMidX{worldCameraRect.topLeft.x + worldCameraRect.width / 2};
    float rectMidY{worldCameraRect.topLeft.y + worldCameraRect.height / 2};
    mWorldCenter = Point2D{rectMidX, rectMidY};

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
