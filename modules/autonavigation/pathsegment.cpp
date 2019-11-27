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

namespace openspace::autonavigation {

PathSegment::PathSegment(
    CameraState start, CameraState end, double duration, double startTime, curveType type)
    : _start(start), _end(end), _duration(duration), _startTime(startTime), _posCurveType(type)
{ 
    if (_posCurveType == bezier)
        setBezierPoints();
}

glm::vec3 PathSegment::getPositionAt(double t) {
    if( _posCurveType == bezier )
        return getBezierPositionAt(t);
    else 
        return _start.position * (1.0 - t) + _end.position * t;
}

glm::dquat PathSegment::getRotationAt(double t) {
    return glm::slerp(_start.rotation, _end.rotation, t);
}

//TODO: generate something that looks good! 
void PathSegment::setBezierPoints() {
    glm::dvec3 startNodePos = sceneGraphNode(_start.referenceNode)->worldPosition();
    glm::dvec3 startDirection = _start.position - startNodePos; //away from node

    // Four control points for cubic bezier
    _controlPoints.push_back(_start.position);
    _controlPoints.push_back(_start.position + 10.0 * startDirection);
    _controlPoints.push_back(_end.position);
    _controlPoints.push_back(_end.position);
}

glm::dvec3 PathSegment::getBezierPositionAt(double t) {
    double a = 1.0 - t;
    return _controlPoints[0] * a * a * a
            + _controlPoints[1] * t * a * a * 3.0
            + _controlPoints[2] * t * t * a * 3.0
            + _controlPoints[3] * t * t * t;
}

CameraState PathSegment::start() const { return _start; }

CameraState PathSegment::end() const { return _end; }

double PathSegment::duration() const { return _duration; }

double PathSegment::startTime() const { return _startTime; }

} // namespace openspace::autonavigation
