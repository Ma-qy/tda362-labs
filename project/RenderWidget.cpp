#include "RenderWidget.h"
#include "ParticleSystem.h"
#include "Global.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <labhelper.h>

const glm::vec3 vertexes[] = {
	glm::vec3(0.0, 0.0, 0.0),
	glm::vec3(1.0, 0.0, 0.0),
	glm::vec3(0.0, 1.0, 0.0),
	glm::vec3(0.0, 0.0, 1.0)
};

const GLuint indices[] = {
	0, 1, 0, 2, 0, 3
};

std::vector<float_t> floorVertices = {
	// vertex           texCoord
	 1.0f,  1.0f, 0.0f, 1.0, 1.0,
	-1.0f,  1.0f, 0.0f, 0.0, 1.0,
	-1.0f, -1.0f, 0.0f, 0.0, 0.0,
	 1.0f,  1.0f, 0.0f, 1.0, 1.0,
	-1.0f, -1.0f, 0.0f, 0.0, 0.0,
	 1.0f, -1.0f, 0.0f, 1.0, 0.0,
};


glm::mat4 floorModel;

namespace Fluid3d
{
	RenderWidget::RenderWidget()
	{
		floorModel = glm::mat4(1.0);
		floorModel = glm::scale(floorModel, glm::vec3(0.6));
		floorModel = glm::translate(floorModel, glm::vec3(0.5, 0.5, 0.0));
	}

	RenderWidget::~RenderWidget()
	{
		//Destroy();
	}


	void RenderWidget::Init()
	{
		BuildShaders();

		GenerateFrameBuffers();
		GenerateBuffers();
		GenerateTextures();

		InitFilters();
		CreateRenderAssets();
		MakeVertexArrays(); // 生成画粒子的vao

		glGenVertexArrays(1, &mVaoNull);
		glEnable(GL_MULTISAMPLE);
	}



	/// <summary>
	/// load shaders, using labhelper
	/// </summary>
	void RenderWidget::BuildShaders()
	{
		//load compute shader and set "kernelbuffer" value
		mComputeShaderProgram = labhelper::loadComputeShader("../project/particleUpdate.comp", false);
		glUseProgram(mComputeShaderProgram);
		glUniform1i(glGetUniformLocation(mComputeShaderProgram, "kernelBuffer"), 1);
		glUseProgram(0);

		mScreenQuadShaderProgram = labhelper::loadShaderProgram("../project/ScreenQuad.vert", "../project/ScreenQuad.frag", false);
		glUseProgram(mScreenQuadShaderProgram);
		glUniform1i(glGetUniformLocation(mScreenQuadShaderProgram, "tex"), 0);
		glUseProgram(0);


		mDrawColorShaderProgram = labhelper::loadShaderProgram("../project/DrawColor3d.vert", "../project/DrawColor3d.frag", false);
		

	}

	

	void RenderWidget::GenerateBuffers()
	{
		glGenBuffers(1, &mCoordVertBuffer);     // coord vbo
		glGenBuffers(1, &mBufferParticals);     // ssbo
		glGenBuffers(1, &mBufferBlocks);
		glGenBuffers(1, &mBufferFloor);
	}
	

	void RenderWidget::GenerateTextures()
	{
		// 测试用的纹理
		glGenTextures(1, &mTestTexture);
		glBindTexture(GL_TEXTURE_2D, mTestTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 100, 100, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		// 核函数纹理
		glGenTextures(1, &mTexKernelBuffer);
		glBindTexture(GL_TEXTURE_1D, mTexKernelBuffer);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_1D, 0);

		// 模糊Z后的坐标图
		glGenTextures(1, &mTexZBlurTempBuffer);
		glBindTexture(GL_TEXTURE_2D, mTexZBlurTempBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mWindowWidth, mWindowHeight, 0, GL_RED, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	//生成framebuffer 生成存储深度和厚度纹理
	void RenderWidget::GenerateFrameBuffers()
	{
		glGenFramebuffers(1, &mFboDepth);

		//生成并设置深度纹理
		glGenTextures(1, &mTexZBuffer);
		glBindTexture(GL_TEXTURE_2D, mTexZBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mWindowWidth, mWindowHeight, 0, GL_RED, GL_FLOAT, NULL);  //初始化为单通道浮点纹理，大小与窗口大小相同。这种纹理常用于存储深度值或其他单通道数据。
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  //防止纹理边缘重复
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);    //纹理过滤方式
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		//render buffer
		glGenRenderbuffers(1, &mRboDepthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, mRboDepthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mWindowWidth, mWindowHeight);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		//将纹理和渲染缓冲区附加到 Framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, mFboDepth);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexZBuffer, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRboDepthBuffer);

		//检查 Framebuffer 完整性
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cout << "ERROR: mFboDepth is not complete!" << std::endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		// thickness framebuffer
		glGenFramebuffers(1, &mFboThickness);

		glGenTextures(1, &mTexThicknessBuffer);
		glBindTexture(GL_TEXTURE_2D, mTexThicknessBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mWindowWidth, mWindowHeight, 0, GL_RED, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, mFboThickness);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexThicknessBuffer, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRboDepthBuffer);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cout << "ERROR: mFboThickness is not complete!" << std::endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void RenderWidget::InitFilters()
	{
		//TODO
	}
	void RenderWidget::CreateRenderAssets()
	{
		//TODO
	}
	void RenderWidget::MakeVertexArrays()
	{
		glGenVertexArrays(1, &mVaoParticals);
		glBindVertexArray(mVaoParticals);

		glBindBuffer(GL_ARRAY_BUFFER, mBufferParticals);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleInfo3d), (void*)offsetof(ParticleInfo3d, position));  //正确访问结构体中每个属性。需要这个offset
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleInfo3d), (void*)offsetof(ParticleInfo3d, density));
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);


		glGenVertexArrays(1, &mVaoCoord);
		glBindVertexArray(mVaoCoord);
		glBindBuffer(GL_ARRAY_BUFFER, mCoordVertBuffer);
		glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec3), vertexes, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		glGenVertexArrays(1, &mVaoFloor);
		glBindVertexArray(mVaoFloor);
		glBindBuffer(GL_ARRAY_BUFFER, mBufferFloor);
		glBufferData(GL_ARRAY_BUFFER, floorVertices.size() * sizeof(float_t), floorVertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0)); //vertex
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float_t))); //tex
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);
	}


	void RenderWidget::UploadUniforms(Fluid3d::ParticleSystem& ps)
	{
		glUseProgram(mComputeShaderProgram);
		//labhelper::setUniformSlow(mComputeShaderProgram, "blockNum", ps.mBlockNum);
		glUniform3uiv(glGetUniformLocation(mComputeShaderProgram, "blockNum"), 1, &ps.mBlockNum[0]);
		labhelper::setUniformSlow(mComputeShaderProgram, "blockSize", ps.mBlockSize);
		labhelper::setUniformSlow(mComputeShaderProgram, "containerLowerBound", ps.mLowerBound);
		labhelper::setUniformSlow(mComputeShaderProgram, "containerUpperBound", ps.mUpperBound);
		glUniform1iv(glGetUniformLocation(mComputeShaderProgram, "blockIdOffs"), ps.mBlockIdOffs.size(), ps.mBlockIdOffs.data());

		labhelper::setUniformSlow(mComputeShaderProgram, "gSupportRadius", Para3d::supportRadius);
		labhelper::setUniformSlow(mComputeShaderProgram, "gDensity0", Para3d::density0);
		labhelper::setUniformSlow(mComputeShaderProgram, "gVolume", ps.mVolume);
		labhelper::setUniformSlow(mComputeShaderProgram, "gMass", 0.5f);
		glUniform1f(glGetUniformLocation(mComputeShaderProgram, "gStiffness"), ps.mStiffness);
		//labhelper::setUniformSlow(mComputeShaderProgram, "gExponent", ps.mExponent);
		glUniform1f(glGetUniformLocation(mComputeShaderProgram, "gExponent"), ps.mExponent);
		labhelper::setUniformSlow(mComputeShaderProgram, "gViscosity", ps.mViscosity);
		glUseProgram(0);
		
		//bind kernel buffer as weight function
		glBindTexture(GL_TEXTURE_1D, mTexKernelBuffer);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RG32F, ps.mW.GetBufferSize(), 0, GL_RG, GL_FLOAT, ps.mW.GetData());
		glBindTexture(GL_TEXTURE_1D, 0);
	}

	void RenderWidget::UploadParticleInfo(Fluid3d::ParticleSystem& ps) {
		// 申请装粒子信息的buffer
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBufferParticals);
		glBufferData(GL_SHADER_STORAGE_BUFFER, ps.mParticalInfos.size() * sizeof(ParticleInfo3d), ps.mParticalInfos.data(), GL_DYNAMIC_COPY);

		// 申请block区间buffer
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBufferBlocks);
		glBufferData(GL_SHADER_STORAGE_BUFFER, ps.mBlockExtens.size() * sizeof(glm::uvec2), ps.mBlockExtens.data(), GL_DYNAMIC_COPY);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		mParticalNum = ps.mParticalInfos.size();

	}

	void RenderWidget::DumpParticleInfo(Fluid3d::ParticleSystem& ps) {
		// 把粒子信息拷回CP
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBufferParticals);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, mParticalNum * sizeof(ParticleInfo3d), (void*)ps.mParticalInfos.data());
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void RenderWidget::SolveParticals()
	{
		if (mParticalNum <= 0 || mPauseFlag) {
			return;
		}

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mBufferParticals);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, mBufferBlocks);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_1D, mTexKernelBuffer);
		glBindImageTexture(0, mTestTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glUseProgram(mComputeShaderProgram);
		labhelper::setUniformSlow(mComputeShaderProgram, "gGravityDir", -Glb::Z_AXIS);
		//labhelper::setUniformSlow(mComputeShaderProgram, "gExternelAccleration", mExternelAccleration); //暂时不考虑鼠标带来的外部加速度
		labhelper::setUniformSlow(mComputeShaderProgram, "particalNum", mParticalNum);
		
		//2 pass in compute shader
		for (int pass = 0; pass <= 1; pass++) {
			labhelper::setUniformSlow(mComputeShaderProgram, "pass", pass);
			glDispatchCompute((GLuint)(mParticalNum / 512), 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
		glUseProgram(0);
	}

	void RenderWidget::Update(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
	{
		DrawParticles(viewMatrix, projectionMatrix);
	}

	void RenderWidget::DrawParticles(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
	{
		glFinish();
		//// 以点的形式画粒子
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		
		glEnable(GL_PROGRAM_POINT_SIZE);
		glUseProgram(mDrawColorShaderProgram);
		labhelper::setUniformSlow(mDrawColorShaderProgram, "view", viewMatrix);
		labhelper::setUniformSlow(mDrawColorShaderProgram, "projection", projectionMatrix);
		
		
		glBindVertexArray(mVaoCoord);
		glDrawElements(GL_LINES, 6, GL_UNSIGNED_INT, indices);
		glBindVertexArray(mVaoParticals);
		glDrawArrays(GL_POINTS, 0, mParticalNum);
		/*mSkyBox->Draw(mWindow, mVaoNull, mCamera.GetView(), mCamera.GetProjection());
		mDrawColor3d->UnUse();*/
	}

}