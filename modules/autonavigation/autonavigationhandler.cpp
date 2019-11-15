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
    const glm::dvec3 radVec = glm::vec3(radius); // OBS! assumes sphere, but should really be an ellipsoid

    const glm::dvec3 k = radVec * normal;
    const double gamma = sqrt(dot(k, normal));
    const glm::dvec3 rSurface = k / gamma;

    return rSurface + height * normal; // model space
}


CameraState::CameraState(glm::dvec3 pos, glm::dquat rot)
    : position(pos), rotation(rot) {}


PathSegment::PathSegment(CameraState start, 
                                                CameraState end, 
                                                double duration)
    : start(start), end(end) 
{
    // TODO: make sure duration is larger than zero

    positionInterpolator.setTransferFunction(
        transferfunctions::cubicEaseInOut); //TODO; set as a property or something
    positionInterpolator.setInterpolationTime(duration);

    rotationInterpolator.setTransferFunction(
        transferfunctions::linear); //TODO; set as a property or something
    rotationInterpolator.setInterpolationTime(duration);
}

void PathSegment::startInterpolation() {
    positionInterpolator.start();
    rotationInterpolator.start();
}


AutoNavigationHandler::AutoNavigationHandler()
    : properties::PropertyOwner({ "AutoNavigationHandler" })
{
    // Add the properties
    // TODO
}

AutoNavigationHandler::~AutoNavigationHandler() {} // NOLINT

void AutoNavigationHandler::setPath(PathSegment ps) {
    _path = ps;
}

Camera* AutoNavigationHandler::camera() const {
    return global::navigationHandler.camera();
}

void AutoNavigationHandler::startPath() {
    _path.startInterpolation();
}

void AutoNavigationHandler::updateCamera(double deltaTime) {
    ghoul_assert(camera() != nullptr, "Camera must not be nullptr");

    glm::dvec3 cameraPosition = camera()->positionVec3();
    glm::dquat cameraRotation = camera()->rotationQuaternion();

    // Position
    if (_path.positionInterpolator.isInterpolating()) {
        _path.positionInterpolator.setDeltaTime(static_cast<float>(deltaTime));
        _path.positionInterpolator.step();

        // TODO: see if this can be done in a better way
        double t = _path.positionInterpolator.value();
        cameraPosition = _path.start.position * (1.0 - t) + _path.end.position * t;
    }

    // Rotation
    if (_path.rotationInterpolator.isInterpolating()) {
        _path.rotationInterpolator.setDeltaTime(static_cast<float>(deltaTime));
        _path.rotationInterpolator.step();
        
        cameraRotation = glm::slerp(
            _path.start.rotation,
            _path.end.rotation,
            _path.rotationInterpolator.value());
    }

    camera()->setPositionVec3(cameraPosition);
    camera()->setRotation(cameraRotation);
}

void AutoNavigationHandler::createPathByTarget(glm::dvec3 targetPosition, 
                                               glm::dvec3 lookAtPosition,
                                               const double duration)
{
    ghoul_assert(camera() != nullptr, "Camera must not be nullptr");

    glm::dmat4 lookAtMat = glm::lookAt(
        targetPosition,
        lookAtPosition,
        camera()->lookUpVectorWorldSpace()
    );

    glm::dquat targetRotation = glm::normalize(glm::inverse(glm::quat_cast(lookAtMat)));
    glm::dvec3 startPosition = camera()->positionVec3();
    glm::dquat startRotation = camera()->rotationQuaternion();

    // Generate path
    CameraState start(startPosition, startRotation);
    CameraState end(targetPosition, targetRotation);
    PathSegment pathSegment(start, end, duration);

    setPath(pathSegment);
}

// TEST----------------------------------------------------
CameraState AutoNavigationHandler::createCameraStateFromTargetPosition(
    glm::dvec3 targetPosition, glm::dvec3 lookAtPosition)
{
    ghoul_assert(camera() != nullptr, "Camera must not be nullptr");

    glm::dmat4 lookAtMat = glm::lookAt(
        targetPosition,
        lookAtPosition,
        camera()->lookUpVectorWorldSpace()
    );

    glm::dquat targetRotation = glm::normalize(glm::inverse(glm::quat_cast(lookAtMat)));

    return CameraState(targetPosition, targetRotation);
}

void AutoNavigationHandler::addToPath(const SceneGraphNode* node, double duration) {
    ghoul_assert(node != nullptr, "Target node must not be nullptr");

    // TODO: Compute a target position close to the given node

    CameraState startState;
    if (_pathSegments.empty()) {
        // No previous path, use the camera's current position as start state
        startState.position = camera()->positionVec3();
        startState.rotation = camera()->rotationQuaternion();
    }
    else {
        PathSegment lastSegment = _pathSegments.back();
        startState = lastSegment.end;
    }

    // TODO: make this a function?
    // -----------------------------------------------------------
    glm::dvec3 targetPosition = node->worldPosition();
    glm::dvec3 targetToPrevVector = startState.position - targetPosition;

    // TODO: let the user input this? Or control this in a more clever fashion
    double radius = static_cast<double>(node->boundingSphere());
    double desiredDistance = 2 * radius;

    // move target position out from surface, along vector to camera
    targetPosition += glm::normalize(targetToPrevVector) * (radius + desiredDistance);
    // -----------------------------------------------------------

    CameraState endState = createCameraStateFromTargetPosition(targetPosition, node->worldPosition());

    PathSegment newSegment{ startState, endState, duration };
    _pathSegments.push_back(newSegment);
}

} // namespace openspace::autonavigation
