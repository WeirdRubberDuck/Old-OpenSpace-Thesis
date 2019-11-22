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

#include <modules/autonavigation/pathinstruction.h>
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
        const std::string& targetNodeIdentifier = ghoul::lua::value<std::string>(L, 1);
        const SceneGraphNode* targetNode = sceneGraphNode(targetNodeIdentifier);

        if (!targetNode) {
            lua_settop(L, 0);
            return ghoul::lua::luaError(
                L, fmt::format("Could not find node '{}' to target", targetNodeIdentifier)
            );
        }

        // get duration
        double duration = (nArguments > 1) ? ghoul::lua::value<double>(L, 2) : 5.0; // TODO set defalt value somwhere better

        if (duration <= 0) {
            lua_settop(L, 0);
            return ghoul::lua::luaError(L, "Duration must be larger than zero");
        }
        
        AutoNavigationModule* module = global::moduleEngine.module<AutoNavigationModule>(); // TODO: check if module was found?
        AutoNavigationHandler& handler = module->AutoNavigationHandler();
        
        handler.clearPath();
        handler.addToPath(targetNode, duration);
        handler.startPath();

        lua_settop(L, 0);
        ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
        return 0;
    }

    int goToSurface(lua_State* L) {
        int nArguments = ghoul::lua::checkArgumentsAndThrow(L, { 3, 4 }, "lua::goToSurface");

        // Check if the user provided an existing Scene graph node identifier as the first argument.
        const std::string& globeIdentifier = ghoul::lua::value<std::string>(L, 1);
        SceneGraphNode* targetNode = sceneGraphNode(globeIdentifier);

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

        // TODO: include height as optional parameter
        const double radius = targetNode->boundingSphere();
        const double height = 1.5 * radius; // TODO: should be height over surface

        GeoPosition geoPosition{ latitude, longitude, height, targetNode };
        
        // TODO set defalt duration somwhere better or compute from distance
        double duration = (nArguments > 3) ? ghoul::lua::value<double>(L, 4) : 5.0;

        if (duration <= 0) {
            lua_settop(L, 0);
            return ghoul::lua::luaError(L, "Duration must be larger than zero");
        }

        handler.clearPath();
        handler.addToPath(geoPosition, duration);
        handler.startPath();

        lua_settop(L, 0);
        ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
        return 0;
    }

    int addToPath(lua_State* L) {
        int nArguments = ghoul::lua::checkArgumentsAndThrow(L, { 1, 2 }, "lua::addToPath");

        // get target node
        const std::string& targetNodeIdentifier = ghoul::lua::value<std::string>(L, 1);
        const SceneGraphNode* targetNode = sceneGraphNode(targetNodeIdentifier);

        if (!targetNode) {
            lua_settop(L, 0);
            return ghoul::lua::luaError(
                L, fmt::format("Could not find node '{}' to target", targetNodeIdentifier)
            );
        }

        double duration = (nArguments > 1) ? ghoul::lua::value<double>(L, 2) : 5.0; // TODO set defalt value somwhere better

        if (duration <= 0) {
            lua_settop(L, 0);
            return ghoul::lua::luaError(L, "Duration must be larger than zero");
        }

        AutoNavigationModule* module = global::moduleEngine.module<AutoNavigationModule>();
        AutoNavigationHandler& handler = module->AutoNavigationHandler();
        handler.addToPath(targetNode, duration);

        // TODO: Perhaps test if we succeeded?

        lua_settop(L, 0);
        ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
        return 0;
    }

    int startPath(lua_State* L) {
        ghoul::lua::checkArgumentsAndThrow(L, 0, "lua::startPath");

        AutoNavigationModule* module = global::moduleEngine.module<AutoNavigationModule>();
        AutoNavigationHandler& handler = module->AutoNavigationHandler();

        handler.startPath();

        lua_settop(L, 0);
        ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
        return 0;
    }

    int clearPath(lua_State* L) {
        ghoul::lua::checkArgumentsAndThrow(L, 0, "lua::startPath");

        AutoNavigationModule* module = global::moduleEngine.module<AutoNavigationModule>();
        AutoNavigationHandler& handler = module->AutoNavigationHandler();

        handler.clearPath();

        lua_settop(L, 0);
        ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
        return 0;
    }

    // TEST: try creating a path using a dictionary
    int generatePath(lua_State* L) {
        ghoul::lua::checkArgumentsAndThrow(L, 1, "lua::startPath");

        ghoul::Dictionary dictionary;
        ghoul::lua::luaDictionaryFromState(L, dictionary);

        // Test if we have any instructions
        ghoul::Dictionary instructions = dictionary.value<ghoul::Dictionary>("Instructions");

        // convert dictionary to array of dictionaries
        std::vector<ghoul::Dictionary> instructionsArray;
        for (size_t i = 1; i <= instructions.size(); ++i) {
            ghoul::Dictionary ins = instructions.value<ghoul::Dictionary>(std::to_string(i));
            instructionsArray.push_back(ins);
        }

        if (instructionsArray.empty()) {
            lua_settop(L, 0);
            return ghoul::lua::luaError(
                L, fmt::format("No instructions for camera path generation were provided.")
            );
        }

        // read instructions
        std::vector<PathInstruction> pathInstructions;
        for (ghoul::Dictionary dict : instructionsArray) {
            openspace::documentation::TestResult r =
                openspace::documentation::testSpecification(PathInstruction::Documentation(), dict);

            if (!r.success) {
                lua_settop(L, 0);
                return ghoul::lua::luaError(
                    L, fmt::format("Could not read instruction: {}", ghoul::to_string(r))
                );
            } 

            pathInstructions.push_back(PathInstruction(dict));
        }

        AutoNavigationModule* module = global::moduleEngine.module<AutoNavigationModule>();
        AutoNavigationHandler& handler = module->AutoNavigationHandler();

        handler.clearPath();
        // TODO: create path from instructions
        for (auto pi : pathInstructions) {
            const SceneGraphNode* targetNode = sceneGraphNode(pi.targetNode);
            double duration = pi.duration;
            if (!targetNode) {
                lua_settop(L, 0);
                return ghoul::lua::luaError(
                    L, fmt::format("Could not find node '{}' to target", pi.targetNode)
                );
            }
            handler.addToPath(targetNode, duration);
        }
        handler.startPath();

        lua_settop(L, 0);
        ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
        return 0;
    }

} // namespace openspace::autonavigation::luascriptfunctions
