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

	// These are part of the animation stuff, and again are out here with
	// the big boy global variables so they can be available to both the
	// interrupt handler and the render function.
	float _oscillator;

	// These variables were not global before, but their scope has been
	// divided into several functions here, so they are class-wide
	// private data objects.
	bsg::bsgPtr<bsg::shaderMgr> _boardShader;
	bsg::bsgPtr<bsg::shaderMgr> _ballShader;
	bsg::bsgPtr<bsg::lightList> _lights;

	// Here are the drawable objects that make up the compound object
	// that make up the scene.
	bsg::drawableObj _topShape;
	bsg::drawableObj _bottomShape;

	std::string _vertexFile;
	std::string _fragmentFile;

	// Helpful constants
	float _X_BOARD_OFFSET;
	float _Y_BOARD_OFFSET;
	float _Z_BOARD_OFFSET;

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

		// Create a list of lights.  If the shader you're using doesn't use
		// lighting, and the shapes don't have textures, this is irrelevant.
		_lights->addLight(glm::vec4(0.0f, 0.0f, 3.0f, 1.0f),
											glm::vec4(1.0f, 1.0f, 0.0f, 0.0f));

		_boardShader->addLights(_lights);


		_vertexFile = "../shaders/shader2.vp";
		_fragmentFile = "../shaders/shader.fp";

		_boardShader->addShader(bsg::GLSHADER_VERTEX, _vertexFile);
		_boardShader->addShader(bsg::GLSHADER_FRAGMENT, _fragmentFile);

		// The shaders are loaded, now compile them.
		_boardShader->compileShaders();

		// Add a texture to our shader manager object.
		bsg::bsgPtr<bsg::textureMgr> texture = new bsg::textureMgr();

		texture->readFile(bsg::texturePNG, "../data/board-color.png");
		_boardShader->addTexture(texture);

		_board = new bsg::drawableCollection();

		glm::vec4* boardColor = new glm::vec4(0.6f, 0.4f, 0.2f, 0.0f);

		int x_offset, z_offset;
		for (int i = 0; i < 75; i++) {
			bsg::drawableObjModel* x =
				new bsg::drawableObjModel(_boardShader, "../data/mid-wall.obj");
			x_offset = rand() % 30;
			z_offset = rand() % 30;
			x->setPosition(glm::vec3(-15.0f + x_offset, 0.5f, -15.0f + z_offset));
			_board->addObject(x);
		}

		// Might potentially need to change the shader here
 		bsg::drawableObjModel* labPlane = new bsg::drawableObjModel(_boardShader, "../data/lab-plane.obj");
		labPlane->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
		_board->addObject(labPlane);

		_board->setPosition(glm::vec3(_X_BOARD_OFFSET, _Y_BOARD_OFFSET, _Z_BOARD_OFFSET));
		_board->setRotation(0.0, 0.0, 0.0);
		_scene.addObject(_board);

		// make a new shader for the ball
		std::cout << "pre ball" << endl;
		_ballShader->addShader(bsg::GLSHADER_VERTEX, _vertexFile);
		_ballShader->addShader(bsg::GLSHADER_FRAGMENT, _fragmentFile);
		_ballShader->compileShaders();
		std::cout << "post compile" << endl;
		texture = new bsg::textureMgr();
		texture->readFile(bsg::texturePNG, "../data/ball-color.png");
		_ballShader->addTexture(texture);
		std::cout << "post texture" << endl;

		x_offset = (rand() % 20) - 10;
		z_offset = (rand() % 20) - 10;
		_ball = new bsg::drawableSphere(_ballShader, 25, 25, glm::vec4(0.5f, 0.5f, 0.5f, 0.0f));
		_ball->setPosition(glm::vec3(_X_BOARD_OFFSET + x_offset, _Y_BOARD_OFFSET + 10, _Z_BOARD_OFFSET + z_offset));
		_scene.addObject(_ball);
	}


public:
	DemoVRApp(int argc, char** argv) :
		MVRDemo(argc, argv) {
		// This is the root of the scene graph.
		bsg::scene _scene = bsg::scene();

		// These are tracked separately because multiple objects might use them.
		_boardShader = new bsg::shaderMgr();
		_ballShader = new bsg::shaderMgr();
		_lights = new bsg::lightList();

		_oscillator = 0.0f;

		_X_BOARD_OFFSET = -5.0;
		_Y_BOARD_OFFSET = -10.0;
		_Z_BOARD_OFFSET = -20.0;

	}

	/// The MinVR apparatus invokes this method whenever there is a new
	/// event to process.
	void onVREvent(const MinVR::VREvent &event) {

		//std::cout << "Hearing event:" << event << std::endl;

		// The "FrameStart" heartbeat event recurs at regular intervals,
		// so you can do animation here, as well as in the render
		// function.

		// Quit if the escape button is pressed
		if (event.getName() == "KbdEsc_Down") {
			shutdown();
		} else if (event.getName() == "FrameStart") {
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

			// If you want to adjust the positions of the various objects in
			// your scene, you can do that here.

			// Now the preliminaries are done, on to the actual drawing.

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
	argv[0] = (char*)"bin/kbDemoMinVR";
	argv[1] = (char*)"-c";
	argv[2] = (char*)"../config/desktop-freeglut.xml";
	DemoVRApp app(3, argv);

	// Run it.
	app.run();

	// We never get here.
	return 0;
}
