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

	// shader files we're going to need
	std::string _vertexFile;
	std::string _fragmentFile;

	// These are part of the animation stuff, and again are out here with
	// the big boy global variables so they can be available to both the
	// interrupt handler and the render function.
	float _oscillator;
	float _ballVelocity;
	bool _inited;

	// Helpful constants
	float _BOARD_X_OFFSET, _BOARD_Y_OFFSET, _BOARD_Z_OFFSET;
	float _WAND_X_OFFSET, _WAND_Y_OFFSET, _WAND_Z_OFFSET;
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
		_lights->addLight(glm::vec4(_BOARD_X_OFFSET, _BOARD_Y_OFFSET + 15, _BOARD_Z_OFFSET, 1.0f),
						  glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
		_lights->addLight(glm::vec4(_BOARD_X_OFFSET - 5, _BOARD_Y_OFFSET + 15, _BOARD_Z_OFFSET - 5, 1.0f),
						  glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
		_lights->addLight(glm::vec4(_BOARD_X_OFFSET + 5, _BOARD_Y_OFFSET + 15, _BOARD_Z_OFFSET + 5, 1.0f),
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

		_board = new bsg::drawableCollection();

		int x_offset, z_offset;
		for (int i = 0; i < _NUM_WALLS; i++) {
			bsg::drawableObjModel* x =
				new bsg::drawableObjModel(_boardShader, "../data/mid-wall.obj");
			x_offset = rand() % 30;
			z_offset = rand() % 30;
			x->setPosition(-15.0f + x_offset, 0, -15.0f + z_offset);
			_board->addObject(x);
		}

		// Add a texture to the holes
		bsg::bsgPtr<bsg::textureMgr> holeTexture = new bsg::textureMgr();
		holeTexture->readFile(bsg::texturePNG, "../data/hole.png");
		_holeShader->addTexture(holeTexture);

		float y_offset = _BOARD_Y_OFFSET+20.1f;
		for (int i = 0; i < _NUM_HOLES; i++) {
			bsg::drawableCircle* x = new bsg::drawableCircle(_holeShader, 25, 1.0f, _BOARD_Y_OFFSET);
			x->setScale(glm::vec3(2.0f, 1.0f, 2.0f));
			x_offset = rand() % 25;
			z_offset = rand() % 25;
			x->setPosition(-15.0f + x_offset, y_offset, -15.0f + z_offset);
			_board->addObject(x);
		}

		bsg::bsgPtr<bsg::textureMgr> winTexture = new bsg::textureMgr();
		winTexture->readFile(bsg::texturePNG, "../data/win.png");
		_winShader->addTexture(winTexture);
		x_offset = rand() % 25;
		z_offset = rand() % 25;
		y_offset = _BOARD_Y_OFFSET+10.2f;
		bsg::drawableSquare* x = new bsg::drawableSquare(_winShader, 25, 
				glm::vec3(-15.0f + x_offset, y_offset, -15.0f + z_offset),
				glm::vec3(-15.0f + x_offset, y_offset, -15.0f + z_offset + 5),
				glm::vec3(-15.0f + x_offset + 5, y_offset, -15.0f + z_offset),
				glm::vec4(0, 1, 0, 1));
		_board->addObject(x);

		// Might potentially need to change the shader here
 		bsg::drawableObjModel* labPlane = new bsg::drawableObjModel(_boardShader, "../data/lab-plane.obj");
		labPlane->setPosition(0, 0, 0);
		_board->addObject(labPlane);

		_board->setPosition(_BOARD_X_OFFSET, _BOARD_Y_OFFSET, _BOARD_Z_OFFSET);
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
		_ball->setPosition(_BOARD_X_OFFSET + x_offset, _BOARD_Y_OFFSET + 10, _BOARD_Z_OFFSET + z_offset);
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


public:
	DemoVRApp(int argc, char** argv) 
	: MVRDemo(argc, argv) {
		// This is the root of the scene graph.
		bsg::scene _scene = bsg::scene();

		_vertexFile = "../shaders/textureShader.vp";
		_fragmentFile = "../shaders/textureShader.fp";

		_oscillator = 0.0f;
		_ballVelocity = 0.0f;

		_BOARD_X_OFFSET = -5.0;
		_BOARD_Y_OFFSET = -10.0;
		_BOARD_Z_OFFSET = -20.0;
		_WAND_X_OFFSET = -10.0;
		_WAND_Y_OFFSET = -12.0;
		_WAND_Z_OFFSET = -35.0;
		_NUM_WALLS = 80;
		_NUM_HOLES = 10;
		_inited = false;
	}

	/// The MinVR apparatus invokes this method whenever there is a new
	/// event to process.
	void onVREvent(const MinVR::VREvent &event) {
		// if (event.getName() != "FrameStart") {
		// 	std::cout << "Hearing event:" << event << std::endl;
		// }
		if (event.getName() == "Wand_Move" && _inited) {
			// the user is holding the activate tilt button and is moving
			MinVR::VRFloatArray arr = event.getValue("Transform");
			for (int i = 0; i < arr.size(); i++) {
				std::cout << "Wand Move: " << i << " : " << arr[i] << std::endl;
			}
			// positions are at 12,13,14
			_board->setPosition(arr[12]+_WAND_X_OFFSET,
								arr[13]+_WAND_Y_OFFSET,
								arr[14]+_WAND_Z_OFFSET);
			// rotation is at 0,1,2 and 4,5,6 and 8,9,10?
			float x = _keepRotationLow(-arr[8]);
			float z = _keepRotationLow(arr[0]);
			_board->setRotation(x, 0, z);
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
			// Make scene changes here
			glm::vec3 loc = _ball->getPosition();
			_ball->setPosition(loc.x, loc.y + _ballVelocity, loc.z);
			_ballVelocity -= 0.005f;

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
