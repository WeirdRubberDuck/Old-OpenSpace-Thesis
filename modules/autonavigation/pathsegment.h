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

#ifndef __OPENSPACE_MODULE___PATHSEGMENT___H__
#define __OPENSPACE_MODULE___PATHSEGMENT___H__

#include <ghoul/glm.h>
#include <vector>

namespace openspace::autonavigation {

struct CameraState {
    glm::dvec3 position;
    glm::dquat rotation;
    std::string referenceNode;
};

enum CurveType {
    Bezier, 
    Linear,
    Linear2
};

class PathSegment {
public:
    PathSegment(CameraState start, CameraState end, double duration, double startTime, CurveType type = Bezier);

    CameraState start() const;
    CameraState end() const;
    double duration() const;
    double startTime() const;

    glm::vec3 getPositionAt(double t);
    glm::dquat getRotationAt(double t);
    glm::dquat getLookAtRotation(double t, glm::dvec3 currentPos, glm::dvec3 up);

private: 
    void generateBezier();
    void generateLinear2();

    CameraState _start;
    CameraState _end;
    double _duration;
    double _startTime; 
    CurveType _curveType; 
    std::vector<glm::dvec3> _controlPoints;
};

} // namespace openspace::autonavigation

#endif // __OPENSPACE_MODULE___PATHSEGMENT___H__
