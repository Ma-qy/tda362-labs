#include <iostream>
#include "RenderWidget.h"
#include <thread>
#include "Global.h"
#include "ParticleSystem.h"
#include "Model.h"
#include "Shader.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"



int main() {

    Fluid3d::ParticalSystem3D* ps = new Fluid3d::ParticalSystem3D();
    ps->SetContainerSize(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.45, 0.45, 0.6));
    //ps->AddFluidBlock(glm::vec3(0.05, 0.25, 0.25), glm::vec3(0.15, 0.15, 0.3), glm::vec3(0.0, 0.0, -4.0), 0.01);
    
    //ps->AddFluidBlock(glm::vec3(0.25, 0.05, 0.25), glm::vec3(0.15, 0.15, 0.3), glm::vec3(0.0, 0.0, -4.0), 0.01);

    ps->AddFluidBlock(glm::vec3(0.2, 0.2, 0.3), glm::vec3(0.2, 0.2, 0.3), glm::vec3(0.0, 0.0, 0.0), 0.01);
    ps->UpdateData();

    glm::vec3 floatingPos = glm::vec3(0.4, 0.4, 0.4);
    float floatingRadius = 0.02f;
    ps->SetFloatingBall(floatingPos, floatingRadius);


    std::cout << "partical num = " << ps->mParticalInfos.size() << std::endl;

    Fluid3d::RenderWidget* renderer = new Fluid3d::RenderWidget();
    renderer->Init();
    renderer->UploadUniforms(ps);

    OBJLoader loader;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    if (!loader.loadOBJ("../scenes/sphere.obj", vertices, uvs, normals)) {
        return -1;
    }
    renderer->GenSimpleModelBuffer(vertices, normals);



    while (!renderer->ShouldClose()) {

        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        
        
        //ImGui::ShowDemoWindow(); // Show demo window! :)

        renderer->ProcessInput();    // 处理输入事件
        for (int i = 0; i < Para3d::substep; i++) {
            ps->UpdateData();
            renderer->UploadParticalInfo(ps);
            renderer->SolveParticals();
            renderer->DumpParticalInfo(ps);
        }
        
        renderer->Update(ps);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        renderer->PollEvents();
    }



    return 0;
}

