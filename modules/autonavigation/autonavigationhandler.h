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

#ifndef __OPENSPACE_CORE___AUTONAVIGATIONHANDLER___H__
#define __OPENSPACE_CORE___AUTONAVIGATIONHANDLER___H__

#include <openspace/interaction/interpolator.h>
#include <openspace/properties/propertyowner.h>
#include <ghoul/glm.h>

namespace openspace {
    class Camera;
} // namespace openspace

namespace openspace::autonavigation {

class AutoNavigationHandler : public properties::PropertyOwner {
public:
    
    struct CameraState {
        CameraState() = default;
        CameraState(glm::dvec3 pos, glm::dquat rot);

        glm::dvec3 position;
        glm::dquat rotation;
    };

    struct PathSegment {
        PathSegment() = default;
        PathSegment(CameraState start, CameraState end);

        void startInterpolation();

        CameraState start;
        CameraState end;
        interaction::Interpolator<double> interpolator;
    };

    AutoNavigationHandler();
    ~AutoNavigationHandler();

    // Mutators
    void setPath(PathSegment ps);

    // Accessors
    Camera* camera() const;

    void startPath();
    void updateCamera(double deltaTime);

private:

    PathSegment _path; // TODO: later this will have to be some sort of list
};

} // namespace openspace::autonavigation

#endif // __OPENSPACE_CORE___NAVIGATIONHANDLER___H__
