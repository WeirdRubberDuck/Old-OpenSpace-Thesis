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


namespace openspace::autonavigation {

    PathSegment::PathSegment(
        CameraState start, CameraState end, double duration, double startTime)
        : _start(start), _end(end), _duration(duration), _startTime(startTime)
    { }

    glm::vec3 PathSegment::getPositionAt(double t) {
        return _start.position * (1.0 - t) + _end.position * t;
    }

    glm::dquat PathSegment::getRotationAt(double t) {
        return glm::slerp(_start.rotation, _end.rotation, t);
    }

    // TODO move to cpp file
    // accessors 
    CameraState PathSegment::start() const { return _start; }
    CameraState PathSegment::end() const { return _end; }
    double PathSegment::duration() const { return _duration; }
    double PathSegment::startTime() const { return _startTime; }

} // namespace openspace::autonavigation
