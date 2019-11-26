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
#include <openspace/scene/scenegraphnode.h>
#include <openspace/util/camera.h>
#include <openspace/query/query.h>
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

const double AutoNavigationHandler::pathDuration() const {
    return _pathDuration;
}

PathSegment& AutoNavigationHandler::currentPathSegment() {
    for (PathSegment& ps : _pathSegments) {
        double endTime = ps.startTime + ps.duration;
        if (endTime > _currentTime) {
            return ps;
        }
    }
}

void AutoNavigationHandler::createPath(PathSpecification spec) {
    clearPath();
    bool success = true;
    for (int i = 0; i < spec.instructions().size(); i++) {

        PathSpecification::Instruction ins = spec.instructions().at(i);
        // TODO: process different path instructions

        // Read target node instruction (TODO: make a function)
        if (ins.duration <= 0) {
            LERROR(fmt::format("Failed creating path segment nr {}. Duration can not be negative.", i+1));
            success = false;
            break;
        }

        const SceneGraphNode* targetNode = sceneGraphNode(ins.targetNode);
        if (!targetNode) {
            LERROR(fmt::format("Failed creating path segment nr {}. Could not find node '{}' to target", i+1, ins.targetNode));
            success = false;
            break;
        }
        addToPath(targetNode, ins.duration);
    }

    if (success) 
        startPath();
    else 
        LINFO("Could not create path.");
}

void AutoNavigationHandler::updateCamera(double deltaTime) {
    ghoul_assert(camera() != nullptr, "Camera must not be nullptr");

    if (!_isPlaying || _pathSegments.empty()) return;

    PathSegment cps = currentPathSegment(); 

    // INTERPOLATE (TODO: make a function, and allow different methods)

    double t = (_currentTime - cps.startTime) / cps.duration;
    t = transferfunctions::cubicEaseInOut(t); // TEST
    t = std::max(0.0, std::min(t, 1.0));

    // TODO: don't set every frame and 
    // Set anchor node in orbitalNavigator, to render visible nodes and 
    // add possibility to navigate when we reach the end.
    CameraState cs = (t < 0.5) ? cps.start : cps.end;
    global::navigationHandler.orbitalNavigator().setAnchorNode(cs.referenceNode);

    // TODO: add different ways to interpolate later
    glm::dvec3 cameraPosition = cps.start.position * (1.0 - t) + cps.end.position * t;
    glm::dquat cameraRotation = 
        glm::slerp(cps.start.rotation, cps.end.rotation, t);

    camera()->setPositionVec3(cameraPosition);
    camera()->setRotation(cameraRotation);

    _currentTime += deltaTime;

    // reached the end of the path => stop playing
    if (_currentTime > _pathDuration) {
        _isPlaying = false;
        // TODO: implement suitable stop behaviour
    }
}

void AutoNavigationHandler::addToPath(const SceneGraphNode* node, const double duration) {
    ghoul_assert(node != nullptr, "Target node must not be nullptr");
    ghoul_assert(duration > 0, "Duration must be larger than zero.");

    CameraState start = getStartState();

    glm::dvec3 targetPos = computeTargetPositionAtNode(node, start.position);
    CameraState end = cameraStateFromTargetPosition(
        targetPos, node->worldPosition(), node->identifier());

    addPathSegment(start, end, duration);
}

void AutoNavigationHandler::addToPath(GeoPosition geo, double duration) {
    ghoul_assert(duration > 0, "Duration must be larger than zero.");

    SceneGraphNode* targetNode = geo.globe;
    glm::dvec3 cartesianPos = geo.toCartesian();

    glm::dvec3 targetPos = targetNode->worldPosition() +
        glm::dvec3(targetNode->worldRotationMatrix() * cartesianPos);

    glm::dvec3 lookAtPos = targetNode->worldPosition();
    CameraState end = cameraStateFromTargetPosition(
        targetPos, lookAtPos, targetNode->identifier());

    CameraState start = getStartState();
    addPathSegment(start, end, duration);
}

void AutoNavigationHandler::clearPath() {
    _pathSegments.clear();
    _pathDuration = 0.0;
    _currentTime = 0.0;
}

void AutoNavigationHandler::startPath() {
    ghoul_assert(!_pathSegments.empty(), "Cannot start an empty path");
    
    _pathDuration = 0.0;
    for (auto ps : _pathSegments) {
        _pathDuration += ps.duration;
    }
    _currentTime = 0.0;
    _isPlaying = true;
}

glm::dvec3 AutoNavigationHandler::computeTargetPositionAtNode(
    const SceneGraphNode* node, glm::dvec3 prevPos) 
{
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
    glm::dvec3 targetPos, glm::dvec3 lookAtPos, std::string node)
{
    ghoul_assert(camera() != nullptr, "Camera must not be nullptr");

    glm::dmat4 lookAtMat = glm::lookAt(
        targetPos,
        lookAtPos,
        camera()->lookUpVectorWorldSpace()
    );

    glm::dquat targetRot = glm::normalize(glm::inverse(glm::quat_cast(lookAtMat)));

    return CameraState{ targetPos, targetRot, node};
}

CameraState AutoNavigationHandler::getStartState() {
    CameraState cs;
    if (_pathSegments.empty()) {
        cs.position = camera()->positionVec3();
        cs.rotation = camera()->rotationQuaternion();
        cs.referenceNode = global::navigationHandler.anchorNode()->identifier();
    }
    else {
        cs = _pathSegments.back().end;
    }

    return cs;
}

void AutoNavigationHandler::addPathSegment(CameraState start, CameraState end, double duration) {
    // compute startTime 
    double startTime = 0.0;
    if (!_pathSegments.empty()) {
        PathSegment last = _pathSegments.back();
        startTime = last.startTime + last.duration;
    }

    PathSegment newSegment{ start, end, duration, startTime };
    _pathSegments.push_back(newSegment);
}

} // namespace openspace::autonavigation
