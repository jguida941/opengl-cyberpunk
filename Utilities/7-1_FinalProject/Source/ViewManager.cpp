///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>    
#include <iostream>

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;

	// the following variable is true when the cursor is DISABLED (i.e., 
	// it is hidden and bound to the window); otherwise it is NORMAL 
	// (i.e., unconstrained to the window). 
	bool bCursorDisabled = true;

	// a boolean to note that the TAB key has been pressed (cleared 
	// after a release)
	bool bTabPressed = false;

	// latch key presses so we only flip projection once per tap
	bool bOPressed = false;
	bool bPPressed = false;
	bool bLPressed = false;
	bool bTPressed = false;

	// Allow runtime toggle so grader can verify spotlight behavior live.
	bool bSpotLightEnabled = true;
	// Runtime toggle for secondary/detail texture blending demo.
	bool bDetailBlendEnabled = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
	g_pCamera->MovementSpeed = 20;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// mouse movement drives the look direction
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
	// scroll wheel tweaks movement speed on the fly
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// when the first mouse move event is received, this needs to be recorded so that
	// all subsequent mouse moves can correctly calculate the X position offset and Y
	// position offset for proper operation
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	// calculate the X offset and Y offset values for moving the 3D camera accordingly
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos; // reversed since y-coordinates go from bottom to top

	// set the current positions into the last position variables
	gLastX = xMousePos;
	gLastY = yMousePos;

	// move the 3D camera according to the calculated offsets
	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 *  Mouse_Scroll_Callback()
 *
 *  Let the user slow down or speed up without touching code.
 ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	(void)xOffset;
	if (NULL != g_pCamera)
	{
		g_pCamera->ProcessMouseScroll((float)yOffset);
	}
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}
	
	// enable or disable the cursor to the window
	// code has been updated to release one ball per spacebar press and release
	if (glfwGetKey(m_pWindow, GLFW_KEY_TAB) == GLFW_PRESS)
	{
		bTabPressed = true;
	}
	if (bTabPressed && (glfwGetKey(m_pWindow, GLFW_KEY_TAB) == GLFW_RELEASE))
	{
		bTabPressed = false;
		if (bCursorDisabled)
		{
			glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else 
		{
			glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		// change the variable to the new state
		bCursorDisabled = !bCursorDisabled;
	}

	// process camera zooming in and out
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}

	// process camera panning left and right
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}

	// vertical movement (up/down)
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(UP, gDeltaTime);
	}

	// flip projection on key release to avoid rapid toggling
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
	{
		bOPressed = true;
	}
	if (bOPressed && (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_RELEASE))
	{
		bOPressed = false;
		bOrthographicProjection = true;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{
		bPPressed = true;
	}
	if (bPPressed && (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_RELEASE))
	{
		bPPressed = false;
		bOrthographicProjection = false;
	}

	// L toggles spotlight on/off at runtime.
	if (glfwGetKey(m_pWindow, GLFW_KEY_L) == GLFW_PRESS)
	{
		bLPressed = true;
	}
	if (bLPressed && (glfwGetKey(m_pWindow, GLFW_KEY_L) == GLFW_RELEASE))
	{
		bLPressed = false;
		bSpotLightEnabled = !bSpotLightEnabled;
		std::cout << "[Toggle] Spotlight: " << (bSpotLightEnabled ? "ON" : "OFF") << std::endl;
	}

	// T toggles detail texture blending so the difference is easy to show live.
	if (glfwGetKey(m_pWindow, GLFW_KEY_T) == GLFW_PRESS)
	{
		bTPressed = true;
	}
	if (bTPressed && (glfwGetKey(m_pWindow, GLFW_KEY_T) == GLFW_RELEASE))
	{
		bTPressed = false;
		bDetailBlendEnabled = !bDetailBlendEnabled;
		std::cout << "[Toggle] Secondary texture mode: " << (bDetailBlendEnabled ? "ON" : "OFF") << std::endl;
	}
}

/***********************************************************
 *  IsOrthographic()
 *
 *  Returns the current projection mode flag.
 ***********************************************************/
bool ViewManager::IsOrthographic() const
{
	return bOrthographicProjection;
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;
	glm::vec3 viewPosition;

	int width = 0;
	int height = 0;

	// keep viewport matched to the framebuffer (resize/DPI safe)
	glfwGetFramebufferSize(m_pWindow, &width, &height);
	glViewport(0, 0, width, height);

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	float aspect = (height == 0) ? 1.0f : (float)width / (float)height;

	if (bOrthographicProjection == true)
	{
		// fixed ortho camera so the 2D view stays put
			glm::vec3 orthoPosition = glm::vec3(0.0f, 1.05f, 8.0f);
			glm::vec3 orthoTarget = glm::vec3(0.0f, 1.05f, 0.0f);
		view = glm::lookAt(orthoPosition, orthoTarget, glm::vec3(0.0f, 1.0f, 0.0f));

		const float orthoHeight = 3.0f;
		const float orthoWidth = orthoHeight * aspect;
		projection = glm::ortho(-0.5f * orthoWidth, 0.5f * orthoWidth, 0.0f, orthoHeight, 0.1f, 100.0f);
		viewPosition = orthoPosition;
	}
	else
	{
		// perspective view follows the free camera
		view = g_pCamera->GetViewMatrix();
		projection = glm::perspective(glm::radians(g_pCamera->Zoom), aspect, 0.1f, 100.0f);
		viewPosition = g_pCamera->Position;
	}

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", viewPosition);

		// Spotlight follows the camera like a small flashlight.
		glm::vec3 spotDirection = bOrthographicProjection ? glm::vec3(0.0f, -0.12f, -1.0f) : g_pCamera->Front;
		m_pShaderManager->setVec3Value("spotLight.position", viewPosition);
		m_pShaderManager->setVec3Value("spotLight.direction", glm::normalize(spotDirection));
		m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
		m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
		m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
		m_pShaderManager->setFloatValue("spotLight.linear", 0.09f);
		m_pShaderManager->setFloatValue("spotLight.quadratic", 0.032f);
		m_pShaderManager->setVec3Value("spotLight.ambient", 0.01f, 0.01f, 0.01f);
		m_pShaderManager->setVec3Value("spotLight.diffuse", 0.40f, 0.38f, 0.34f);
		m_pShaderManager->setVec3Value("spotLight.specular", 0.26f, 0.24f, 0.20f);
		m_pShaderManager->setBoolValue("spotLight.bActive", bSpotLightEnabled);

		// This controls whether detail texture blending is visible.
		m_pShaderManager->setFloatValue("detailBlendGlobal", bDetailBlendEnabled ? 1.0f : 0.0f);
	}
}
