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

        handler.createPathToNode(targetNode, duration);
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
        double duration = 5.0; // TODO set defalt value somwhere better/compute per distance

        // TODO: include height over surface as optional parameter

        handler.createPathToSurface(targetNode, latitude, longitude, duration);
        handler.startPath();

        ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
        return 0;
    }

} // namespace openspace::autonavigation::luascriptfunctions
