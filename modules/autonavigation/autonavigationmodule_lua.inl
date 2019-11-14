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

#include <openspace/engine/globals.h>
#include <openspace/engine/moduleengine.h>
#include <openspace/interaction/navigationhandler.h>
#include <openspace/scene/scenegraphnode.h>
#include <openspace/scripting/lualibrary.h>
#include <openspace/util/camera.h>
#include <openspace/util/updatestructures.h>
#include <openspace/query/query.h>
#include <ghoul/logging/logmanager.h>
#include <glm/gtx/vector_angle.hpp>

namespace openspace::autonavigation::luascriptfunctions {

    // TODO: remove later when this function is not needed as an example
    int testMove(lua_State* L) {
        ghoul::lua::checkArgumentsAndThrow(L, 3, "lua::testMove");

        const double v1 = ghoul::lua::value<double>(L, 1);
        const double v2 = ghoul::lua::value<double>(L, 2);
        const double v3 = ghoul::lua::value<double>(L, 3);
        glm::dvec3 diffVec{ v1, v2, v3 };

        Camera* camera = global::navigationHandler.camera();
        if (!camera) return -1;

        camera->setPositionVec3(camera->positionVec3() + diffVec);

        lua_settop(L, 0);
        ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
        return 0;
    }

    // TODO: remove later when this function is not needed as an example
    int testAccessNavigationHandler(lua_State* L) {
        ghoul::lua::checkArgumentsAndThrow(L, 0, "lua::testAccessNavigationHandler");

        AutoNavigationModule* module = global::moduleEngine.module<AutoNavigationModule>();
        AutoNavigationHandler handler = module->AutoNavigationHandler();

        // TOOD: call a test function to see if it is working

        ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
        return 0;
    }

    int goTo(lua_State* L) {
        const int nArguments = ghoul::lua::checkArgumentsAndThrow(
            L,
            { 1, 2 },
            "lua::goTo"
        );

        const std::string& targetNodeName = ghoul::lua::value<std::string>(L, 1);

        double duration = 5; // TODO set defalt value somwhere better
        if (nArguments > 1) {
            duration = ghoul::lua::value<double>(L, 2);
        }

        const SceneGraphNode* targetNode = sceneGraphNode(targetNodeName);
        if (!targetNode) {
            lua_settop(L, 0);
            return ghoul::lua::luaError(
                L, fmt::format("Could not find node '{}' to target", targetNodeName)
            );
        }

        // TODO: create functions
        Camera* camera = global::navigationHandler.camera();
        if (!camera) {
            ghoul::lua::luaError(L, fmt::format("No camera"));
        }

        glm::dvec3 startPosition = camera->positionVec3();
        glm::dquat startRotation = camera->rotationQuaternion();

        // Find target node position and a desired rotation
        glm::dvec3 targetPosition = targetNode->worldPosition();
        double nodeRadius = static_cast<double>(targetNode->boundingSphere());
        glm::dvec3 targetToCameraVector = startPosition - targetPosition;

        // TODO: let the user input this? Or control this in a more clever fashion
        double desiredDistance = 2*nodeRadius;

        // move target position out from surface, along vector to camera
        targetPosition += glm::normalize(targetToCameraVector) * (nodeRadius + desiredDistance);

         //Target rotation
        glm::dmat4 lookAtMat = glm::lookAt(
            targetPosition,
            targetNode->worldPosition(),
            camera->lookUpVectorWorldSpace() 
        );

        glm::dquat targetRotation = glm::normalize(glm::inverse(glm::quat_cast(lookAtMat)));

        AutoNavigationModule* module = global::moduleEngine.module<AutoNavigationModule>();
        AutoNavigationHandler& handler = module->AutoNavigationHandler();

        // Generate path
        AutoNavigationHandler::CameraState start(startPosition, startRotation);
        AutoNavigationHandler::CameraState end(targetPosition, targetRotation);
        AutoNavigationHandler::PathSegment pathSegment(start, end, duration);

        handler.setPath(pathSegment);
        handler.startPath();

        ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
        return 0;
    }

    int goToSurface(lua_State* L) {
        ghoul::lua::checkArgumentsAndThrow(L, 3, "lua::goToSurface");

        // Check if the user provided an existing Scene graph node identifier as the first argument.
        const std::string& globeIdentifier = ghoul::lua::value<std::string>(L, 1);
        const SceneGraphNode* targetNode = sceneGraphNode(globeIdentifier);
        if (!targetNode) {
            return ghoul::lua::luaError(L, "Unknown node name: " + globeIdentifier);
        }

        Camera* camera = global::navigationHandler.camera();
        if (!camera) {
            ghoul::lua::luaError(L, fmt::format("No camera"));
        }

        // TODO: test if the node is a globe? Or allow any sort of node?
        // TODO: test different cases!

        const double latitude = ghoul::lua::value<double>(L, 2);
        const double longitude = ghoul::lua::value<double>(L, 3);

        // COMPUTE TARGET POSITION

            // TODO: test suitable default heights
            const double radius = targetNode->boundingSphere();
            const double height = 2*radius; // TODO: make optional input parameter, also, base on surface 

            // Compute cartesian position from geo pos 
            const double lat = glm::radians(latitude);
            const double lon = glm::radians(longitude);

            const double cosLat = glm::cos(lat);
            const glm::dvec3 normal = glm::dvec3(
                cosLat * cos(lon),
                cosLat * sin(lon),
                sin(lat)
            );
            const glm::dvec3 radVec = glm::vec3(radius); // OBS! assumes sphere, but should really be an ellipsoid
            const glm::dvec3 k = radVec * normal;
            const double gamma = sqrt(dot(k, normal));
            const glm::dvec3 rSurface = k / gamma;
            glm::dvec3 cartesianPosition = rSurface + height * normal; // OBS! SHould set this based on some referenceFrame??

        glm::dvec3 targetPosition = targetNode->worldPosition() +
            glm::dvec3(targetNode->worldRotationMatrix() * cartesianPosition);

        // CREATE PATH
        glm::dvec3 startPosition = camera->positionVec3();
        glm::dquat startRotation = camera->rotationQuaternion();
       
        //Target rotation
        glm::dmat4 lookAtMat = glm::lookAt(
            targetPosition,
            targetNode->worldPosition(),
            camera->lookUpVectorWorldSpace()
        );

        glm::dquat targetRotation = glm::normalize(glm::inverse(glm::quat_cast(lookAtMat)));

        AutoNavigationModule* module = global::moduleEngine.module<AutoNavigationModule>();
        AutoNavigationHandler& handler = module->AutoNavigationHandler();

        // Generate path
        double duration = 5; // TODO set defalt value somwhere better

        AutoNavigationHandler::CameraState start(startPosition, startRotation);
        AutoNavigationHandler::CameraState end(targetPosition, targetRotation);
        AutoNavigationHandler::PathSegment pathSegment(start, end, duration);

        // START PATH
        handler.setPath(pathSegment);
        handler.startPath();

        ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
        return 0;
    }

} // namespace openspace::autonavigation::luascriptfunctions
