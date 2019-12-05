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

#include <modules/autonavigation/pathsegment.h>

#include <openspace/scene/scenegraphnode.h>
#include <openspace/query/query.h>
#include <ghoul/Misc/interpolator.h>
#include <ghoul/logging/logmanager.h>

namespace {
    constexpr const char* _loggerCat = "PathSegment";
} // namespace

namespace openspace::autonavigation {

PathSegment::PathSegment(
    CameraState start, CameraState end, double duration, double startTime, CurveType type)
    : _start(start), _end(end), _duration(duration), _startTime(startTime), _curveType(type)
{ 
    switch(_curveType) {
    case Bezier:
        generateBezier();
        break;
    case Linear:
        break;
    default:
        LERROR(fmt::format("Cannot create curve of type {}. Type does not exist!", _curveType));
    }  
}

CameraState PathSegment::start() const { return _start; }

CameraState PathSegment::end() const { return _end; }

double PathSegment::duration() const { return _duration; }

double PathSegment::startTime() const { return _startTime; }

glm::vec3 PathSegment::getPositionAt(double t) {
    switch(_curveType) {
    case Bezier: 
        return interpolateBezier(t); //TODO: use helper function when created
        break;
    case Linear:
        return ghoul::interpolateLinear(t, _start.position, _end.position);
        break;
    default:
        LERROR(fmt::format("Cannot get position for curve type {}. Type does not exist!", _curveType));
    }        
}

glm::dquat PathSegment::getRotationAt(double t) {
    return glm::slerp(_start.rotation, _end.rotation, t);
}

//TODO: generate something that looks good! 
void PathSegment::generateBezier() {
    glm::dvec3 startNodePos = sceneGraphNode(_start.referenceNode)->worldPosition();
    glm::dvec3 endNodePos = sceneGraphNode(_end.referenceNode)->worldPosition();
    // vectors pointing away from target nodes
    glm::dvec3 startDirection =_start.position - startNodePos; 
    glm::dvec3 endDirection = _end.position - endNodePos; 

    // TODO: create better control points!
    // Four control points for cubic bezier
    _controlPoints.push_back(_start.position);
    _controlPoints.push_back(_start.position + 10.0 * startDirection);
    _controlPoints.push_back(_end.position + 10.0 * endDirection);
    _controlPoints.push_back(_end.position);
}

// TODO: Create a more general helper function and use instead
glm::dvec3 PathSegment::interpolateBezier(double t) { 
    double a = 1.0 - t;
    return _controlPoints[0] * a * a * a
            + _controlPoints[1] * t * a * a * 3.0
            + _controlPoints[2] * t * t * a * 3.0
            + _controlPoints[3] * t * t * t;
}

} // namespace openspace::autonavigation
