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
#include <openspace/scene/scenegraphnode.h>
#include <ghoul/glm.h>

namespace openspace {
    class Camera;
} // namespace openspace

namespace openspace::autonavigation {

// TODO: move to its own file?
struct GeoPosition {
    double latitude;    // degrees
    double longitude;   // degrees
    double height;
    const SceneGraphNode* globe;

    glm::dvec3 toCartesian();
};

struct CameraState {
    CameraState() = default;
    CameraState(glm::dvec3 pos, glm::dquat rot);

    glm::dvec3 position;
    glm::dquat rotation;
};

struct PathSegment {
    PathSegment() = default;
    PathSegment(CameraState start,
        CameraState end, double duration);

    void startInterpolation();

    CameraState start;
    CameraState end;
    interaction::Interpolator<double> positionInterpolator;
    interaction::Interpolator<double> rotationInterpolator;
};

class AutoNavigationHandler : public properties::PropertyOwner {
public:
    AutoNavigationHandler();
    ~AutoNavigationHandler();

    // Mutators
    void setPath(PathSegment ps);

    // Accessors
    Camera* camera() const;

    void startPath();
    void updateCamera(double deltaTime);

    // TEST----------------------------------------------------
    CameraState createCameraStateFromTargetPosition(
        glm::dvec3 targetPosition, glm::dvec3 lookAtPosition);

    void addToPath(const SceneGraphNode* node, double duration);

    // ----------------------------------------------------

    // Create a path segment from the current camera position to a target
    // camera position and look at position, with given duration
    void createPathByTarget(glm::dvec3 targetPosition,
        glm::dvec3 lookAtPosition, const double duration);

private:

    // TODO: remove this!
    PathSegment _path; // TODO: later this will have to be some sort of list

    // OBS! Could we use a simpler/more efficient data structure?
    std::vector<PathSegment> _pathSegments;
};

} // namespace openspace::autonavigation

#endif // __OPENSPACE_CORE___NAVIGATIONHANDLER___H__
