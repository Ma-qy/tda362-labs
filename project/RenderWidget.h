#ifndef RENDER_WIDGET_H
#define RENDER_WIDGET_H
#define GLFW_INCLUDE_NONE
#include <vector>
#include "Shader.h"


#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>



#include "ComputeShader.h"
#include "ParticleSystem.h"
#include "RenderCamera.h"
#include "SkyBox.h"
#include "DepthFilter.h"
#include "Material.h"
#include "FluidShadowMap.h"

namespace Fluid3d {
    class RenderWidget
    {
    public:
        RenderWidget();
        ~RenderWidget();

        int32_t Init();

        // 上传、读取数据
        void UploadUniforms(Fluid3d::ParticalSystem3D* ps);
        void UploadParticalInfo(Fluid3d::ParticalSystem3D* ps);
        void DumpParticalInfo(Fluid3d::ParticalSystem3D* ps);

        // 求解、渲染
        void SolveParticals();
        void Update(ParticalSystem3D* ps);

        // window
        bool ShouldClose();
        void ProcessInput();
        void PollEvents();

        void GuiSettings(ParticalSystem3D* ps);
        void GenSimpleModelBuffer(const std::vector<glm::vec3>& vertices, std::vector<glm::vec3> normals);

    private:
        static void ResizeCallback(GLFWwindow* window, int width, int height);
        static void CursorPosCallBack(GLFWwindow* window, double xpos, double ypos);
        static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);

    private:
        bool CreateWindow();
        void UpdateFPS();
        void BuildShaders();
        void InitFilters();
        void GenerateFrameBuffers();
        void GenerateBuffers();
        void GenerateTextures();
        void LoadSkyBox();
        void CreateRenderAssets();
        void MakeVertexArrays();
        void DrawParticals();

        void AddFuild(ParticalSystem3D* ps);
        void ChangeFuild(ParticalSystem3D* ps);
        
        void AddjustSpherePos();
        void AddjustFloatingSphere();

        void Genuniformbuffer();

        int32_t Destroy();

    private:
        // window
        GLFWwindow* mWindow = nullptr;
        int mWindowWidth = 1280;
        int mWindowHeight = 0;

        // camera
        RenderCamera mCamera;
        bool mFirstMouseFlag = true;
        float mLastX;
        float mLastY;
        bool mLeftPressFlag = false;
        bool mRightPressFlag = false;
        bool mMiddlePressFlag = false;
        bool mPauseFlag = false;
        bool mAddFluid = false;

        // shaders
        Glb::Shader* mScreenQuad = nullptr;
        Glb::Shader* mDrawColor3d = nullptr;
        Glb::ComputeShader* mComputeParticals = nullptr;
        Glb::Shader* mPointSpriteZValue = nullptr;
        Glb::Shader* mPointSpriteThickness = nullptr;
        Glb::Shader* mDrawFluidColor = nullptr;
        Glb::Shader* mDrawModel = nullptr;
        Glb::Shader* mDrawSmoke = nullptr;
        Glb::ComputeShader* mComputeSmoke = nullptr;
        Glb::Shader* msimpleShader = nullptr;

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
        GLuint simpleVBO, simpleVAO;

        // buffers
        GLuint mCoordVertBuffer = 0;
        GLuint mBufferParticals = 0;
        GLuint mBufferBlocks = 0;
        GLuint mBufferFloor = 0;
        GLuint mUniformBuffer = 0;

        // texures
        GLuint mTestTexture = 0;
        GLuint mTexKernelBuffer = 0;
        GLuint mTexZBlurTempBuffer = 0;

        // SkyBox
        SkyBox* mSkyBox = nullptr;

        // Materials
        Material* mSlabWhite = nullptr;

        // time statistics
        int32_t mParticalNum = 0;
        float_t mUpdateTime = 0.0f;
        float_t updateTitleTime = 0.0f;
        float_t frameCount = 0.0f;

        // shadow map
        PointLight mLight;
        FluidShadowMap* mShadowMap;

        DepthFilter* mDepthFilter;

        glm::vec3 mExternelAccleration = { 0.0, 0.0, 0.0 };


        //gui setting 
        bool mChangeToSmoke = false;
        bool mResetPars = false;
        bool mSmokeCompute = false;
        bool mObstacleFlag = false;
        bool mFloatingBallFlag = false;
        

        int simplesize = 0;


        //paras for sphere
        float mSpherePoX = 0.3f;
        float mSpherePoY = 0.3f;
        float mSpherePoZ = 0.06f;
        float mSphereScale = 0.06f;

        glm::vec3 mBallPos = glm::vec3(0.4, 0.4, 0.4);
        float mBallScale = 0.02f;

        float BallPosx = 0.4f;
        float BallPosy = 0.4f;
        float BallPosz = 0.1f;
        float BallRadius = 0.02f;

        
    };
}

#endif // !RENDER_WIDGET_H

