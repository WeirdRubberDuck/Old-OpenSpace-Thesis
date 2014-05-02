/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014                                                                    *
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

// open space includes
#include <openspace/rendering/renderablevolumegl.h>

#include <openspace/engine/openspaceengine.h>

#include <ghoul/opengl/texturereader.h>
#include <ghoul/opencl/clworksize.h>
#include <ghoul/filesystem/filesystem.h>

#include <sgct.h>

#include <algorithm>

namespace {
    std::string _loggerCat = "RenderableVolumeGL";
}

namespace openspace {

RenderableVolumeGL::RenderableVolumeGL(const ghoul::Dictionary& dictionary):
    RenderableVolume(dictionary),
    _backTexture(nullptr), _frontTexture(nullptr), _screenQuad(0),
    _programUpdateOnSave(false) {
        
    _shaderMutex = new std::mutex;
    
    _filename = "";
    if(dictionary.hasKey("Volume")) {
        if(dictionary.getValue("Volume", _filename)) {
            _filename = findPath(_filename);
        }
    }
    
    LDEBUG("filename: " << _filename);
    
    ghoul::Dictionary hintsDictionary;
    if(dictionary.hasKey("Hints"))
        dictionary.getValue("Hints", hintsDictionary);
    _hints = readHints(hintsDictionary);

    std::string vshaderpath = "";
    std::string fshaderpath = "";
        
    if (dictionary.hasKey("Shaders")) {
        ghoul::Dictionary shaderDictionary;
        if(dictionary.getValue("Shaders", shaderDictionary)) {
            if (shaderDictionary.hasKey("VertexShader")) {
                shaderDictionary.getValue("VertexShader", vshaderpath);
            }
            if (shaderDictionary.hasKey("FragmentShader")) {
                shaderDictionary.getValue("FragmentShader", fshaderpath);
            }
            
            vshaderpath = findPath(vshaderpath);
            fshaderpath = findPath(fshaderpath);
            
            _vertexSourceFile = new ghoul::filesystem::File(vshaderpath, false);
            _fragmentSourceFile = new ghoul::filesystem::File(fshaderpath, false);
            
            _twopassProgram = new ghoul::opengl::ProgramObject("TwoPassProgram");
            ghoul::opengl::ShaderObject* vertexShader = new ghoul::opengl::ShaderObject(ghoul::opengl::ShaderObject::ShaderTypeVertex,vshaderpath);
            ghoul::opengl::ShaderObject* fragmentShader = new ghoul::opengl::ShaderObject(ghoul::opengl::ShaderObject::ShaderTypeFragment,fshaderpath);
            _twopassProgram->attachObject(vertexShader);
            _twopassProgram->attachObject(fragmentShader);
        }
        
        
    }
    
    if(dictionary.hasKey("UpdateOnSave")) {
        dictionary.getValue("UpdateOnSave", _programUpdateOnSave);
    }

}

RenderableVolumeGL::~RenderableVolumeGL() {
    deinitialize();
    if(_fbo)
        delete _fbo;
    if(_backTexture)
        delete _backTexture;
    if(_frontTexture)
        delete _frontTexture;
    if(_volume)
        delete _volume;
    if(_boundingBox)
        delete _boundingBox;
}

bool RenderableVolumeGL::initialize() {
    assert(_filename != "");
    //	------ VOLUME READING ----------------
	ghoul::RawVolumeReader rawReader(_hints);
	_volume = rawReader.read(_filename);
    
    //	------ SETUP GEOMETRY ----------------
	const GLfloat size = 1.0f;
	const GLfloat vertex_texcoord_data[] = { // square of two triangles (sigh)
        //	  x      y     z     s     t
        -size, -size, 0.0f, 0.0f, 0.0f,
        size,	size, 0.0f, 1.0f, 1.0f,
        -size,  size, 0.0f, 0.0f, 1.0f,
        -size, -size, 0.0f, 0.0f, 0.0f,
        size, -size, 0.0f, 1.0f, 0.0f,
        size,	size, 0.0f, 1.0f, 1.0f
    };
    
	GLuint vertexPositionBuffer;
	glGenVertexArrays(1, &_screenQuad); // generate array
	glBindVertexArray(_screenQuad); // bind array
	glGenBuffers(1, &vertexPositionBuffer); // generate buffer
	glBindBuffer(GL_ARRAY_BUFFER, vertexPositionBuffer); // bind buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_texcoord_data), vertex_texcoord_data, GL_STATIC_DRAW);
    
	// Vertex positions
	GLuint vertexLocation = 2;
	glEnableVertexAttribArray(vertexLocation);
	glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), reinterpret_cast<void*>(0));
    
	// Texture coordinates
	GLuint texcoordLocation = 0;
	glEnableVertexAttribArray(texcoordLocation);
	glVertexAttribPointer(texcoordLocation, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));
    
	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind buffer
	glBindVertexArray(0); //unbind array
    
	_boundingBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::Regular);
    
    //	------ SETUP SHADERS -----------------
    // TODO error control or better design pattern
	OsEng.ref().configurationManager().getValue("RaycastProgram", _fboProgram);
    
    auto privateCallback = [this](const ghoul::filesystem::File& file) {
        safeShaderCompilation();
    };
    if(_programUpdateOnSave) {
        _vertexSourceFile->setCallback(privateCallback);
        _fragmentSourceFile->setCallback(privateCallback);
    }
    
    _twopassProgram->compileShaderObjects();
    _twopassProgram->linkProgramObject();
    _twopassProgram->setUniform("texBack", 0);
    _twopassProgram->setUniform("texFront", 1);
    _twopassProgram->setUniform("texVolume", 2);
	//OsEng.ref().configurationManager().getValue("TwoPassProgram", _twopassProgram);
    
    //	------ SETUP FBO ---------------------
	_fbo = new ghoul::opengl::FramebufferObject();
	_fbo->activate();
    
	int x = sgct::Engine::instance()->getActiveXResolution();
	int y = sgct::Engine::instance()->getActiveYResolution();
	_backTexture = new ghoul::opengl::Texture(glm::size3_t(x,y,1));
	_frontTexture = new ghoul::opengl::Texture(glm::size3_t(x,y,1));
	_backTexture->uploadTexture();
	_frontTexture->uploadTexture();
	_fbo->attachTexture(_backTexture, GL_COLOR_ATTACHMENT0);
	_fbo->attachTexture(_frontTexture, GL_COLOR_ATTACHMENT1);
    
	_fbo->deactivate();

    
    return true;
}

bool RenderableVolumeGL::deinitialize() {

    
    return true;
}

void RenderableVolumeGL::render(const Camera *camera, const psc &thisPosition) {
    
	float speed = 50.0f;
	float time = sgct::Engine::getTime();
    glm::mat4 transform = camera->getViewProjectionMatrix();
    
    double factor = pow(10.0,thisPosition[3]);
    transform = glm::translate(transform, glm::vec3(thisPosition[0]*factor, thisPosition[1]*factor, thisPosition[2]*factor));
	transform = glm::rotate(transform, time*speed, glm::vec3(0.0f, 1.0f, 0.0f));
	
        
    _stepSize = 0.01f;
	
    //	------ DRAW TO FBO -------------------
	GLuint sgctFBO = ghoul::opengl::FramebufferObject::getActiveObject(); // Save SGCTs main FBO
	_fbo->activate();
	_fboProgram->activate();
	_fboProgram->setUniform("modelViewProjection", transform);
    
	//	Draw backface
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClearColor(0.2f, 0.2f, 0.2f, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	_boundingBox->draw();
	glDisable(GL_CULL_FACE);
    
	//	Draw frontface
	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.2f, 0.2f, 0.2f, 0);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	_boundingBox->draw();
	glDisable(GL_CULL_FACE);
    
	_fboProgram->deactivate();
	_fbo->deactivate();
    
    //	------ DRAW TO SCREEN ----------------
	glBindFramebuffer(GL_FRAMEBUFFER, sgctFBO); // Re-bind SGCTs main FBO
    
	//	Draw screenquad
	glClearColor(0.2f, 0.2f, 0.2f, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    _shaderMutex->lock();
	_twopassProgram->activate();
	_twopassProgram->setUniform("stepSize", _stepSize);
    
	//	 Set textures
	glActiveTexture(GL_TEXTURE0);
	_backTexture->bind();
	glActiveTexture(GL_TEXTURE1);
	_frontTexture->bind();
	glActiveTexture(GL_TEXTURE2);
	_volume->bind();
    
	glBindVertexArray(_screenQuad);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
    
	_twopassProgram->deactivate();
    _shaderMutex->unlock();

}

void RenderableVolumeGL::update() {
    
}

void RenderableVolumeGL::safeShaderCompilation() {
    _shaderMutex->lock();
    _twopassProgram->rebuildFromFile();
    _twopassProgram->compileShaderObjects();
    _twopassProgram->linkProgramObject();
    _twopassProgram->setUniform("texBack", 0);
    _twopassProgram->setUniform("texFront", 1);
    _twopassProgram->setUniform("texVolume", 2);
    _shaderMutex->unlock();
}
    

	
} // namespace openspace