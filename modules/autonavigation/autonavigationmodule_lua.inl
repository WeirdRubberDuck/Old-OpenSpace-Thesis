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

    int goTo(lua_State* L) {
        int nArguments = ghoul::lua::checkArgumentsAndThrow(L, { 1, 2 }, "lua::goTo");

        // get target node
        const std::string& targetNodeName = ghoul::lua::value<std::string>(L, 1);
        const SceneGraphNode* targetNode = sceneGraphNode(targetNodeName);

        if (!targetNode) {
            lua_settop(L, 0);
            return ghoul::lua::luaError(
                L, fmt::format("Could not find node '{}' to target", targetNodeName)
            );
        }

        // get duration
        double duration = (nArguments > 1) ? ghoul::lua::value<double>(L, 2) : 5.0; // TODO set defalt value somwhere better
        
        AutoNavigationModule* module = global::moduleEngine.module<AutoNavigationModule>(); // TODO: check if module was found?
        AutoNavigationHandler& handler = module->AutoNavigationHandler();

        // Find target node position and a desired rotation
        glm::dvec3 targetPosition = targetNode->worldPosition();
        glm::dvec3 targetToCameraVector = handler.camera()->positionVec3() - targetPosition;

        // TODO: let the user input this? Or control this in a more clever fashion
        double radius = static_cast<double>(targetNode->boundingSphere());
        double desiredDistance = 2 * radius;

        // move target position out from surface, along vector to camera
        targetPosition += glm::normalize(targetToCameraVector) * (radius + desiredDistance);

        handler.createPathByTarget(targetPosition, targetNode->worldPosition(), duration);
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
            lua_settop(L, 0);
            return ghoul::lua::luaError(L, "Unknown node name: " + globeIdentifier);
        }

        // TODO: test if the node is a globe? Or allow any sort of node?
        // TODO: test different cases!

        AutoNavigationModule* module = global::moduleEngine.module<AutoNavigationModule>();
        AutoNavigationHandler& handler = module->AutoNavigationHandler();

        double latitude = ghoul::lua::value<double>(L, 2);
        double longitude = ghoul::lua::value<double>(L, 3);

        // TODO: include height over surface as optional parameter
        // TODO: test suitable default heights
        const double radius = targetNode->boundingSphere();
        const double height = 1.5 * radius; 
        double duration = 5.0; // TODO set defalt value somwhere better/compute per distance

        GeoPosition geoPosition{ latitude, longitude, height, targetNode };

        glm::dvec3 cartesianPosition = geoPosition.toCartesian();
        glm::dvec3 targetPosition = targetNode->worldPosition() +
            glm::dvec3(targetNode->worldRotationMatrix() * cartesianPosition);

        handler.createPathByTarget(targetPosition, targetNode->worldPosition(), duration);
        handler.startPath();

        ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
        return 0;
    }

} // namespace openspace::autonavigation::luascriptfunctions
