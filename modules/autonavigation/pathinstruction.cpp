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

#include <openspace/documentation/verifier.h>

namespace {
    constexpr const char* _loggerCat = "PathInstruction";

    constexpr const char* KeyInstructions = "Instructions";
    constexpr const char* KeyTarget = "Target";
    constexpr const char* KeyDuration = "Duration";
} // namespace

namespace openspace::autonavigation {

PathInstruction::PathInstruction(const ghoul::Dictionary& dictionary) {
    const bool hasTarget = dictionary.hasValue<std::string>(KeyTarget);
    if (!hasTarget) {
        throw ghoul::RuntimeError(
            "A camera path instruction requires a target node, to go to or use as reference frame."
        );
    }

    targetNode = dictionary.value<std::string>(KeyTarget);

    if (dictionary.hasValue<double>(KeyDuration)) {
        duration = dictionary.value<double>(KeyDuration);
    } 
    else {
        duration = 5.0; // TODO: handle better. 
    }
}

PathInstruction::PathInstruction(std::string node, double duration) 
    : targetNode(std::move(node)), duration(duration) 
{}

ghoul::Dictionary PathInstruction::dictionary() const {
    ghoul::Dictionary instructionDict;
    instructionDict.setValue(KeyTarget, targetNode);
    instructionDict.setValue(KeyDuration, duration);

    return instructionDict;
}

documentation::Documentation PathInstruction::Documentation() {
    using namespace documentation;

    return {
        "Path Instruction",
        "camera_path_instruction",
        {
            {
                KeyTarget,
                new StringVerifier,
                Optional::No,
                "The identifier of the target node."
            },
            {
                KeyDuration,
                new DoubleVerifier,
                Optional::Yes,
                "The desired duration for the camera movement."
            },
        }
    };
}

} // namespace openspace::autonavigation
