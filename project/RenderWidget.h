#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


#include "Material.h"
#include "ParticleSystem.h"
#include "DepthFilter.h"
#include "FluidShadowMap.h"
#include "SkyBox.h"
#include "RenderCamera.h"


namespace Fluid3d {
	class RenderWidget
	{
	private:
		// window
		GLFWwindow* mWindow = nullptr;
		int mWindowWidth = 1000;
		int mWindowHeight = 1000;

		// camera
		RenderCamera mCamera;
		bool mFirstMouseFlag = true;
		float mLastX;
		float mLastY;
		bool mLeftPressFlag = false;
		bool mRightPressFlag = false;
		bool mMiddlePressFlag = false;
		bool mPauseFlag = false;

		// shaders
		GLuint mComputeShaderProgram;
		GLuint mScreenQuadShaderProgram;
		GLuint mDrawColorShaderProgram;
		GLuint mPointZShaderProgram;
		GLuint mThicknessShaderProgram;
		GLuint mFluidColorShaderProgram;
		GLuint mModelShaderProgram;

		// fbo
		GLuint mFboDepth = 0;
		GLuint mTexZBuffer = 0;
		GLuint mRboDepthBuffer = 0;

		GLuint mFboThickness = 0;
		GLuint mTexThicknessBuffer = 0;

		// vao
		GLuint mVaoNull = 0;
		GLuint mVaoParticals = 0;
		GLuint mVaoCoord = 0;
		GLuint mVaoFloor = 0;


		// buffers
		GLuint mCoordVertBuffer = 0;
		GLuint mBufferParticals = 0;
		GLuint mBufferBlocks = 0;
		GLuint mBufferFloor = 0;

		// texures
		GLuint mTestTexture = 0;
		GLuint mTexKernelBuffer = 0;
		GLuint mTexZBlurTempBuffer = 0;

		// time statistics
		int32_t mParticalNum = 0;
		float_t mUpdateTime = 0.0f;
		float_t updateTitleTime = 0.0f;
		float_t frameCount = 0.0f;
		
		
		DepthFilter* mDepthFilter;


		Material* mSlabWhite = nullptr;
		FluidShadowMap* mShadowMap = nullptr;
		PointLight mLight;
		SkyBox* mSkyBox = nullptr;

		glm::vec3 mExternelAccleration = { 0.0, 0.0, 0.0 };
		
	public:
		RenderWidget();
		~RenderWidget();

		bool CreateWindow();

		void Init();

		// �ϴ�����ȡ����
		void UploadUniforms(Fluid3d::ParticleSystem& ps);
		void UploadParticleInfo(Fluid3d::ParticleSystem& ps);
		void DumpParticleInfo(Fluid3d::ParticleSystem& ps);

		// ��⡢��Ⱦ
		void SolveParticals();
		void Update();

		// window
		bool ShouldClose();
		void ProcessInput();
		void PollEvents();

	private:
		void BuildShaders();
		void GenerateFrameBuffers();
		void GenerateBuffers();
		void GenerateTextures();
		void InitFilters();
		void CreateRenderAssets();
		void MakeVertexArrays(); // ���ɻ����ӵ�vao

		void DrawParticles();
		void LoadSkyBox();

		static void ResizeCallback(GLFWwindow* window, int width, int height);
		static void CursorPosCallBack(GLFWwindow* window, double xpos, double ypos);
		static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
		static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
	};


}