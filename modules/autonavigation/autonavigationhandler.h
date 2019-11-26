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

#include <modules/autonavigation/pathsegment.h>
#include <modules/autonavigation/pathspecification.h>
#include <openspace/interaction/interpolator.h>
#include <openspace/properties/propertyowner.h>
#include <openspace/scene/scenegraphnode.h>
#include <ghoul/glm.h>

namespace openspace {
    class Camera;
} // namespace openspace

namespace openspace::autonavigation {

struct CameraState;

// TODO: move to its own file?
struct GeoPosition {
    double latitude;    // degrees
    double longitude;   // degrees
    double height;
    SceneGraphNode* globe;

    glm::dvec3 toCartesian(); // TODO: move out of struct
};

class AutoNavigationHandler : public properties::PropertyOwner {
public:
    AutoNavigationHandler();
    ~AutoNavigationHandler();

    // Mutators

    // Accessors
    Camera* camera() const;
    const double pathDuration() const;
    PathSegment& currentPathSegment();

    void createPath(PathSpecification spec);

    void updateCamera(double deltaTime);
    void addToPath(const SceneGraphNode* node, double duration);
    void addToPath(GeoPosition geo, double duration);
    void clearPath();
    void startPath();

    // TODO: move these to privates
    glm::dvec3 computeTargetPositionAtNode(const SceneGraphNode* node, 
        const glm::dvec3 prevPos);

    CameraState cameraStateFromTargetPosition(glm::dvec3 targetPos, 
        glm::dvec3 lookAtPos, std::string node);

private:
    CameraState getStartState();
    void addPathSegment(CameraState start, CameraState end, double duration);

    std::vector<PathSegment> _pathSegments;

    double _pathDuration;
    double _currentTime; 
    bool _isPlaying = false;
};

} // namespace openspace::autonavigation

#endif // __OPENSPACE_CORE___NAVIGATIONHANDLER___H__