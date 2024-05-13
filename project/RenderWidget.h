#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "ParticleSystem.h"



namespace Fluid3d {
	class RenderWidget
	{
	private:
		// window
		
		int mWindowWidth = 1000;
		int mWindowHeight = 1000;

		// shaders
		GLuint mComputeShaderProgram;
		GLuint mScreenQuadShaderProgram;
		GLuint mDrawColorShaderProgram;

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
		
		bool mPauseFlag = false;



	public:
		RenderWidget();
		~RenderWidget();

		void Init();

		// 上传、读取数据
		void UploadUniforms(Fluid3d::ParticleSystem& ps);
		void UploadParticleInfo(Fluid3d::ParticleSystem& ps);
		void DumpParticleInfo(Fluid3d::ParticleSystem& ps);

		// 求解、渲染
		void SolveParticals();
		void Update(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

	private:
		void BuildShaders();
		void GenerateFrameBuffers();
		void GenerateBuffers();
		void GenerateTextures();
		void InitFilters();
		void CreateRenderAssets();
		void MakeVertexArrays(); // 生成画粒子的vao

		void DrawParticles(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
	};
}