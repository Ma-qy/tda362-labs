#include "RenderWidget.h"
#include "Global.h"

#include <iostream>
#include <fstream>
#include <thread>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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
glm::mat4 simpleModel;
glm::mat4 floatingBallModel;

namespace Fluid3d {
    RenderWidget::RenderWidget() {
        mWindowWidth = 1280;
        mWindowHeight = 720;
        floorModel = glm::mat4(1.0);
        floorModel = glm::scale(floorModel, glm::vec3(0.6));
        floorModel = glm::translate(floorModel, glm::vec3(0.5, 0.5, 0.0));

        simpleModel = glm::mat4(1.0);
        simpleModel = glm::translate(simpleModel, glm::vec3(mSpherePoX, mSpherePoY, mSpherePoZ));
        simpleModel = glm::scale(simpleModel, glm::vec3(mSphereScale));
    }

    RenderWidget::~RenderWidget() {
        Destroy();
    }

    int32_t RenderWidget::Init() {
        if (!CreateWindow()) {
            return -1;
        }

        BuildShaders();
        Genuniformbuffer();
        GenerateFrameBuffers();
        GenerateBuffers();
        GenerateTextures();

        InitFilters();
        LoadSkyBox();
        CreateRenderAssets();
        MakeVertexArrays(); // 生成画粒子的vao

        glGenVertexArrays(1, &mVaoNull);
        glEnable(GL_MULTISAMPLE);
    }

    void RenderWidget::UploadUniforms(Fluid3d::ParticalSystem3D* ps) {
        mComputeParticals->Use();
        mComputeParticals->SetUVec3("blockNum", ps->mBlockNum);
        mComputeParticals->SetVec3("blockSize", ps->mBlockSize);
        mComputeParticals->SetVec3("containerLowerBound", ps->mLowerBound);
        mComputeParticals->SetVec3("containerUpperBound", ps->mUpperBound);
        glUniform1iv(glGetUniformLocation(mComputeParticals->GetId(), "blockIdOffs"), ps->mBlockIdOffs.size(), ps->mBlockIdOffs.data());
        mComputeParticals->SetFloat("gSupportRadius", Para3d::supportRadius);
        mComputeParticals->SetFloat("gVolume", ps->mVolume);
        mComputeParticals->SetVec3("gGravityDir", -Glb::Z_AXIS);
        mComputeParticals->UnUse();

        mComputeSmoke->Use();
        mComputeSmoke->SetUVec3("blockNum", ps->mBlockNum);
        mComputeSmoke->SetVec3("blockSize", ps->mBlockSize);
        mComputeSmoke->SetVec3("containerLowerBound", ps->mLowerBound);
        mComputeSmoke->SetVec3("containerUpperBound", ps->mUpperBound);
        glUniform1iv(glGetUniformLocation(mComputeSmoke->GetId(), "blockIdOffs"), ps->mBlockIdOffs.size(), ps->mBlockIdOffs.data());
        mComputeSmoke->SetFloat("gSupportRadius", Para3d::supportRadius);
        mComputeSmoke->SetFloat("gVolume", ps->mVolume);
        mComputeSmoke->SetVec3("gGravityDir", -Glb::Z_AXIS);
        mComputeSmoke->UnUse();


        glBindTexture(GL_TEXTURE_1D, mTexKernelBuffer);
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RG32F, ps->mW.GetBufferSize(), 0, GL_RG, GL_FLOAT, ps->mW.GetData());
        glBindTexture(GL_TEXTURE_1D, 0);
    }

    void RenderWidget::UploadParticalInfo(Fluid3d::ParticalSystem3D* ps) {
        // 装粒子信息的buffer
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBufferParticals);
        glBufferData(GL_SHADER_STORAGE_BUFFER, ps->mParticalInfos.size() * sizeof(ParticalInfo3d), ps->mParticalInfos.data(), GL_DYNAMIC_COPY);

        // block区间buffer
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBufferBlocks);
        glBufferData(GL_SHADER_STORAGE_BUFFER, ps->mBlockExtens.size() * sizeof(glm::uvec2), ps->mBlockExtens.data(), GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        mParticalNum = ps->mParticalInfos.size();

        //buffer for floating sphere
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mUniformBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Fluid3d::SphereInfo), ps->FloatingSphere.data(), GL_DYNAMIC_COPY);
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void RenderWidget::DumpParticalInfo(Fluid3d::ParticalSystem3D* ps) {
        // 把粒子信息拷回CPU
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBufferParticals);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, mParticalNum * sizeof(ParticalInfo3d), (void*)ps->mParticalInfos.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


        if (mFloatingBallFlag) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, mUniformBuffer);
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Fluid3d::SphereInfo), (void*)ps->FloatingSphere.data());
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            
            mBallPos = ps->FloatingSphere[0].position;
            // std::cout << ps->FloatingSphere[0].buoyangcy.z << std::endl;
        }
        
    }

    void RenderWidget::SolveParticals() {
        if (mParticalNum <= 0 || mPauseFlag) {
            return;
        }

        //glFinish();

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mBufferParticals);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, mBufferBlocks);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, mUniformBuffer);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, mTexKernelBuffer);
        glBindImageTexture(0, mTestTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        if (mSmokeCompute) {
            mComputeSmoke->Use();
            mComputeSmoke->SetVec3("gExternelAccleration", mExternelAccleration);
            mComputeSmoke->SetFloat("gDensity0", Para3d::s_density0);
            mComputeSmoke->SetFloat("gMass", Para3d::s_gMass);
            mComputeSmoke->SetFloat("gStiffness", Para3d::s_stiffness);
            mComputeSmoke->SetFloat("gExponent", Para3d::s_exponent);
            mComputeSmoke->SetFloat("gViscosity", Para3d::s_viscosity);
            mComputeSmoke->SetFloat("gGravity", Para3d::s_gravity);
            mComputeSmoke->SetInt("particalNum", mParticalNum);

            for (int pass = 0; pass <= 1; pass++) {
                mComputeSmoke->SetUInt("pass", pass);
                glDispatchCompute(mParticalNum / 512 + 1, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            }
            mComputeSmoke->UnUse();
        }
        else {
            if (mChangeToSmoke) {
                mComputeParticals->Use();
                mComputeParticals->SetVec3("gExternelAccleration", mExternelAccleration);
                mComputeParticals->SetFloat("gDensity0", Para3d::s_density0);
                mComputeParticals->SetFloat("gMass", Para3d::s_gMass);
                mComputeParticals->SetFloat("gStiffness", Para3d::s_stiffness);
                mComputeParticals->SetFloat("gExponent", Para3d::s_exponent);
                mComputeParticals->SetFloat("gViscosity", Para3d::s_viscosity);
                mComputeParticals->SetFloat("gGravity", Para3d::s_gravity);
                mComputeParticals->SetInt("particalNum", mParticalNum);
                mComputeParticals->SetBool("ObstacleFlag", mObstacleFlag);
            }
            else {
                mComputeParticals->Use();
                mComputeParticals->SetVec3("gExternelAccleration", mExternelAccleration);
                mComputeParticals->SetFloat("gDensity0", Para3d::density0);
                mComputeParticals->SetFloat("gMass", Para3d::gMass);
                mComputeParticals->SetFloat("gStiffness", Para3d::stiffness);
                mComputeParticals->SetFloat("gExponent", Para3d::exponent);
                mComputeParticals->SetFloat("gViscosity", Para3d::viscosity);
                mComputeParticals->SetFloat("gGravity", Para3d::gravity);
                mComputeParticals->SetInt("particalNum", mParticalNum);
                mComputeParticals->SetBool("ObstacleFlag", mObstacleFlag);
            }

            for (int pass = 0; pass <= 1; pass++) {
                mComputeParticals->SetUInt("pass", pass);
                glDispatchCompute(mParticalNum / 512 + 1, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            }
            mComputeParticals->UnUse();
        }
    }



    void RenderWidget::Update(ParticalSystem3D* ps) {
        DrawParticals();
        
        ChangeFuild(ps);

        AddjustFloatingSphere();
       

        // Gui
        GuiSettings(ps);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        UpdateFPS();    // 显示FPS

        glfwSwapBuffers(mWindow);   // 交换前后缓冲
        AddFuild(ps);
    }

    bool RenderWidget::ShouldClose() {
        return glfwWindowShouldClose(mWindow);
    }

    void RenderWidget::ProcessInput() {
        if (glfwGetKey(mWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(mWindow, true);
        }

        return;
    }

    void RenderWidget::PollEvents() {
        glfwPollEvents();
    }

    void RenderWidget::GuiSettings(ParticalSystem3D* ps)
    {
        ImGui::Begin("Settings");
        ImGui::Text("Menu!");
        
        ImGui::Checkbox("Change To Smoke", &mChangeToSmoke);
        
        ImGui::Checkbox("Smoke Compute Shader", &mSmokeCompute);
        if (ImGui::Button("Reset All")) {
            mResetPars = true;
        }

        ImGui::InputFloat("Position X", &mSpherePoX);
        ImGui::InputFloat("Position Y", &mSpherePoY);
        ImGui::InputFloat("Position Z", &mSpherePoZ);
        ImGui::InputFloat("Scale", &mSphereScale);

        if (ImGui::Button("Apply Position")) {
            AddjustSpherePos();
        }

        ImGui::Checkbox("Add Obstacle", &mObstacleFlag);
        ImGui::Checkbox("Add Balls", &mFloatingBallFlag);

        
        ImGui::InputFloat("Ball X", &BallPosx);
        ImGui::InputFloat("Ball Y", &BallPosy);
        ImGui::InputFloat("Ball Z", &BallPosz);
        ImGui::InputFloat("Ball Radius", &BallRadius);
        if (ImGui::Button("Reset Ball")) {
            mBallPos = glm::vec3(BallPosx, BallPosy, BallPosz);
            mBallScale = BallRadius;
            ps->SetFloatingBall(mBallPos, BallRadius);
        }

        ImGui::End();

        return;
    }

    void RenderWidget::AddjustSpherePos() {
        simpleModel = glm::mat4(1.0);
        simpleModel = glm::translate(simpleModel, glm::vec3(mSpherePoX, mSpherePoY, mSpherePoZ));
        simpleModel = glm::scale(simpleModel, glm::vec3(mSphereScale));
    }

    void RenderWidget::AddjustFloatingSphere()
    {
        if (mFloatingBallFlag) {
            
            floatingBallModel = glm::mat4(1.0);
            floatingBallModel = glm::translate(floatingBallModel, mBallPos);
            floatingBallModel = glm::scale(floatingBallModel, glm::vec3(mBallScale));
        }

    }

    void RenderWidget::Genuniformbuffer()
    {
        glGenBuffers(1, &mUniformBuffer);
        
    }

    void RenderWidget::ResizeCallback(GLFWwindow* window, int width, int height) {
        // 找到this指针
        auto thisPtr = reinterpret_cast<RenderWidget*>(glfwGetWindowUserPointer(window));
        glViewport(0, 0, width, height);
        thisPtr->mCamera.SetPerspective(float(width) / float(height));
    }

    void RenderWidget::CursorPosCallBack(GLFWwindow* window, double xpos, double ypos) {
        auto thisPtr = reinterpret_cast<RenderWidget*>(glfwGetWindowUserPointer(window));
        if (!(thisPtr->mLeftPressFlag || thisPtr->mRightPressFlag || thisPtr->mMiddlePressFlag)) {
            return;
        }

        if (thisPtr->mFirstMouseFlag) {
            thisPtr->mLastX = xpos;
            thisPtr->mLastY = ypos;
            thisPtr->mFirstMouseFlag = false;
        }

        float xOffset = xpos - thisPtr->mLastX;
        float yOffset = ypos - thisPtr->mLastY;

        if (thisPtr->mLeftPressFlag) {
            thisPtr->mCamera.ProcessRotate(glm::vec2(xOffset, yOffset));
        }
        else if (thisPtr->mRightPressFlag) {
            thisPtr->mCamera.ProcessMove(glm::vec2(xOffset, yOffset));
        }
        else if (thisPtr->mMiddlePressFlag) {
            thisPtr->mExternelAccleration = thisPtr->mCamera.GetRight() * xOffset - thisPtr->mCamera.GetUp() * yOffset;
        }

        thisPtr->mLastX = xpos;
        thisPtr->mLastY = ypos;
    }

    void RenderWidget::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        auto thisPtr = reinterpret_cast<RenderWidget*>(glfwGetWindowUserPointer(window));
        if (action == GLFW_PRESS) {
            thisPtr->mFirstMouseFlag = true;
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                thisPtr->mLeftPressFlag = true;
            }
            else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                thisPtr->mRightPressFlag = true;

            }
            else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
                thisPtr->mMiddlePressFlag = true;
            }

        }
        else if (action == GLFW_RELEASE) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                thisPtr->mLeftPressFlag = false;
            }
            else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                thisPtr->mRightPressFlag = false;
            }
            else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
                thisPtr->mMiddlePressFlag = false;
                thisPtr->mExternelAccleration = glm::vec3(0.0);
            }
        }
    }

    void RenderWidget::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        auto thisPtr = reinterpret_cast<RenderWidget*>(glfwGetWindowUserPointer(window));
        thisPtr->mCamera.ProcessScale(static_cast<float>(yoffset));
    }

    void RenderWidget::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
        auto thisPtr = reinterpret_cast<RenderWidget*>(glfwGetWindowUserPointer(window));
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            thisPtr->mPauseFlag = true;
        }
        if (key == GLFW_KEY_K && action == GLFW_PRESS) {
            thisPtr->mAddFluid = true;
        }
        
        else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
            thisPtr->mPauseFlag = false;
        }

    }

    bool RenderWidget::CreateWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);  // 版本
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 9);    // 多重采样

        // 创建窗口
        mWindow = glfwCreateWindow(mWindowWidth, mWindowHeight, "Fluid Simulation", NULL, NULL);
        if (mWindow == nullptr)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        glfwSetWindowPos(mWindow, 100, 100);
        glfwMakeContextCurrent(mWindow);

        // 注册回调函数
        glfwSetWindowUserPointer(mWindow, this);
        glfwSetFramebufferSizeCallback(mWindow, ResizeCallback);
        glfwSetCursorPosCallback(mWindow, CursorPosCallBack);
        glfwSetMouseButtonCallback(mWindow, MouseButtonCallback);
        glfwSetScrollCallback(mWindow, ScrollCallback);
        glfwSetKeyCallback(mWindow, KeyCallback);

       if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return false;
        }


        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        
        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(mWindow, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
        ImGui_ImplOpenGL3_Init();

        return true;
    }

    void RenderWidget::UpdateFPS() {
        // 计算FPS
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - mUpdateTime;
        mUpdateTime = currentTime;
        updateTitleTime += deltaTime;
        frameCount += 1.0f;

        // 更新FPS
        if (updateTitleTime > 1.0f) {
            char title[128] = "";
            sprintf_s(title, "Fluid Simulation FPS=%.2f", frameCount / updateTitleTime);
            glfwSetWindowTitle(mWindow, title);
            updateTitleTime = 0.0f;
            frameCount = 0.0f;
        }

        return;
    }

    void RenderWidget::BuildShaders() {
        mComputeParticals = new Glb::ComputeShader("ComputeParticals");
        std::vector<std::string> computeShaderpaths = {
            std::string("../project/particleUpdate.comp"),
        };
        mComputeParticals->BuildFromFiles(computeShaderpaths);
        mComputeParticals->Use();
        glUniform1i(glGetUniformLocation(mComputeParticals->GetId(), "kernelBuffer"), 1);
        mComputeParticals->UnUse();

        mComputeSmoke = new Glb::ComputeShader("ComputeSmoke");
        std::vector<std::string> smokecomputeShaderpaths = {
            std::string("../project/SmokeUpdate.comp"),
        };
        mComputeSmoke->BuildFromFiles(smokecomputeShaderpaths);
        mComputeSmoke->Use();
        glUniform1i(glGetUniformLocation(mComputeSmoke->GetId(), "kernelBuffer"), 1);
        mComputeSmoke->UnUse();

        msimpleShader = new Glb::Shader();
        std::string vertPath = "../project/simple.vert";
        std::string fragPath = "../project/simple.frag";
        msimpleShader->BuildFromFile(vertPath, fragPath);
        msimpleShader->Use();
        
        msimpleShader->SetVec3("material_color", glm::vec3(1.0, 0.5, 0.2));
        msimpleShader->UnUse();


        mScreenQuad = new Glb::Shader();
        std::string screenQuadVertPath = "../project/ScreenQuad.vert";
        std::string screenQuadFragPath = "../project/ScreenQuad.frag";
        mScreenQuad->BuildFromFile(screenQuadVertPath, screenQuadFragPath);
        mScreenQuad->Use();
        glUniform1i(glGetUniformLocation(mScreenQuad->GetId(), "tex"), 0);
        mScreenQuad->UnUse();

        mDrawColor3d = new Glb::Shader();
        std::string drawColorVertPath = "../project/DrawColor3d.vert";
        std::string drawColorFragPath = "../project/DrawColor3d.frag";
        mDrawColor3d->BuildFromFile(drawColorVertPath, drawColorFragPath);

        mPointSpriteZValue = new Glb::Shader();
        std::string pointSpriteZValueVertPath = "../project/PointSprite.vert";
        std::string pointSpriteZValueGeomPath = "../project/PointSprite.geom";
        std::string pointSpriteZValueFragPath = "../project/PointSpriteZValue.frag";
        mPointSpriteZValue->BuildFromFile(pointSpriteZValueVertPath, pointSpriteZValueFragPath, pointSpriteZValueGeomPath);
        mPointSpriteZValue->Use();
        mPointSpriteZValue->SetFloat("zFar", Para3d::zFar);
        mPointSpriteZValue->SetFloat("zNear", Para3d::zNear);
        mPointSpriteZValue->UnUse();

        mPointSpriteThickness = new Glb::Shader();
        std::string pointSpriteThicknessVertPath = "../project/PointSprite.vert";
        std::string pointSpriteThicknessGeomPath = "../project/PointSprite.geom";
        std::string pointSpriteThicknessFragPath = "../project/PointSpriteThickness.frag";
        mPointSpriteThickness->BuildFromFile(pointSpriteThicknessVertPath, pointSpriteThicknessFragPath, pointSpriteThicknessGeomPath);

        mDrawFluidColor = new Glb::Shader();
        std::string drawFluidColorVertPath = "../project/DrawFluidColor.vert";
        std::string drawFluidColorFragPath = "../project/DrawFluidColor.frag";
        mDrawFluidColor->BuildFromFile(drawFluidColorVertPath, drawFluidColorFragPath);
        mDrawFluidColor->Use();
        mDrawFluidColor->SetFloat("zFar", Para3d::zFar);
        mDrawFluidColor->SetFloat("zNear", Para3d::zNear);
        mDrawFluidColor->SetFloat("eta", 1.0 / Para3d::IOR);
        mDrawFluidColor->SetVec3("f0", Para3d::F0);
        mDrawFluidColor->SetVec4("cameraIntrinsic", Glb::ProjToIntrinsic(mCamera.GetProjection(), mWindowWidth, mWindowHeight));
        mDrawFluidColor->SetVec3("fluidColor", Para3d::FLUID_COLOR);
        mDrawFluidColor->SetVec3("shadowColor", Para3d::SHADOW_COLOR);
        mDrawFluidColor->SetFloat("thicknessFactor", Para3d::THICKNESS_FACTOR);
       
        mDrawFluidColor->UnUse();

        mDrawModel = new Glb::Shader();
        std::string drawModelVertPath = "../project/DrawModel.vert";
        std::string drawModelFragPath = "../project/DrawModel.frag";
        mDrawModel->BuildFromFile(drawModelVertPath, drawModelFragPath);

        Glb::RandomGenerator rand;
        mDrawSmoke = new Glb::Shader();
        std::string drawSmokeVertPath = "../project/SmokeDraw.vert";
        std::string drawSmokeFragPath = "../project/SmokeDraw.frag";
        mDrawSmoke->BuildFromFile(drawSmokeVertPath, drawSmokeFragPath);
        mDrawSmoke->Use();
        mDrawSmoke->SetFloat("zFar", Para3d::zFar);
        mDrawSmoke->SetFloat("zNear", Para3d::zNear);
        mDrawSmoke->SetFloat("eta", 1.0 / 1.001);   //for smoke
        mDrawSmoke->SetVec3("f0", Para3d::F0);
        mDrawSmoke->SetVec4("cameraIntrinsic", Glb::ProjToIntrinsic(mCamera.GetProjection(), mWindowWidth, mWindowHeight));
        mDrawSmoke->SetVec3("shadowColor", { 0.5, 0.5, 0.5 });
        mDrawSmoke->SetFloat("thicknessFactor", Para3d::THICKNESS_FACTOR);
        mDrawSmoke->SetVec3("fluidColor", { 0.4, 0.4, 0.4 });  //for smoke
        
        mDrawSmoke->UnUse();



    }

    void RenderWidget::InitFilters() {
        mDepthFilter = new DepthFilter();
        mDepthFilter->Create(8.0, 0.025);
    }

    void RenderWidget::GenerateFrameBuffers() {
        // depth framebuffer
        glGenFramebuffers(1, &mFboDepth);

        glGenTextures(1, &mTexZBuffer);
        glBindTexture(GL_TEXTURE_2D, mTexZBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mWindowWidth, mWindowHeight, 0, GL_RED, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenRenderbuffers(1, &mRboDepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, mRboDepthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mWindowWidth, mWindowHeight);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, mFboDepth);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexZBuffer, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRboDepthBuffer);
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

    void RenderWidget::GenerateBuffers() {
        glGenBuffers(1, &mCoordVertBuffer);     // coord vbo
        glGenBuffers(1, &mBufferParticals);     // ssbo
        glGenBuffers(1, &mBufferBlocks);
        glGenBuffers(1, &mBufferFloor);


    }

    void RenderWidget::GenerateTextures() {
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

    void RenderWidget::LoadSkyBox() {
        mSkyBox = new SkyBox();
        mSkyBox->Create();
        std::vector<std::string> paths
        {
            "../scenes/skybox/right.jpg",
            "../scenes/skybox/left.jpg",
            "../scenes/skybox/top.jpg",
            "../scenes/skybox/bottom.jpg",
            "../scenes/skybox/front.jpg",
            "../scenes/skybox/back.jpg"
        };
        mSkyBox->LoadImages(paths);
        mSkyBox->BuildShader();
    }

    void RenderWidget::CreateRenderAssets() {
        // 材质
        mSlabWhite = new Material();
        mSlabWhite->Create();
        std::string albedoPath = "../scenes/Slab/TexturesCom_Marble_SlabWhite_1K_albedo.png";
        std::string roughnessPath = "../scenes/Slab/TexturesCom_Marble_SlabWhite_1K_roughness.png";
        mSlabWhite->LoadTextures(albedoPath, roughnessPath);

        // 灯光
        mLight.pos = glm::vec3(-0.8, -0.8, 2.0);
        mLight.dir = glm::vec3(0.5, 0.5, -1.0);
        mLight.aspect = 1.0f;
        mLight.fovy = 30.0;

        // 阴影贴图
        mShadowMap = new FluidShadowMap();
        mShadowMap->SetImageSize(mWindowWidth, mWindowHeight);
        mShadowMap->SetLightInfo(mLight);
        mShadowMap->SetIor(Para3d::IOR);
        mShadowMap->Init();
    }

    void RenderWidget::MakeVertexArrays() {
        glGenVertexArrays(1, &mVaoParticals);
        glBindVertexArray(mVaoParticals);
        glBindBuffer(GL_ARRAY_BUFFER, mBufferParticals);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticalInfo3d), (void*)offsetof(ParticalInfo3d, position));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ParticalInfo3d), (void*)offsetof(ParticalInfo3d, density));
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
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float_t)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    void RenderWidget::DrawParticals() {
        glFinish();
        //// 以点的形式画粒子
        //glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        //glEnable(GL_DEPTH_TEST);
        //glDepthFunc(GL_LEQUAL);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glEnable(GL_PROGRAM_POINT_SIZE);
        //mDrawColor3d->Use();
        //mDrawColor3d->SetMat4("view", mCamera.GetView());
        //mDrawColor3d->SetMat4("projection", mCamera.GetProjection());
        //glBindVertexArray(mVaoCoord);
        //glDrawElements(GL_LINES, 6, GL_UNSIGNED_INT, indices);
        //glBindVertexArray(mVaoParticals);
        //glDrawArrays(GL_POINTS, 0, mParticalNum);
        //mSkyBox->Draw(mWindow, mVaoNull, mCamera.GetView(), mCamera.GetProjection());
        //mDrawColor3d->UnUse();

        // 预处理
        glBindFramebuffer(GL_FRAMEBUFFER, mFboDepth);
        glViewport(0, 0, mWindowWidth, mWindowHeight);
        glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 画深度图
        mPointSpriteZValue->Use();
        mPointSpriteZValue->SetMat4("view", mCamera.GetView());
        mPointSpriteZValue->SetMat4("projection", mCamera.GetProjection());
        mPointSpriteZValue->SetFloat("particalRadius", 0.01f);
        mPointSpriteZValue->SetVec3("cameraUp", mCamera.GetUp());
        mPointSpriteZValue->SetVec3("cameraRight", mCamera.GetRight());
        mPointSpriteZValue->SetVec3("cameraFront", mCamera.GetFront());
        glBindVertexArray(mVaoParticals);
        glDrawArrays(GL_POINTS, 0, mParticalNum);
        mPointSpriteZValue->UnUse();

        // 模糊深度
        GLuint bufferA = mTexZBuffer;
        GLuint bufferB = mTexZBlurTempBuffer;
        mDepthFilter->Filter(bufferA, bufferB, glm::ivec2(mWindowWidth, mWindowHeight));

        // 画厚度图
        glBindFramebuffer(GL_FRAMEBUFFER, mFboThickness);
        glViewport(0, 0, mWindowWidth, mWindowHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        mPointSpriteThickness->Use();
        mPointSpriteThickness->SetMat4("view", mCamera.GetView());
        mPointSpriteThickness->SetMat4("projection", mCamera.GetProjection());
        mPointSpriteThickness->SetFloat("particalRadius", 0.01f);
        mPointSpriteThickness->SetVec3("cameraUp", mCamera.GetUp());
        mPointSpriteThickness->SetVec3("cameraRight", mCamera.GetRight());
        mPointSpriteThickness->SetVec3("cameraFront", mCamera.GetFront());
        glBindVertexArray(mVaoParticals);
        glDrawArrays(GL_POINTS, 0, mParticalNum);
        mPointSpriteThickness->UnUse();
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        // 阴影
        mShadowMap->Update(mVaoParticals, mParticalNum, mDepthFilter);

        // 渲染
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, mWindowWidth, mWindowHeight);
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        

        
        
        //// 画流体

        if (!mChangeToSmoke) {
            // floor with caustic
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
            mDrawModel->Use();
            mDrawModel->SetBool("CausticFlag", true);
            mDrawModel->SetMat4("model", floorModel);
            mDrawModel->SetMat4("view", mCamera.GetView());
            mDrawModel->SetMat4("projection", mCamera.GetProjection());
            mDrawModel->SetMat4("lightView", mShadowMap->mLightView);
            mDrawModel->SetMat4("lightProjection", mShadowMap->mLightProjection);
            glBindVertexArray(mVaoFloor);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
            mDrawModel->UnUse();

            mShadowMap->DrawCaustic(&mCamera, mVaoNull, floorModel);

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
            mDrawFluidColor->Use();
            mDrawFluidColor->SetMat4("camToWorldRot", glm::transpose(mCamera.GetView()));
            mDrawFluidColor->SetMat4("camToWorld", glm::inverse(mCamera.GetView()));
            mDrawFluidColor->SetMat4("model", floorModel);
            mDrawFluidColor->SetMat4("projection", mCamera.GetProjection());
            mDrawFluidColor->SetMat4("lightView", mShadowMap->mLightView);
            mDrawFluidColor->SetMat4("lightProjection", mShadowMap->mLightProjection);
            mDrawFluidColor->SetBool("obstacleFlag", mObstacleFlag);
            glBindVertexArray(mVaoParticals);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);
            mDrawFluidColor->UnUse();
        }
        else {
            // floor without caustic
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
            mDrawModel->Use();
            mDrawModel->SetBool("CausticFlag", false);
            mDrawModel->SetMat4("model", floorModel);
            mDrawModel->SetMat4("view", mCamera.GetView());
            mDrawModel->SetMat4("projection", mCamera.GetProjection());
            mDrawModel->SetMat4("lightView", mShadowMap->mLightView);
            mDrawModel->SetMat4("lightProjection", mShadowMap->mLightProjection);
            glBindVertexArray(mVaoFloor);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
            mDrawModel->UnUse();


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
            mDrawSmoke->Use();
            mDrawSmoke->SetMat4("camToWorldRot", glm::transpose(mCamera.GetView()));
            mDrawSmoke->SetMat4("camToWorld", glm::inverse(mCamera.GetView()));
            mDrawSmoke->SetMat4("model", floorModel);
            mDrawSmoke->SetMat4("projection", mCamera.GetProjection());
            mDrawSmoke->SetMat4("lightView", mShadowMap->mLightView);
            mDrawSmoke->SetMat4("lightProjection", mShadowMap->mLightProjection);
            glBindVertexArray(mVaoParticals);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);
            mDrawSmoke->UnUse();
        }
        

        if (mObstacleFlag) {
            // Render the model
            msimpleShader->Use();
            msimpleShader->SetMat4("modelViewProjectionMatrix", mCamera.GetProjection()* mCamera.GetView()* simpleModel);
            
            msimpleShader->SetMat4("model", simpleModel);
            msimpleShader->SetVec3("lightPos", mLight.pos);
            msimpleShader->SetVec3("viewPos", mCamera.GetPosition());
            msimpleShader->SetVec3("material_color", glm::vec3(1.0, 0.5, 0.2));

            glBindVertexArray(simpleVAO);
            glDrawArrays(GL_TRIANGLES, 0, simplesize);
            glBindVertexArray(0);
            msimpleShader->UnUse();
        }

        if (mFloatingBallFlag) {
            msimpleShader->Use();
            msimpleShader->SetMat4("modelViewProjectionMatrix", mCamera.GetProjection()* mCamera.GetView()* floatingBallModel);
            msimpleShader->SetMat4("model", floatingBallModel);
            msimpleShader->SetVec3("lightPos", mLight.pos);
            msimpleShader->SetVec3("viewPos", mCamera.GetPosition());

            msimpleShader->SetVec3("material_color", glm::vec3(1.0, 0.0, 0.2));
            glBindVertexArray(simpleVAO);
            glDrawArrays(GL_TRIANGLES, 0, simplesize);
            glBindVertexArray(0);
            msimpleShader->UnUse();
        }
        

        mSkyBox->Draw(mWindow, mVaoNull, mCamera.GetView(), mCamera.GetProjection());
        


//#define TEST_QUAD
#ifdef TEST_QUAD
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mTexThicknessBuffer);
        mScreenQuad->Use();
        glBindVertexArray(mVaoNull);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        mScreenQuad->UnUse();
#endif //TEST_QUAD
    }

    void RenderWidget::AddFuild(ParticalSystem3D* ps)
    {
        if (mAddFluid == true) {
            std::cout << "add fluid" << std::endl;
            ps->AddFluidBlock(glm::vec3(0.15, 0.15, 0.1), glm::vec3(0.15, 0.15, 0.3), glm::vec3(0.0, 0.0, 1.0), 0.020);
        }
        
        mAddFluid = false;
    }

    void RenderWidget::ChangeFuild(ParticalSystem3D* ps)
    {
        if (mResetPars) {
            ps->RemoveAllFluid();
        }
        mResetPars = false;
    }

    void RenderWidget::GenSimpleModelBuffer(const std::vector<glm::vec3>& vertices, std::vector<glm::vec3> normals)
    {
        glGenVertexArrays(1, &simpleVAO);
        glGenBuffers(1, &simpleVBO);

        glBindVertexArray(simpleVAO);

        glBindBuffer(GL_ARRAY_BUFFER, simpleVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);

        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(1);


        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        simplesize = vertices.size();

    }

    
    int32_t RenderWidget::Destroy() {
        delete mScreenQuad;
        delete mDrawColor3d;
        delete mComputeParticals;

        glDeleteVertexArrays(1, &mVaoNull);
        glDeleteVertexArrays(1, &mVaoParticals);
        glDeleteVertexArrays(1, &mVaoCoord);

        glDeleteBuffers(1, &mCoordVertBuffer);
        glDeleteBuffers(1, &mBufferParticals);
        glDeleteBuffers(1, &mBufferBlocks);

        glDeleteTextures(1, &mTestTexture);
        glDeleteTextures(1, &mTexKernelBuffer);



        glfwTerminate();
        return 0;
    }


}

