/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2019                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#include <modules/autonavigation/autonavigationhandler.h>

#include <modules/autonavigation/transferfunctions.h>
#include <openspace/engine/globals.h>
#include <openspace/engine/windowdelegate.h>
#include <openspace/interaction/navigationhandler.h>
#include <openspace/util/camera.h>
#include <ghoul/logging/logmanager.h>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/quaternion.hpp>

namespace {
    constexpr const char* _loggerCat = "AutoNavigationHandler";
} // namespace

namespace openspace::autonavigation {

glm::dvec3 GeoPosition::toCartesian() {
    ghoul_assert(globe != nullptr, "Globe must not be nullptr");

    // Compute the normal based on the angles
    const double lat = glm::radians(latitude);
    const double lon = glm::radians(longitude);
    const double cosLat = glm::cos(lat);
    const glm::dvec3 normal = glm::dvec3(
        cosLat * cos(lon),
        cosLat * sin(lon),
        sin(lat)
    ); // OBS! Should this be normalised?

    const double radius = static_cast<double>(globe->boundingSphere());
    const glm::dvec3 radVec = glm::dvec3(radius); // OBS! assumes sphere, but should really be an ellipsoid

    const glm::dvec3 k = radVec * normal;
    const double gamma = sqrt(dot(k, normal));
    const glm::dvec3 rSurface = k / gamma;

    return rSurface + height * normal; // model space
}

// ------------------------------------------------------------

AutoNavigationHandler::AutoNavigationHandler()
    : properties::PropertyOwner({ "AutoNavigationHandler" })
{
    // Add the properties
    // TODO
}

AutoNavigationHandler::~AutoNavigationHandler() {} // NOLINT
Camera* AutoNavigationHandler::camera() const {
    return global::navigationHandler.camera();
}

void AutoNavigationHandler::updateCamera(double deltaTime) {
    ghoul_assert(camera() != nullptr, "Camera must not be nullptr");

    if (!_isPlaying || _pathSegments.empty()) return;

    //  find current pathsegment (TODO: make private function)
    PathSegment currentSegment = _pathSegments.front();
    double durationSum = 0.0;
    for (PathSegment& ps : _pathSegments) {
        durationSum += ps.duration;
        if (durationSum > _currentTime) { // TODO: make sure edge cases are correct
            currentSegment = ps;
            break;
        }
    }

    // interpolate (TODO: make a function, depending on spline type?)

    const CameraState& start = currentSegment.start;
    const CameraState& end = currentSegment.end;
    const double duration = currentSegment.duration;

    // TODO: compute scaled time based on where on the curve we are
    double startTime = durationSum - duration; // Later, perhaps save this..
    double t = (_currentTime - startTime) / duration;
    t = transferfunctions::cubicEaseInOut(t); // TEST
    t = std::max(0.0, std::min(t, 1.0));

    // TODO: add different ways to interpolate later
    glm::dvec3 cameraPosition = start.position * (1.0 - t) + end.position * t;
    glm::dquat cameraRotation = glm::slerp(
        start.rotation,
        end.rotation,
        t);

    camera()->setPositionVec3(cameraPosition);
    camera()->setRotation(cameraRotation);

    _currentTime += deltaTime;

    // reached the end of the path
    if (_currentTime > _totalDuration) {
        _isPlaying = false;
    }
}

void AutoNavigationHandler::addToPath(const SceneGraphNode* node, const double duration) {
    ghoul_assert(node != nullptr, "Target node must not be nullptr");

    CameraState start;
    if (_pathSegments.empty()) {
        // No previous path, use the camera's current position as start state
        start.position = camera()->positionVec3();
        start.rotation = camera()->rotationQuaternion();
    }
    else {
        start = _pathSegments.back().end;
    }

    glm::dvec3 targetPos = computeTargetPositionAtNode(node, start.position);
    CameraState end = cameraStateFromTargetPosition(targetPos, node->worldPosition());

    PathSegment newSegment{ start, end, duration };
    _pathSegments.push_back(newSegment);
}

void AutoNavigationHandler::clearPath() {
    _pathSegments.clear();
}

void AutoNavigationHandler::startPath() {

    // TODO: if no path, return

    _totalDuration = 0.0;
    for (auto ps : _pathSegments) {
        _totalDuration += ps.duration;
    }
    _currentTime = 0.0;
    _isPlaying = true;
}

glm::dvec3 AutoNavigationHandler::computeTargetPositionAtNode(const SceneGraphNode* node, glm::dvec3 prevPos) {
    glm::dvec3 targetPos = node->worldPosition();
    glm::dvec3 targetToPrevVector = prevPos - targetPos;

    // TODO: let the user input this? Or control this in a more clever fashion
    double radius = static_cast<double>(node->boundingSphere());
    double desiredDistance = 2 * radius;

    // move target position out from surface, along vector to camera
    targetPos += glm::normalize(targetToPrevVector) * (radius + desiredDistance);

    return targetPos;
}

CameraState AutoNavigationHandler::cameraStateFromTargetPosition(
    glm::dvec3 targetPos, glm::dvec3 lookAtPos)
{
    ghoul_assert(camera() != nullptr, "Camera must not be nullptr");

    glm::dmat4 lookAtMat = glm::lookAt(
        targetPos,
        lookAtPos,
        camera()->lookUpVectorWorldSpace()
    );

    glm::dquat targetRot = glm::normalize(glm::inverse(glm::quat_cast(lookAtMat)));

    return CameraState{ targetPos, targetRot };
}

} // namespace openspace::autonavigation
