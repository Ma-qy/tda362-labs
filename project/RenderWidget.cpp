#include "RenderWidget.h"
#include "ParticleSystem.h"
#include "Global.h"
#include "DepthFilter.h"
#include "Material.h"
#include "SkyBox.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>

#include <SDL.h>
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
	RenderWidget::RenderWidget(RenderCamera* camera)
		: mCamera(camera)
	{
		
	}

	RenderWidget::~RenderWidget()
	{
		//Destroy();
	}


	void RenderWidget::Init()
	{
		floorModel = glm::mat4(1.0);
		floorModel = glm::scale(floorModel, glm::vec3(0.5));
		floorModel = glm::translate(floorModel, glm::vec3(0.5, 0.5, 1.2));

		BuildShaders();

		GenerateFrameBuffers();
		GenerateBuffers();
		GenerateTextures();

		InitFilters();
		LoadSkyBox();
		CreateRenderAssets();
		MakeVertexArrays(); // ���ɻ����ӵ�vao

		glGenVertexArrays(1, &mVaoNull);
		glEnable(GL_MULTISAMPLE);
	}

	void RenderWidget::InitFilters() {
		mDepthFilter = new DepthFilter();
		mDepthFilter->Create(8.0, 0.025);
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
		

		mPointZShaderProgram = labhelper::loadShaderProgram("../project/PointSprite.vert", "../project/PointSpriteZValue.frag", "../project/PointSprite.geom", false);
		glUseProgram(mPointZShaderProgram);
		labhelper::setUniformSlow(mPointZShaderProgram, "zFar", Para3d::zFar);
		labhelper::setUniformSlow(mPointZShaderProgram, "zNear", Para3d::zNear);
		glUseProgram(0);


		mThicknessShaderProgram = labhelper::loadShaderProgram("../project/PointSprite.vert", "../project/PointSpriteThickness.frag", "../project/PointSprite.geom", false);

		mFluidColorShaderProgram = labhelper::loadShaderProgram("../project/DrawFluidColor.vert", "../project/DrawFluidColor.frag", false);
		glUseProgram(mFluidColorShaderProgram);
		labhelper::setUniformSlow(mFluidColorShaderProgram, "zFar", Para3d::zFar);
		labhelper::setUniformSlow(mFluidColorShaderProgram, "zNear", Para3d::zNear);
		labhelper::setUniformSlow(mFluidColorShaderProgram, "eta", (float)(1.0f / Para3d::IOR));
		labhelper::setUniformSlow(mFluidColorShaderProgram, "f0", Para3d::F0);
		glm::vec4 cin = Glb::ProjToIntrinsic(mCamera->GetProjection(), mWindowWidth, mWindowHeight);
		glUniform4fv(glGetUniformLocation(mFluidColorShaderProgram, "cameraIntrinsic"), 1, &cin[0]);
		labhelper::setUniformSlow(mFluidColorShaderProgram, "fluidColor", Para3d::FLUID_COLOR);
		labhelper::setUniformSlow(mFluidColorShaderProgram, "thicknessFactor", Para3d::THICKNESS_FACTOR);
		glUseProgram(0);

		mModelShaderProgram = labhelper::loadShaderProgram("../project/DrawModel.vert", "../project/DrawModel.frag", false);


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
		// �����õ�����
		glGenTextures(1, &mTestTexture);
		glBindTexture(GL_TEXTURE_2D, mTestTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 100, 100, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		// �˺�������
		glGenTextures(1, &mTexKernelBuffer);
		glBindTexture(GL_TEXTURE_1D, mTexKernelBuffer);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_1D, 0);

		// ģ��Z�������ͼ
		glGenTextures(1, &mTexZBlurTempBuffer);
		glBindTexture(GL_TEXTURE_2D, mTexZBlurTempBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mWindowWidth, mWindowHeight, 0, GL_RED, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	//����framebuffer ���ɴ洢��Ⱥͺ������
	void RenderWidget::GenerateFrameBuffers()
	{
		glGenFramebuffers(1, &mFboDepth);

		//���ɲ������������
		glGenTextures(1, &mTexZBuffer);
		glBindTexture(GL_TEXTURE_2D, mTexZBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mWindowWidth, mWindowHeight, 0, GL_RED, GL_FLOAT, NULL);  //��ʼ��Ϊ��ͨ��������������С�봰�ڴ�С��ͬ���������������ڴ洢���ֵ��������ͨ�����ݡ�
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  //��ֹ������Ե�ظ�
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);    //�������˷�ʽ
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		//render buffer
		glGenRenderbuffers(1, &mRboDepthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, mRboDepthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mWindowWidth, mWindowHeight);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		//����������Ⱦ���������ӵ� Framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, mFboDepth);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexZBuffer, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRboDepthBuffer);

		//��� Framebuffer ������
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

	
	void RenderWidget::CreateRenderAssets()
	{
		mSlabWhite = new Material();
		mSlabWhite->Create();
		std::string albedoPath = "../scenes/Slab/TexturesCom_Marble_SlabWhite_1K_albedo.png";
		std::string roughnessPath = "../scenes/Slab/TexturesCom_Marble_SlabWhite_1K_roughness.png";
		mSlabWhite->LoadTextures(albedoPath, roughnessPath);

		// �ƹ�
		mLight.pos = glm::vec3(-0.8, -0.8, 2.0);
		mLight.dir = glm::vec3(0.5, 0.5, -1.0);
		mLight.aspect = 1.0f;
		mLight.fovy = 30.0;

		mShadowMap = new FluidShadowMap();
		mShadowMap->SetImageSize(mWindowWidth, mWindowHeight);
		mShadowMap->SetLightInfo(mLight);
		mShadowMap->SetIor(Para3d::IOR);
		mShadowMap->Init();

	}
	void RenderWidget::MakeVertexArrays()
	{
		glGenVertexArrays(1, &mVaoParticals);
		glBindVertexArray(mVaoParticals);

		//particle position ��Ϣ���� mVaoParticals��0λ��
		glBindBuffer(GL_ARRAY_BUFFER, mBufferParticals);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleInfo3d), (void*)offsetof(ParticleInfo3d, position));  //��ȷ���ʽṹ����ÿ�����ԡ���Ҫ���offset
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
		labhelper::setUniformSlow(mComputeShaderProgram, "blockNum", ps.mBlockNum);
		//glUniform3uiv(glGetUniformLocation(mComputeShaderProgram, "blockNum"), 1, &ps.mBlockNum[0]);
		labhelper::setUniformSlow(mComputeShaderProgram, "blockSize", ps.mBlockSize);
		labhelper::setUniformSlow(mComputeShaderProgram, "containerLowerBound", ps.mLowerBound);
		labhelper::setUniformSlow(mComputeShaderProgram, "containerUpperBound", ps.mUpperBound);
		glUniform1iv(glGetUniformLocation(mComputeShaderProgram, "blockIdOffs"), ps.mBlockIdOffs.size(), ps.mBlockIdOffs.data());

		labhelper::setUniformSlow(mComputeShaderProgram, "gSupportRadius", Para3d::supportRadius);
		labhelper::setUniformSlow(mComputeShaderProgram, "gDensity0", Para3d::density0);
		labhelper::setUniformSlow(mComputeShaderProgram, "gVolume", ps.mVolume);
		labhelper::setUniformSlow(mComputeShaderProgram, "gMass", 0.5f);
		labhelper::setUniformSlow(mComputeShaderProgram, "gStiffness", ps.mStiffness);
		labhelper::setUniformSlow(mComputeShaderProgram, "gExponent", ps.mExponent);
		
		labhelper::setUniformSlow(mComputeShaderProgram, "gViscosity", ps.mViscosity);
		glUseProgram(0);
		
		//bind kernel buffer as weight function
		glBindTexture(GL_TEXTURE_1D, mTexKernelBuffer);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RG32F, ps.mW.GetBufferSize(), 0, GL_RG, GL_FLOAT, ps.mW.GetData());
		glBindTexture(GL_TEXTURE_1D, 0);
	}

	void RenderWidget::UploadParticleInfo(Fluid3d::ParticleSystem& ps) {
		// ����װ������Ϣ��buffer
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBufferParticals);
		glBufferData(GL_SHADER_STORAGE_BUFFER, ps.mParticalInfos.size() * sizeof(ParticleInfo3d), ps.mParticalInfos.data(), GL_DYNAMIC_COPY);

		// ����block����buffer
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBufferBlocks);
		glBufferData(GL_SHADER_STORAGE_BUFFER, ps.mBlockExtens.size() * sizeof(glm::uvec2), ps.mBlockExtens.data(), GL_DYNAMIC_COPY);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		mParticalNum = ps.mParticalInfos.size();

	}

	void RenderWidget::DumpParticleInfo(Fluid3d::ParticleSystem& ps) {
		// ��������Ϣ����CP
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
		labhelper::setUniformSlow(mComputeShaderProgram, "gExternelAccleration", mExternelAccleration); //��ʱ���������������ⲿ���ٶ�
		labhelper::setUniformSlow(mComputeShaderProgram, "particalNum", mParticalNum);
		
		//2 pass in compute shader
		for (GLuint pass = 0; pass <= 1; pass++) {
			labhelper::setUniformSlow(mComputeShaderProgram, "pass", pass);
			glDispatchCompute(mParticalNum / 512 + 1, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
		glUseProgram(0);
	}

	void RenderWidget::Update()
	{
		DrawParticles();
	}

	void RenderWidget::DrawParticles()
	{
		glFinish();
		//// �Ե����ʽ������

		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//
		//glEnable(GL_DEPTH_TEST);
		//glDepthFunc(GL_LEQUAL);
		//
		//glEnable(GL_PROGRAM_POINT_SIZE);
		//glUseProgram(mDrawColorShaderProgram);
		//labhelper::setUniformSlow(mDrawColorShaderProgram, "view", mCamera.GetView());
		//labhelper::setUniformSlow(mDrawColorShaderProgram, "projection", mCamera.GetProjection());
		//
		//
		//glBindVertexArray(mVaoCoord);
		//glDrawElements(GL_LINES, 6, GL_UNSIGNED_INT, indices);
		//glBindVertexArray(mVaoParticals);
		//glDrawArrays(GL_POINTS, 0, mParticalNum);
		///*mSkyBox->Draw(mWindow, mVaoNull, mCamera.GetView(), mCamera.GetProjection());
		//mDrawColor3d->UnUse();*/

		//深度图
		glBindFramebuffer(GL_FRAMEBUFFER, mFboDepth);

		glViewport(0, 0, mWindowWidth, mWindowHeight);
		glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


 		glUseProgram(mPointZShaderProgram);
		labhelper::setUniformSlow(mPointZShaderProgram, "view", mCamera->GetView());
		labhelper::setUniformSlow(mPointZShaderProgram, "projection", mCamera->GetProjection());
		labhelper::setUniformSlow(mPointZShaderProgram, "particalRadius", Para3d::particalRadius);
		labhelper::setUniformSlow(mPointZShaderProgram, "cameraUp", mCamera->GetUp());
		labhelper::setUniformSlow(mPointZShaderProgram, "cameraRight", mCamera->GetRight());
		labhelper::setUniformSlow(mPointZShaderProgram, "cameraFront", mCamera->GetFront());
		glBindVertexArray(mVaoParticals);
		glDrawArrays(GL_POINTS, 0, mParticalNum);
	
		glUseProgram(0);
		//
		//

		//混合
		GLuint bufferA = mTexZBuffer;
		GLuint bufferB = mTexZBlurTempBuffer;
		mDepthFilter->Filter(bufferA, bufferB, glm::ivec2(mWindowWidth, mWindowHeight));


		//厚度图
		glBindFramebuffer(GL_FRAMEBUFFER, mFboThickness);
		
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		glUseProgram(mThicknessShaderProgram);
		labhelper::setUniformSlow(mThicknessShaderProgram, "view", mCamera->GetView());
		labhelper::setUniformSlow(mThicknessShaderProgram, "projection", mCamera->GetProjection());
		labhelper::setUniformSlow(mThicknessShaderProgram, "particalRadius", Para3d::particalRadius);
		labhelper::setUniformSlow(mThicknessShaderProgram, "cameraUp", mCamera->GetUp());
		labhelper::setUniformSlow(mThicknessShaderProgram, "cameraRight", mCamera->GetRight());
		labhelper::setUniformSlow(mThicknessShaderProgram, "cameraFront", mCamera->GetFront());
		glBindVertexArray(mVaoParticals);
		glDrawArrays(GL_POINTS, 0, mParticalNum);
		glUseProgram(0);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		// shadowmap
		mShadowMap->Update(mVaoParticals, mParticalNum, mDepthFilter);

		// 
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, mWindowWidth, mWindowHeight);
		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		mShadowMap->DrawCaustic(mVaoNull, floorModel);

		// 地板
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mShadowMap->GetShadowMap());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mShadowMap->GetCausticMap());
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, mSkyBox->GetId());
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, mSlabWhite->mTexAlbedo);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, mSlabWhite->mTexRoughness);
		glUseProgram(mModelShaderProgram);
		labhelper::setUniformSlow(mModelShaderProgram, "model", floorModel);
		labhelper::setUniformSlow(mModelShaderProgram, "view", mCamera->GetView());
		labhelper::setUniformSlow(mModelShaderProgram, "projection", mCamera->GetProjection());
		labhelper::setUniformSlow(mModelShaderProgram, "lightView", mShadowMap->mLightView);
		labhelper::setUniformSlow(mModelShaderProgram, "lightProjection", mShadowMap->mLightProjection);
		glBindVertexArray(mVaoFloor);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glUseProgram(0);

		// 流体
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, mSkyBox->GetId());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mShadowMap->GetShadowMap());
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, mShadowMap->GetCausticMap());
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, mSlabWhite->mTexAlbedo);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, mSlabWhite->mTexRoughness);
		glBindImageTexture(0, bufferB, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
		glBindImageTexture(1, mTexThicknessBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mBufferFloor);
		glUseProgram(mFluidColorShaderProgram);
		labhelper::setUniformSlow(mFluidColorShaderProgram, "camToWorldRot", glm::transpose(mCamera->GetView()));
		labhelper::setUniformSlow(mFluidColorShaderProgram, "camToWorld", glm::inverse(mCamera->GetView()));
		labhelper::setUniformSlow(mFluidColorShaderProgram, "model", floorModel);
		labhelper::setUniformSlow(mFluidColorShaderProgram, "projection", mCamera->GetProjection());
		labhelper::setUniformSlow(mFluidColorShaderProgram, "lightView", mShadowMap->mLightView);
		labhelper::setUniformSlow(mFluidColorShaderProgram, "lightProjection", mShadowMap->mLightProjection);
		
		glBindVertexArray(mVaoParticals);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
		glUseProgram(0);
		

		//skybox
		mSkyBox->Draw(mVaoNull,mCamera->GetView(), mCamera->GetProjection());
		
	
	}

	void RenderWidget::LoadSkyBox()
	{
		mSkyBox = new SkyBox();
		mSkyBox->Create();
		std::vector<std::string> paths
		{
			"../scenes/skybox/right.jpg",
			"../scenes/skybox/left.jpg",
			
			"../scenes/skybox/bottom.jpg",
			"../scenes/skybox/top.jpg",
			"../scenes/skybox/front.jpg",
			"../scenes/skybox/back.jpg"
		};
		mSkyBox->LoadImages(paths);
		mSkyBox->BuildShader();
	}
}