///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include <cmath>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_DetailTextureValueName = "detailTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseDetailTextureName = "bUseDetailTexture";
	const char* g_UseLightingName = "bUseLighting";
	// Picnic table dimensions (matched to the previously working layout).
	const float g_TableTopLength = 6.0f;
	const float g_TableTopWidth = 2.2f;
	const float g_TableTopThickness = 0.15f;
	const float g_TableTopCenterY = 1.10f;
	const float g_TableBenchThickness = 0.12f;
	const float g_TableBenchWidth = 0.60f;
	const float g_TableBenchCenterY = 0.60f;
	const float g_TableBenchOffsetZ = (g_TableTopWidth * 0.5f) + (g_TableBenchWidth * 0.5f) + 0.2f;
	const float g_TableLegThickness = 0.25f;
	const float g_TableLegHeight = g_TableTopCenterY - (g_TableTopThickness * 0.5f);
	const float g_TableLegCenterY = g_TableLegHeight * 0.5f;
	const float g_TableLegCornerX = (g_TableTopLength * 0.5f) - 0.6f;
	const float g_TableLegCornerZ = (g_TableTopWidth * 0.5f) - 0.3f;
	const float g_TableTopSurfaceY = g_TableTopCenterY + (g_TableTopThickness * 0.5f);
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	DestroyGLTextures();
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// Prevent writing past our fixed texture slot array.
	if (m_loadedTextures >= 16)
	{
		std::cout << "Texture limit reached. Could not load: " << filename << std::endl;
		return false;
	}

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

			// set the texture wrapping parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			// Use trilinear filtering so distant textures stay smooth and do not shimmer.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// If anisotropic filtering is available, use it to keep angled surfaces sharper.
#if defined(GL_TEXTURE_MAX_ANISOTROPY_EXT) && defined(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT)
			float maxAnisotropy = 1.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
			float requestedAnisotropy = (maxAnisotropy > 8.0f) ? 8.0f : maxAnisotropy;
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, requestedAnisotropy);
#endif

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			stbi_image_free(image);
			glBindTexture(GL_TEXTURE_2D, 0);
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
		m_textureIDs[i].ID = 0;
		m_textureIDs[i].tag.clear();
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(bFound);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ,
	glm::vec3 offset)
{
	// variables for this method
	glm::mat4 model;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ + offset);

	model = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, model);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		// Color-only path should disable both texture samplers.
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setBoolValue(g_UseDetailTextureName, false);
		m_pShaderManager->setFloatValue("detailBlendFactor", 0.0f);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		int textureID = FindTextureSlot(textureTag);
		if (textureID < 0)
		{
			m_pShaderManager->setIntValue(g_UseTextureName, false);
			m_pShaderManager->setBoolValue(g_UseDetailTextureName, false);
			m_pShaderManager->setFloatValue("detailBlendFactor", 0.0f);
			return;
		}

		m_pShaderManager->setIntValue(g_UseTextureName, true);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
		// Reset detail texture each time so settings do not leak between objects.
		m_pShaderManager->setBoolValue(g_UseDetailTextureName, false);
		m_pShaderManager->setFloatValue("detailBlendFactor", 0.0f);
		m_pShaderManager->setVec2Value("detailUVScale", 1.0f, 1.0f);
	}
}

/***********************************************************
 *  SetShaderDetailTexture()
 *
 *  Enables a second texture so we can blend subtle detail
 *  over a base texture on selected objects.
 ***********************************************************/
void SceneManager::SetShaderDetailTexture(
	std::string textureTag,
	float blendFactor,
	float uScale,
	float vScale)
{
	if (NULL == m_pShaderManager)
	{
		return;
	}

	int textureSlot = FindTextureSlot(textureTag);
	if (textureSlot < 0)
	{
		m_pShaderManager->setBoolValue(g_UseDetailTextureName, false);
		m_pShaderManager->setFloatValue("detailBlendFactor", 0.0f);
		return;
	}

	// Clamp blend to [0,1] so invalid values do not break shading.
	if (blendFactor < 0.0f)
	{
		blendFactor = 0.0f;
	}
	if (blendFactor > 1.0f)
	{
		blendFactor = 1.0f;
	}

	m_pShaderManager->setBoolValue(g_UseDetailTextureName, true);
	m_pShaderManager->setSampler2DValue(g_DetailTextureValueName, textureSlot);
	m_pShaderManager->setFloatValue("detailBlendFactor", blendFactor);
	m_pShaderManager->setVec2Value("detailUVScale", uScale, vScale);
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

// *********************** Materials ***************************
void SceneManager::DefineObjectMaterials()
{
	m_objectMaterials.clear();

	// This helps avoid repeating the same push_back code over and over.
	auto addMaterial = [this](const std::string& tag, const glm::vec3& diffuse, const glm::vec3& specular, float shininess)
	{
		OBJECT_MATERIAL material;
		material.tag = tag;
		material.diffuseColor = diffuse;
		material.specularColor = specular;
		material.shininess = shininess;
		m_objectMaterials.push_back(material);
	};

	// Ground plane material.
	addMaterial("floor", glm::vec3(1.0f), glm::vec3(0.24f), 18.0f);
	// Picnic table wood.
	addMaterial("tableWood", glm::vec3(1.0f), glm::vec3(0.14f), 12.0f);
	// Soda body + metal parts.
	addMaterial("sodaBody", glm::vec3(0.72f, 0.79f, 0.98f), glm::vec3(0.95f), 92.0f);
	addMaterial("sodaMetal", glm::vec3(1.0f), glm::vec3(0.98f), 120.0f);
	// Pizza colors.
	addMaterial("pizzaCrust", glm::vec3(0.80f, 0.58f, 0.34f), glm::vec3(0.13f), 6.0f);
	addMaterial("pizzaCheese", glm::vec3(0.98f, 0.84f, 0.38f), glm::vec3(0.10f), 8.0f);
	addMaterial("pepperoni", glm::vec3(0.82f, 0.18f, 0.14f), glm::vec3(0.20f), 18.0f);
	// Dice + pips + ball.
	addMaterial("dice", glm::vec3(0.96f), glm::vec3(0.60f), 28.0f);
	addMaterial("dicePip", glm::vec3(0.05f), glm::vec3(0.20f), 18.0f);
	addMaterial("ball", glm::vec3(0.92f, 0.20f, 0.22f), glm::vec3(0.35f), 26.0f);
}


// ******************* Lighting *********************
void SceneManager::SetupSceneLights()
{
	// Main toggle for Phong lighting in fragment shader.
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Main directional light so we always have readable shading.
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.35f, -1.0f, -0.22f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.26f, 0.26f, 0.26f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.70f, 0.70f, 0.70f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.38f, 0.38f, 0.38f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Warm point light for fill.
	m_pShaderManager->setVec3Value("pointLights[0].position", 3.0f, 2.8f, 2.2f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.05f, 0.04f, 0.03f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.34f, 0.29f, 0.22f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.28f, 0.24f, 0.18f);
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.08f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.03f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Cool point light from the back side so dark sides still read well.
	m_pShaderManager->setVec3Value("pointLights[1].position", -3.4f, 2.5f, -1.9f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.02f, 0.03f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.16f, 0.20f, 0.30f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.14f, 0.20f, 0.30f);
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.08f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.03f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// Unused slots stay off.
	m_pShaderManager->setBoolValue("pointLights[2].bActive", false);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", false);
	m_pShaderManager->setBoolValue("pointLights[4].bActive", false);
	m_pShaderManager->setBoolValue("spotLight.bActive", false);
}

void SceneManager::DrawPicnicTable(const glm::vec3& centerPosition, float yRotationDegrees)
{
	// Rotate local table part offsets into world space.
	auto offsetFromCenter = [centerPosition, yRotationDegrees](const glm::vec3& localOffset)
	{
		float radians = glm::radians(yRotationDegrees);
		float c = std::cos(radians);
		float s = std::sin(radians);
		// Match GLM's Y-rotation convention so placement and mesh rotation agree.
		glm::vec3 rotated = glm::vec3(
			(localOffset.x * c) + (localOffset.z * s),
			localOffset.y,
			(-localOffset.x * s) + (localOffset.z * c));
		return centerPosition + rotated;
	};

	SetShaderMaterial("tableWood");
	SetShaderTexture("wood");
	// Stronger blend so T key on/off is easy to show during grading.
	SetShaderDetailTexture("woodDetail", 0.55f, 8.0f, 3.0f);

	// Table top.
	SetTransformations(
		glm::vec3(g_TableTopLength, g_TableTopThickness, g_TableTopWidth),
		0.0f,
		yRotationDegrees,
		0.0f,
		offsetFromCenter(glm::vec3(0.0f, g_TableTopCenterY, 0.0f)));
	SetTextureUVScale(2.8f, 1.3f);
	m_basicMeshes->DrawBoxMesh();

	// Left and right benches.
	SetTransformations(
		glm::vec3(g_TableTopLength, g_TableBenchThickness, g_TableBenchWidth),
		0.0f,
		yRotationDegrees,
		0.0f,
		offsetFromCenter(glm::vec3(0.0f, g_TableBenchCenterY, g_TableBenchOffsetZ)));
	SetTextureUVScale(2.8f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	SetTransformations(
		glm::vec3(g_TableTopLength, g_TableBenchThickness, g_TableBenchWidth),
		0.0f,
		yRotationDegrees,
		0.0f,
		offsetFromCenter(glm::vec3(0.0f, g_TableBenchCenterY, -g_TableBenchOffsetZ)));
	SetTextureUVScale(2.8f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Main table legs (four simple box supports).
	SetShaderDetailTexture("woodDetail", 0.35f, 5.0f, 3.0f);
	const glm::vec3 legOffsets[] = {
		glm::vec3(g_TableLegCornerX, g_TableLegCenterY, g_TableLegCornerZ),
		glm::vec3(-g_TableLegCornerX, g_TableLegCenterY, g_TableLegCornerZ),
		glm::vec3(g_TableLegCornerX, g_TableLegCenterY, -g_TableLegCornerZ),
		glm::vec3(-g_TableLegCornerX, g_TableLegCenterY, -g_TableLegCornerZ)
	};
	for (const glm::vec3& localLegOffset : legOffsets)
	{
		SetTransformations(
			glm::vec3(g_TableLegThickness, g_TableLegHeight, g_TableLegThickness),
			0.0f,
			yRotationDegrees,
			0.0f,
			offsetFromCenter(localLegOffset));
		SetTextureUVScale(1.0f, 2.0f);
		m_basicMeshes->DrawBoxMesh();
	}
}

void SceneManager::DrawSodaCan(const glm::vec3& baseCenterPosition, float radius, float height)
{
	// Main can body.
	SetTransformations(
		glm::vec3(radius, height, radius),
		0.0f,
		0.0f,
		0.0f,
		baseCenterPosition);
	SetShaderMaterial("sodaBody");
	SetShaderTexture("canBody");
	SetTextureUVScale(2.2f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Top lid.
	glm::vec3 lidPosition = glm::vec3(baseCenterPosition.x, baseCenterPosition.y + height - 0.01f, baseCenterPosition.z);
	SetTransformations(
		glm::vec3(radius * 0.94f, 0.04f, radius * 0.94f),
		0.0f,
		0.0f,
		0.0f,
		lidPosition);
	SetShaderMaterial("sodaMetal");
	SetShaderTexture("canTop");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Top and bottom rim bands (higher-detail torus rings).
	SetShaderTexture("canRim");
	SetTextureUVScale(3.0f, 1.0f);
	const float rimScale = radius * 1.04f;
	const float rimYInset = 0.010f;
	SetTransformations(
		glm::vec3(rimScale, rimScale, rimScale),
		90.0f,
		0.0f,
		0.0f,
		glm::vec3(baseCenterPosition.x, baseCenterPosition.y + height - rimYInset, baseCenterPosition.z));
	m_basicMeshes->DrawTorusMesh();

	SetTransformations(
		glm::vec3(rimScale, rimScale, rimScale),
		90.0f,
		0.0f,
		0.0f,
		glm::vec3(baseCenterPosition.x, baseCenterPosition.y + rimYInset, baseCenterPosition.z));
	m_basicMeshes->DrawTorusMesh();
}

void SceneManager::DrawPizzaSlice(const glm::vec3& centerPosition, float yRotationDegrees)
{
	// Crust wedge.
	SetTransformations(
		glm::vec3(1.40f, 0.18f, 1.80f),
		0.0f,
		yRotationDegrees,
		0.0f,
		centerPosition);
	SetShaderMaterial("pizzaCrust");
	SetShaderColor(0.80f, 0.58f, 0.34f, 1.0f);
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPrismMesh();

	// Cheese wedge slightly above crust.
	SetTransformations(
		glm::vec3(1.22f, 0.08f, 1.58f),
		0.0f,
		yRotationDegrees,
		0.0f,
		glm::vec3(centerPosition.x, centerPosition.y + 0.10f, centerPosition.z + 0.02f));
	SetShaderMaterial("pizzaCheese");
	SetShaderColor(0.98f, 0.84f, 0.38f, 1.0f);
	m_basicMeshes->DrawPrismMesh();

	// Pepperoni circles on top.
	const glm::vec3 localPepperoni[] = {
		glm::vec3(-0.28f, 0.16f, -0.16f),
		glm::vec3(0.24f, 0.16f, -0.22f),
		glm::vec3(0.00f, 0.16f, 0.26f),
	};

	float radians = glm::radians(yRotationDegrees);
	float c = std::cos(radians);
	float s = std::sin(radians);
	for (const glm::vec3& localPos : localPepperoni)
	{
		glm::vec3 rotated = glm::vec3(
			(localPos.x * c) + (localPos.z * s),
			localPos.y,
			(-localPos.x * s) + (localPos.z * c));
		glm::vec3 worldPos = centerPosition + rotated;

		SetTransformations(
			glm::vec3(0.13f, 0.025f, 0.13f),
			0.0f,
			0.0f,
			0.0f,
			worldPos);
		SetShaderMaterial("pepperoni");
		SetShaderColor(0.82f, 0.18f, 0.14f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();
	}
}

void SceneManager::DrawDice(const glm::vec3& centerPosition, float edgeLength)
{
	// Dice cube.
	SetTransformations(
		glm::vec3(edgeLength),
		0.0f,
		0.0f,
		0.0f,
		centerPosition);
	SetShaderMaterial("dice");
	SetShaderColor(0.96f, 0.96f, 0.96f, 1.0f);
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Draw pips on all six faces so the dice reads correctly from any angle.
	const float pipRadius = edgeLength * 0.085f;
	const float pipOffset = edgeLength * 0.22f;
	const float faceOffset = (edgeLength * 0.50f) + (pipRadius * 0.20f);

	SetShaderMaterial("dicePip");
	SetShaderColor(0.08f, 0.08f, 0.08f, 1.0f);

	auto drawPipAtLocal = [this, centerPosition, pipRadius](float localX, float localY, float localZ)
	{
		SetTransformations(
			glm::vec3(pipRadius),
			0.0f,
			0.0f,
			0.0f,
			centerPosition + glm::vec3(localX, localY, localZ));
		m_basicMeshes->DrawSphereMesh();
	};

	// +Z face: 1
	drawPipAtLocal(0.0f, 0.0f, faceOffset);

	// -Z face: 6
	const float pipRows[] = { -pipOffset, 0.0f, pipOffset };
	for (float y : pipRows)
	{
		drawPipAtLocal(-pipOffset, y, -faceOffset);
		drawPipAtLocal(pipOffset, y, -faceOffset);
	}

	// +X face: 3
	drawPipAtLocal(faceOffset, -pipOffset, -pipOffset);
	drawPipAtLocal(faceOffset, 0.0f, 0.0f);
	drawPipAtLocal(faceOffset, pipOffset, pipOffset);

	// -X face: 4
	drawPipAtLocal(-faceOffset, -pipOffset, -pipOffset);
	drawPipAtLocal(-faceOffset, -pipOffset, pipOffset);
	drawPipAtLocal(-faceOffset, pipOffset, -pipOffset);
	drawPipAtLocal(-faceOffset, pipOffset, pipOffset);

	// +Y face: 5
	drawPipAtLocal(-pipOffset, faceOffset, -pipOffset);
	drawPipAtLocal(-pipOffset, faceOffset, pipOffset);
	drawPipAtLocal(0.0f, faceOffset, 0.0f);
	drawPipAtLocal(pipOffset, faceOffset, -pipOffset);
	drawPipAtLocal(pipOffset, faceOffset, pipOffset);

	// -Y face: 2
	drawPipAtLocal(-pipOffset, -faceOffset, -pipOffset);
	drawPipAtLocal(pipOffset, -faceOffset, pipOffset);
}

void SceneManager::DrawBall(const glm::vec3& centerPosition, float radius)
{
	SetTransformations(
		glm::vec3(radius),
		0.0f,
		0.0f,
		0.0f,
		centerPosition);
	SetShaderMaterial("ball");
	SetShaderColor(0.92f, 0.20f, 0.22f, 1.0f);
	m_basicMeshes->DrawSphereMesh();
}


/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Create materials and lights used by the scene
	DefineObjectMaterials();
	SetupSceneLights();

	// Load scene textures. Try project-root paths first, then Debug-relative paths.
	bool loaded = CreateGLTexture("textures/pavers.jpg", "ground");
	if (!loaded) loaded = CreateGLTexture("../textures/pavers.jpg", "ground");
	if (!loaded) std::cout << "WARNING: ground texture was not loaded." << std::endl;

	loaded = CreateGLTexture("textures/rusticwood.jpg", "wood");
	if (!loaded) loaded = CreateGLTexture("../textures/rusticwood.jpg", "wood");
	if (!loaded) std::cout << "WARNING: wood texture was not loaded." << std::endl;

	loaded = CreateGLTexture("textures/stainless.jpg", "canBody");
	if (!loaded) loaded = CreateGLTexture("../textures/stainless.jpg", "canBody");
	if (!loaded) std::cout << "WARNING: can body texture was not loaded." << std::endl;

	loaded = CreateGLTexture("textures/stainless_end.jpg", "canTop");
	if (!loaded) loaded = CreateGLTexture("../textures/stainless_end.jpg", "canTop");
	if (!loaded) std::cout << "WARNING: can top texture was not loaded." << std::endl;

	loaded = CreateGLTexture("textures/stainless.jpg", "canRim");
	if (!loaded) loaded = CreateGLTexture("../textures/stainless.jpg", "canRim");
	if (!loaded) std::cout << "WARNING: can rim texture was not loaded." << std::endl;

	// Secondary texture used by the T-key texture mode on wooden surfaces.
	loaded = CreateGLTexture("textures/stainedglass.jpg", "woodDetail");
	if (!loaded) loaded = CreateGLTexture("../textures/stainedglass.jpg", "woodDetail");
	if (!loaded) std::cout << "WARNING: wood detail texture was not loaded." << std::endl;

	BindGLTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh(0.02f);
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Ground plane so the objects are not floating in empty space.
	SetTransformations(
		glm::vec3(18.0f, 1.0f, 10.0f),
		0.0f,
		0.0f,
		0.0f,
		glm::vec3(0.0f, -0.03f, 0.0f));
	SetShaderMaterial("floor");
	SetShaderTexture("ground");
	SetTextureUVScale(6.0f, 3.0f);
	m_basicMeshes->DrawPlaneMesh();

	// One picnic table in perspective.
	const glm::vec3 tableCenter = glm::vec3(0.0f, 0.0f, 0.0f);
	const float tableYawDegrees = 0.0f;
	const float tableTopY = tableCenter.y + g_TableTopSurfaceY;
	DrawPicnicTable(tableCenter, tableYawDegrees);

	// Rotate local table offsets into world space.
	auto offsetOnTable = [tableCenter, tableTopY, tableYawDegrees](const glm::vec3& localOffset)
	{
		float radians = glm::radians(tableYawDegrees);
		float c = std::cos(radians);
		float s = std::sin(radians);
		// Keep table-object offsets on the same rotation convention as the table parts.
		glm::vec3 rotated = glm::vec3(
			(localOffset.x * c) + (localOffset.z * s),
			localOffset.y,
			(-localOffset.x * s) + (localOffset.z * c));
		return glm::vec3(tableCenter.x, tableTopY, tableCenter.z) + rotated;
	};

	// Main four objects on the table.
	DrawSodaCan(offsetOnTable(glm::vec3(-2.35f, 0.0f, 0.08f)), 0.30f, 1.05f);
	DrawPizzaSlice(offsetOnTable(glm::vec3(-0.55f, 0.12f, -0.15f)), tableYawDegrees + 8.0f);
	DrawDice(offsetOnTable(glm::vec3(1.10f, 0.29f, 0.02f)), 0.58f);
	DrawBall(offsetOnTable(glm::vec3(2.35f, 0.36f, 0.22f)), 0.36f);

	// Keep one main object set for the final scene:
	// soda can, pizza slice, dice, and ball on a single picnic table.
}
