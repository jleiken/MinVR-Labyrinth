#include "bsg.h"
#include "bsgMenagerie.h"
#include "bsgObjModel.h"

#include "MVRDemo.h"

class DemoVRApp: public MVRDemo {

	// Data values that were global in the demo2.cpp file are defined as
	// private members of the VRApp.
private:

	bsg::scene _scene;

	// These are the shapes that make up the scene.  They are out here in
	// the global variables so they can be available in both the main()
	// function and the renderScene() function.
	bsg::drawableCollection* _board;
	bsg::drawableSphere* _ball;
	bsg::drawableSquare* _win;

	// shader files we're going to need
	std::string _vertexFile;
	std::string _fragmentFile;

	// These are part of the animation stuff, and again are out here with
	// the big boy global variables so they can be available to both the
	// interrupt handler and the render function.
	float _oscillator;
	float _xVelocity;
	float _yVelocity;
	float _zVelocity;
	bool _inited;
	bool _isMoving;

	// Helpful constants
	float BOARD_X_OFFSET, BOARD_Y_OFFSET, BOARD_Z_OFFSET;
	float WAND_X_OFFSET, WAND_Y_OFFSET, WAND_Z_OFFSET;
	float GRAVITY;
	int _NUM_HOLES;
	int _NUM_WALLS;

	// This contains a bunch of sanity checks from the graphics
	// initialization of demo2.cpp.  They are still useful with MinVR.
	void _checkContext() {
		glewExperimental = true; // Needed for core profile
		if (glewInit() != GLEW_OK) {
			throw std::runtime_error("Failed to initialize GLEW");
		}

		// Now that we have a graphics context, let's look at what's inside.
		std::cout << "Hardware check: "
							<< glGetString(GL_RENDERER)  // e.g. Intel 3000 OpenGL Engine
							<< " / "
							<< glGetString(GL_VERSION)    // e.g. 3.2 INTEL-8.0.61
							<< std::endl;

		if (glewIsSupported("GL_VERSION_2_1")) {
			std::cout << "Software check: Ready for OpenGL 2.1." << std::endl;
		} else {
			throw std::runtime_error("Software check: OpenGL 2.1 not supported.");
		}

		// This is the background color of the viewport.
		// (gray)
		glClearColor(0.75, 0.75, 0.75, 1.0);

		// Now we're ready to start issuing OpenGL calls.  Start by enabling
		// the modes we want.  The DEPTH_TEST is how you get hidden faces.
		glEnable(GL_DEPTH_TEST);

		if (glIsEnabled(GL_DEPTH_TEST)) {
			std::cout << "Depth test enabled" << std::endl;
		} else {
			std::cout << "No depth test enabled" << std::endl;
		}

		// This is just a performance enhancement that allows OpenGL to
		// ignore faces that are facing away from the camera.
		glEnable(GL_CULL_FACE);
		glLineWidth(4);
		glEnable(GL_LINE_SMOOTH);
	}

	// Just a little debug function so that a user can see what's going on
	// in a non-graphical sense.
	void _showCameraPosition() {

		std::cout << "Camera is at ("
							<< _scene.getCameraPosition().x << ", "
							<< _scene.getCameraPosition().y << ", "
							<< _scene.getCameraPosition().z << ")... ";
		std::cout << "looking at ("
							<< _scene.getLookAtPosition().x << ", "
							<< _scene.getLookAtPosition().y << ", "
							<< _scene.getLookAtPosition().z << ")." << std::endl;
	}

	void _initializeScene() {
		// initialize the shaders and lights
		bsg::bsgPtr<bsg::shaderMgr> _boardShader = new bsg::shaderMgr();
		bsg::bsgPtr<bsg::shaderMgr> _holeShader = new bsg::shaderMgr();
		bsg::bsgPtr<bsg::shaderMgr> _winShader = new bsg::shaderMgr();
		bsg::bsgPtr<bsg::shaderMgr> _ballShader = new bsg::shaderMgr();
		bsg::bsgPtr<bsg::lightList> _lights = new bsg::lightList();

		// Create a list of lights
		_lights->addLight(glm::vec4(BOARD_X_OFFSET, BOARD_Y_OFFSET + 15, BOARD_Z_OFFSET, 1.0f),
						  glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
		_lights->addLight(glm::vec4(BOARD_X_OFFSET - 5, BOARD_Y_OFFSET + 15, BOARD_Z_OFFSET - 5, 1.0f),
						  glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
		_lights->addLight(glm::vec4(BOARD_X_OFFSET + 5, BOARD_Y_OFFSET + 15, BOARD_Z_OFFSET + 5, 1.0f),
						  glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));

		_boardShader->addLights(_lights);
		_holeShader->addLights(_lights);
		_winShader->addLights(_lights);
		_ballShader->addLights(_lights);

		_boardShader->addShader(bsg::GLSHADER_VERTEX, _vertexFile);
		_boardShader->addShader(bsg::GLSHADER_FRAGMENT, _fragmentFile);
		_holeShader->addShader(bsg::GLSHADER_VERTEX, _vertexFile);
		_holeShader->addShader(bsg::GLSHADER_FRAGMENT, _fragmentFile);
		_winShader->addShader(bsg::GLSHADER_VERTEX, _vertexFile);
		_winShader->addShader(bsg::GLSHADER_FRAGMENT, _fragmentFile);
		_ballShader->addShader(bsg::GLSHADER_VERTEX, _vertexFile);
		_ballShader->addShader(bsg::GLSHADER_FRAGMENT, _fragmentFile);

		// The shaders are loaded, now compile them.
		_boardShader->compileShaders();
		_holeShader->compileShaders();
		_winShader->compileShaders();
		_ballShader->compileShaders();

		// Add a texture (just color for now) to the board objects
		bsg::bsgPtr<bsg::textureMgr> boardTexture = new bsg::textureMgr();
		boardTexture->readFile(bsg::texturePNG, "../data/board.png");
		_boardShader->addTexture(boardTexture);
		glm::vec4 boardColor = glm::vec4(0.549f, 0.408f, 0.263f, 1);

		_board = new bsg::drawableCollection();

		int x_offset, z_offset;
		for (int i = 0; i < _NUM_WALLS; i++) {
			bsg::drawableCube* x = new bsg::drawableCube(_boardShader, 25, boardColor);
			x->setScale(glm::vec3(1, 2, 1));
			x_offset = rand() % 28;
			z_offset = rand() % 28;
			x->setPosition(-14.0f + x_offset, 1, -14.0f + z_offset);
			_board->addObject(x);
		}

		// Add a texture to the holes
		bsg::bsgPtr<bsg::textureMgr> holeTexture = new bsg::textureMgr();
		holeTexture->readFile(bsg::texturePNG, "../data/hole.png");
		_holeShader->addTexture(holeTexture);

		float y_offset = 0.1f;
		for (int i = 0; i < _NUM_HOLES; i++) {
			bsg::drawableCircle* x = new bsg::drawableCircle(_holeShader, 25, 1.0f, 0);
			x->setScale(glm::vec3(2.0f, 1.0f, 2.0f));
			x_offset = rand() % 28;
			z_offset = rand() % 28;
			x->setPosition(-14.0f + x_offset, y_offset, -14.0f + z_offset);
			_board->addObject(x);
		}

		bsg::bsgPtr<bsg::textureMgr> winTexture = new bsg::textureMgr();
		winTexture->readFile(bsg::texturePNG, "../data/win.png");
		_winShader->addTexture(winTexture);
		x_offset = rand() % 25;
		y_offset = 0.2f;
		z_offset = rand() % 25;
		_win = new bsg::drawableSquare(_winShader, 25, 
				glm::vec3(-13.0f + x_offset, y_offset, -13.0f + z_offset),
				glm::vec3(-13.0f + x_offset, y_offset, -13.0f + z_offset + 5),
				glm::vec3(-13.0f + x_offset + 5, y_offset, -13.0f + z_offset),
				glm::vec4(0, 1, 0, 1));
		_board->addObject(_win);

		// Add the board outline and the 3D base so coliders work
 		bsg::drawableCube* wWall = new bsg::drawableCube(_boardShader, 25, boardColor);
		wWall->setPosition(-15, 1, 0);
		wWall->setScale(glm::vec3(1, 2, 30));
		wWall->setRotation(0, 0, 0);
		_board->addObject(wWall);
		bsg::drawableCube* eWall = new bsg::drawableCube(_boardShader, 25, boardColor);
		eWall->setPosition(15, 1, 0);
		eWall->setScale(glm::vec3(1, 2, 30));
		_board->addObject(eWall);
		bsg::drawableCube* sWall = new bsg::drawableCube(_boardShader, 25, boardColor);
		sWall->setPosition(0, 1, 15);
		sWall->setScale(glm::vec3(31, 2, 1));
		_board->addObject(sWall);
		bsg::drawableCube* nWall = new bsg::drawableCube(_boardShader, 25, boardColor);
		nWall->setPosition(0, 1, -15);
		nWall->setScale(glm::vec3(31, 2, 1));
		_board->addObject(nWall);
		bsg::drawableSquare* labPlane = new bsg::drawableSquare(_boardShader, 25,
										glm::vec3(-16, 0, -15),
										glm::vec3(-15, 0, 15),
										glm::vec3(15, 0, -15),
										boardColor);
		labPlane->setName("plane");
		labPlane->setPosition(0, 0, 0);
		_board->addObject(labPlane);

		_board->setPosition(BOARD_X_OFFSET, BOARD_Y_OFFSET, BOARD_Z_OFFSET);
		_board->setRotation(0, 0, 0);
		_scene.addObject(_board);

		// make a new shader for the ball
		bsg::bsgPtr<bsg::textureMgr> ballTexture = new bsg::textureMgr();
		ballTexture->readFile(bsg::texturePNG, "../data/ball.png");
		_ballShader->addTexture(ballTexture);

		x_offset = (rand() % 20) - 10;
		z_offset = (rand() % 20) - 10;
		_ball = new bsg::drawableSphere(_ballShader, 25, 25, glm::vec4(0.5f, 0.5f, 0.5f, 0.0f));
		_ball->setScale(glm::vec3(1.5f, 1.5f, 1.5f));
		_ball->setPosition(BOARD_X_OFFSET + x_offset, BOARD_Y_OFFSET + 10, BOARD_Z_OFFSET + z_offset);
		_scene.addObject(_ball);

		// set inited to true so other functions know we've been initialized
		_inited = true;
	}

	float _keepRotationLow(float rot) {
		if (rot < -0.33) {
			return -0.33;
		} else if (rot > 0.33) {
			return 0.33;
		} else {
			return rot;
		}
	}

	void loser(void) {
		_ball->setPosition(BOARD_X_OFFSET, BOARD_Y_OFFSET - 15, BOARD_Z_OFFSET);
		_isMoving = false;
	}

	void winner(void) {
		_ball->setPosition(BOARD_X_OFFSET, BOARD_Y_OFFSET + 10, BOARD_Z_OFFSET);
		_isMoving = false;
	}

	bool insideCustomBoundingBox(const glm::vec4 &testPoint,
								 glm::mat4 modelMatrix,
                                 bsg::drawableObj* obj,
								 bool is2dObj) {
		glm::vec4 lower = modelMatrix * obj->getBoundingBoxLower();
		glm::vec4 upper = modelMatrix * obj->getBoundingBoxUpper();

		if (is2dObj) {
			return
				(testPoint.x <= upper.x) &&
				(testPoint.x >= lower.x) &&
				(testPoint.z <= upper.z) &&
				(testPoint.z >= lower.z) &&
				((std::abs(testPoint.y - lower.y) <= 0.75) || 
				(std::abs(testPoint.y - upper.y) <= 0.75));
		} else {
			return
				(testPoint.x <= upper.x) &&
				(testPoint.x >= lower.x) &&
				(testPoint.y <= upper.y) &&
				(testPoint.y >= lower.y) &&
				(testPoint.z <= upper.z) &&
				(testPoint.z >= lower.z);
		}
	}


public:
	DemoVRApp(int argc, char** argv) 
	: MVRDemo(argc, argv) {
		// This is the root of the scene graph.
		bsg::scene _scene = bsg::scene();

		_vertexFile = "../shaders/textureShader.vp";
		_fragmentFile = "../shaders/textureShader.fp";

		_oscillator = 0.0f;
		_yVelocity = 0.0f;

		BOARD_X_OFFSET = -5.0;
		BOARD_Y_OFFSET = -10.0;
		BOARD_Z_OFFSET = -20.0;
		WAND_X_OFFSET = -10.0;
		WAND_Y_OFFSET = -12.0;
		WAND_Z_OFFSET = -35.0;
		GRAVITY = 0.005f;
		_NUM_WALLS = 80;
		_NUM_HOLES = 10;
		_inited = false;
		_isMoving = true;
	}

	/// The MinVR apparatus invokes this method whenever there is a new
	/// event to process.
	void onVREvent(const MinVR::VREvent &event) {
		// if (event.getName() != "FrameStart") {
		// 	std::cout << "Hearing event:" << event << std::endl;
		// }
		if (event.getName().find("Wand") != -1 && event.getName().find("Joystick") == -1) {
		}
		if (event.getName().find("Wand") != -1 && event.getName().find("Move") && _inited) {
			// the user is holding the activate tilt button and is moving
			MinVR::VRFloatArray arr = event.getValue("Transform");
			glm::mat4 mat = glm::mat4(arr[0], arr[1], arr[2], arr[3],
									  arr[4], arr[5], arr[6], arr[7],
									  arr[8], arr[9], arr[10], arr[11],
									  arr[12], arr[13], arr[14], arr[15]);
			// apply all transformations
			// positions are at 12,13,14
			_board->setPosition(arr[12]+WAND_X_OFFSET,
								arr[13]+WAND_Y_OFFSET,
								arr[14]+WAND_Z_OFFSET);
			// rotation is at 0,1,2 and 4,5,6 and 8,9,10?
			glm::vec3 rot = glm::vec3(atan2(mat[1][0], mat[0][0]), atan2(-mat[2][0], sqrt(mat[2][1] * mat[2][1] + mat[2][2] * mat[2][2])), atan2(mat[2][1], mat[2][2]));
			float actualZ = -rot.x;
			rot.x = -rot.z;
			rot.z = actualZ;
			// float x = _keepRotationLow(rot.x);
			// float y = _keepRotationLow(rot.y);
			// float z = _keepRotationLow(rot.z);
			_board->setRotation(rot);
		} else if (event.getName() == "KbdEsc_Down") {
			// Quit if the escape button is pressed
			shutdown();
		} else if (event.getName() == "FrameStart") {
			// The "FrameStart" heartbeat event recurs at regular intervals,
			// so you can do animation here, as well as in the render
			// function.
			_oscillator = event.getValue("ElapsedSeconds");
		}
	}

	/// \brief Set the render context.
	///
	/// The onVRRender methods are the heart of the MinVR rendering
	/// apparatus.  Some render calls are shared among multiple views,
	/// for example a stereo view has two renders, with the same render
	/// context.
	void onVRRenderContext(const VRState &renderState) {

		// Check if this is the first call.  If so, do some initialization.
		if ((int)renderState.getValue("InitRender") == 1) {
			_checkContext();
			_initializeScene();

			// Make any initializations necessary for the scene and its shaders.
			_scene.prepare();
		}

	 	// Load the scene models to the GPU.
		_scene.load();
	}

	/// \brief Draw the image.
	///
	/// This is the heart of any graphics program, the render function.
	/// It is called each time through the main graphics loop, and
	/// re-draws the scene according to whatever has changed since the
	/// last time it was drawn.
	void onVRRenderScene(const VRState &renderState) {
			// Make the ball fall
			if (_isMoving) {
				glm::vec3 loc = _ball->getPosition();
				loc.x += _xVelocity;
				loc.y += _yVelocity;
				loc.z += _zVelocity;
				_ball->setPosition(loc.x, loc.y, loc.z);
				glm::vec3 boardRot = _board->getPitchYawRoll();
				float sinX = sin(boardRot.x), sinZ = sin(boardRot.z);
				if (sinX != 0) {
					_xVelocity += (sinX - GRAVITY)*0.1;
				} else if (sinZ != 0) {
					_zVelocity += (sinZ - GRAVITY)*0.1;
				}
				_yVelocity -= GRAVITY;

				glm::vec4 loc4;
				loc4.x = loc.x;
				loc4.y = loc.y;
				loc4.z = loc.z;

				// check if the ball has fallen into the board
				for (bsg::drawableCollection::iterator comp = _board->begin(); comp != _board->end(); comp++) {
					bsg::bsgPtr<bsg::drawableMulti> multi = comp->second;
					bsg::DrawableObjList lst = multi->getDrawableObjList();
					bool isWin = multi->printObj("").find("square") != -1;
					bool isHole = multi->printObj("").find("circle") != -1;
					bool isPlane = multi->printObj("").find("plane") != -1;
					bool is2d =  isWin || isHole || isPlane;
					for (bsg::DrawableObjList::iterator it = lst.begin(); it != lst.end(); it++) {
						bsg::drawableObj* obj = it->ptr();
						if (insideCustomBoundingBox(loc4, multi->getModelMatrix(), obj, is2d)) {
							if (isPlane) {
								_yVelocity = 0;
								// glm::vec3 ballPos = _ball->getPosition();
								// _ball->setPosition(ballPos.x, multi->getPosition().y, ballPos.z);
							} else {
								_xVelocity = 0, _zVelocity = 0;
							}
							if (isWin) {
								winner();
							} else if (isHole) {
								loser();
							}
						}
					}
				}
			}
			if (_ball->getPosition().y < -30) {
				loser();
			}

			// Now draw the scene
			// First clear the display.
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			// We let MinVR give us the projection matrix from the render
			// state argument to this method.
			std::vector<float> pm = renderState.getValue("ProjectionMatrix");
			glm::mat4 projMatrix = glm::mat4( pm[0],  pm[1], pm[2], pm[3],
											pm[4],  pm[5], pm[6], pm[7],
											pm[8],  pm[9],pm[10],pm[11],
											pm[12],pm[13],pm[14],pm[15]);

			// The draw step.  We let MinVR give us the view matrix.
			std::vector<float> vm = renderState.getValue("ViewMatrix");
			glm::mat4 viewMatrix = glm::mat4( vm[0],  vm[1], vm[2], vm[3],
											vm[4],  vm[5], vm[6], vm[7],
											vm[8],  vm[9],vm[10],vm[11],
											vm[12],vm[13],vm[14],vm[15]);

			//bsg::bsgUtils::printMat("view", viewMatrix);
			_scene.draw(viewMatrix, projMatrix);

			// We let MinVR swap the graphics buffers.
			// glutSwapBuffers();
		}
};

// The main function is just a shell of its former self.  Just
// initializes a MinVR graphics object and runs it.
int main(int argc, char **argv) {
	// Is the MINVR_ROOT variable set?  MinVR usually needs this to find
	// some important things.
	if (getenv("MINVR_ROOT") == NULL) {
		std::cout << "***** No MINVR_ROOT -- MinVR might not be found *****" << std::endl
							<< "MinVR is found (at runtime) via the 'MINVR_ROOT' variable."
							<< std::endl << "Try 'export MINVR_ROOT=/my/path/to/MinVR'."
							<< std::endl;
	}


	// Initialize the app.
	DemoVRApp app(argc, argv);

	// Run it.
	app.run();

	// We never get here.
	return 0;
}
