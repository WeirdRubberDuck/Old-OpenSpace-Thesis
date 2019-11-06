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

#include <modules/autonavigation/autonavigationmodule.h>

#include <modules/autonavigation/autonavigationmodule_lua.inl>
#include <openspace/engine/globalscallbacks.h>
#include <ghoul/logging/logmanager.h> 
#include <memory>

namespace openspace {

AutoNavigationModule::AutoNavigationModule() : OpenSpaceModule(Name) {}

autonavigation::AutoNavigationHandler AutoNavigationModule::AutoNavigationHandler() {
    return _autoNavgationHandler;
}

std::vector<documentation::Documentation> AutoNavigationModule::documentations() const {
    return {
        // TODO: call documentation methods for the other classes in this module
    };
}

scripting::LuaLibrary AutoNavigationModule::luaLibrary() const {
    scripting::LuaLibrary res;
    res.name = "autonavigation";
    res.functions = {
        // TODO: add our scripting functions
        {
            "testMove",
            &autonavigation::luascriptfunctions::testMove,
            {},
            "double, double, double",
            "Test function that moves the camera by the difference vector specified by the input x, y and z values"
        },
        {
            "testAccessNavigationHandler",
            &autonavigation::luascriptfunctions::testAccessNavigationHandler,
            {},
            "void",
            "Test function to show how AutoNavigationHandler can be accessed"
        }
    };
    return res;  
}

void AutoNavigationModule::internalInitialize(const ghoul::Dictionary&) {
    global::callback::preSync.emplace_back([this]() {
        _autoNavigationHandler.updateCamera();
    });

    // TODO: register other classes (that are Factory created) and used in this module, if any
}

} // namespace openspace
