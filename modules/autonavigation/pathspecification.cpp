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

#include <modules/autonavigation/pathspecification.h>

#include <openspace/documentation/verifier.h>

namespace {
    constexpr const char* _loggerCat = "PathInstruction";

    constexpr const char* KeyInstructions = "Instructions";
    constexpr const char* KeyTarget = "Target";
    constexpr const char* KeyDuration = "Duration";
} // namespace

namespace openspace::autonavigation {

PathSpecification::Instruction::Instruction(const ghoul::Dictionary& dictionary) {
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

PathSpecification::Instruction::Instruction(std::string node, double duration)
    : targetNode(std::move(node)), duration(duration) 
{}

ghoul::Dictionary PathSpecification::Instruction::dictionary() const {
    ghoul::Dictionary instructionDict;
    instructionDict.setValue(KeyTarget, targetNode);
    instructionDict.setValue(KeyDuration, duration);

    return instructionDict;
}

documentation::Documentation PathSpecification::Instruction::Documentation() {
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

// ------------------------------------------------------------------------

PathSpecification::PathSpecification(const ghoul::Dictionary& dictionary) {
    ghoul::Dictionary instructions = dictionary.value<ghoul::Dictionary>(KeyInstructions); 

    for (size_t i = 1; i <= instructions.size(); ++i) {
        ghoul::Dictionary ins = instructions.value<ghoul::Dictionary>(std::to_string(i));
        openspace::documentation::TestResult r = openspace::documentation::testSpecification(Instruction::Documentation(), ins);

        if (!r.success) {
            throw ghoul::RuntimeError(
                "Could not read instruction number " + std::to_string(i) + "."
            );
        } 
        else {
            _instructions.push_back(Instruction(ins));
        }
    }
}

PathSpecification::PathSpecification(const std::vector<Instruction> instructions)
    : _instructions(instructions)
{}

const std::vector<PathSpecification::Instruction>& PathSpecification::instructions() const {
    return _instructions;
}

ghoul::Dictionary PathSpecification::dictionary() const {
    ghoul::Dictionary dict;
    dict.setValue(KeyInstructions, _instructions);
    return dict;
}

documentation::Documentation PathSpecification::Documentation() {
    using namespace documentation;

    return {
        "Path Specification",
        "camera_path_specification",
        {
            {
                KeyInstructions,
                new TableVerifier,
                Optional::No,
                "A list of path instructions."
            },
        }
    };
}


} // namespace openspace::autonavigation
