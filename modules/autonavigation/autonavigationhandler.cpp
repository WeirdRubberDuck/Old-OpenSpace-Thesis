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
        double endTime = ps.startTime() + ps.duration();
        if (endTime > _currentTime) {
            return ps;
        }
    }
}

void AutoNavigationHandler::createPath(PathSpecification& spec) {
    clearPath();
    bool success = true;
    for (int i = 0; i < spec.instructions().size(); i++) {
        PathSpecification::Instruction ins = spec.instructions()[i];

        // TODO: test different types of segments
        success = readTargetNodeInstruction(ins, i);
        if (!success) break;
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
    double t = (_currentTime - cps.startTime()) / cps.duration();
    t = transferfunctions::cubicEaseInOut(t); // TEST
    t = std::max(0.0, std::min(t, 1.0));

    // TODO: don't set every frame
    // Set anchor node in orbitalNavigator, to render visible nodes and 
    // add possibility to navigate when we reach the end.
    CameraState cs = (t < 0.5) ? cps.start() : cps.end();
    global::navigationHandler.orbitalNavigator().setAnchorNode(cs.referenceNode);

    // TODO: add different ways to interpolate and separate transfer functions for pos and rot
    glm::dvec3 cameraPosition = cps.getPositionAt(t);
    glm::dquat cameraRotation = cps.getRotationAt(t);

    camera()->setPositionVec3(cameraPosition); 
    camera()->setRotation(cameraRotation);

    _currentTime += deltaTime;

    // reached the end of the path => stop playing
    if (_currentTime > _pathDuration) {
        _isPlaying = false;
        // TODO: implement suitable stop behaviour
    }
}
//TODO: remove! No londer used
void AutoNavigationHandler::addToPath(const SceneGraphNode* node, const double duration) {
    ghoul_assert(node != nullptr, "Target node must not be nullptr");
    ghoul_assert(duration > 0, "Duration must be larger than zero.");

    CameraState start = getStartState();

    glm::dvec3 targetPos = computeTargetPositionAtNode(node, start.position);
    CameraState end = cameraStateFromTargetPosition(
        targetPos, node->worldPosition(), node->identifier());

    // compute startTime 
    double startTime = 0.0;
    if (!_pathSegments.empty()) {
        PathSegment last = _pathSegments.back();
        startTime = last.startTime() + last.duration();
    }

    PathSegment newSegment{ start, end, duration, startTime };
    _pathSegments.push_back(newSegment);
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
        _pathDuration += ps.duration();
    }
    _currentTime = 0.0;
    _isPlaying = true;
}

glm::dvec3 AutoNavigationHandler::computeTargetPositionAtNode(
    const SceneGraphNode* node, glm::dvec3 prevPos, std::optional<double> height) 
{
    glm::dvec3 targetPos = node->worldPosition();
    glm::dvec3 targetToPrevVector = prevPos - targetPos;

    double radius = static_cast<double>(node->boundingSphere());

    double desiredDistance;
    if (height.has_value()) {
        desiredDistance = height.value();
    } 
    else {
        desiredDistance = 2 * radius;
    }

    // TODO: compute actual distance above surface and validate negative values

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
        cs = _pathSegments.back().end();
    }

    return cs;
}

bool AutoNavigationHandler::readTargetNodeInstruction(PathSpecification::Instruction& instruction, int index)
{
    CameraState startState, endState;
    double duration, startTime;

    startState = getStartState();

    // Compute end state 
    std::string& identifier = instruction.targetNode;
    const SceneGraphNode* targetNode = sceneGraphNode(identifier);
    if (!targetNode) {
        LERROR(fmt::format("Failed creating path segment number {}. Could not find node '{}' to target", index + 1, identifier));
        return false;
    }

    glm::dvec3 targetPos;
    if (instruction.position.has_value()) {
        // note that the anchor and reference frame is our targetnode. 
        // The position in instruction is given is relative coordinates.
        targetPos = targetNode->worldPosition() + targetNode->worldRotationMatrix() * instruction.position.value();
    }
    else {
        targetPos = computeTargetPositionAtNode(
            targetNode, 
            startState.position, 
            instruction.height
        );
    }

    endState = cameraStateFromTargetPosition(
        targetPos, targetNode->worldPosition(), identifier);

    // compute duration 
    if (instruction.duration.has_value()) {
        if (instruction.duration <= 0) {
            LERROR(fmt::format("Failed creating path segment number {}. Duration can not be negative.", index + 1));
            return false;
        }
        duration = instruction.duration.value();
    }
    else {
        // TODO: compute default duration
        duration = 5.0;
    }

    // compute startTime 
    startTime = 0.0;
    if (!_pathSegments.empty()) {
        PathSegment& last = _pathSegments.back();
        startTime = last.startTime() + last.duration();
    }

    // create new path
    _pathSegments.push_back(
        PathSegment{ startState, endState, duration, startTime }
    );

    return true;
}

} // namespace openspace::autonavigation
