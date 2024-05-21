#include "RenderWidget.h"
#include "Global.h"
#include "ParticleSystem.h"
#include "labhelper.h"
#include "RenderCamera.h"

#include <iostream>
#include <thread>
#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

using namespace glm;

SDL_Window* g_window = nullptr;
float currentTime = 0.0f;
float previousTime = 0.0f;
float deltaTime = 0.0f;
bool showUI = false;

ivec2 g_prevMouseCoords = { -1, -1 };
bool g_isMouseDragging = false;

Fluid3d::RenderCamera mCamera;
vec3 worldUp = mCamera.GetUp();
vec3 cameraDirection = mCamera.GetFront();
vec3 cameraPosition = mCamera.GetPosition();
vec3 cameraRight = mCamera.GetRight();
float cameraSpeed = 1.f;



///////////////////////////////////////////////////////////////////////////////
/// This function is used to update the scene according to user input
///////////////////////////////////////////////////////////////////////////////
bool handleEvents(void)
{
	// Allow ImGui to capture events.
	ImGuiIO& io = ImGui::GetIO();

	// check events (keyboard among other)
	SDL_Event event;
	bool quitEvent = false;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSdlGL3_ProcessEvent(&event);

		if (event.type == SDL_QUIT || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE))
		{
			quitEvent = true;
		}
		else if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_g)
		{
			showUI = !showUI;
		}
		else if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_PRINTSCREEN)
		{
			labhelper::saveScreenshot();
		}
		if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT
			&& (!showUI || !io.WantCaptureMouse))
		{
			g_isMouseDragging = true;
			int x;
			int y;
			SDL_GetMouseState(&x, &y);
			g_prevMouseCoords.x = x;
			g_prevMouseCoords.y = y;
		}

		if (!(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)))
		{
			g_isMouseDragging = false;
		}

		if (event.type == SDL_MOUSEMOTION && g_isMouseDragging && !io.WantCaptureMouse)
		{
			// More info at https://wiki.libsdl.org/SDL_MouseMotionEvent
			int delta_x = event.motion.x - g_prevMouseCoords.x;
			int delta_y = event.motion.y - g_prevMouseCoords.y;

			glm::vec2 offset(delta_x, delta_y);
			//std::cout << offset.x << offset.y << "rotate offset" << std::endl;
			mCamera.ProcessRotate(offset);

			g_prevMouseCoords.x = event.motion.x;
			g_prevMouseCoords.y = event.motion.y;
		}
	}

	// check keyboard state (which keys are still pressed)
	const uint8_t* state = SDL_GetKeyboardState(nullptr);

	static bool was_shift_pressed = state[SDL_SCANCODE_LSHIFT];
	if (was_shift_pressed && !state[SDL_SCANCODE_LSHIFT])
	{
		cameraSpeed /= 5;
	}
	if (!was_shift_pressed && state[SDL_SCANCODE_LSHIFT])
	{
		cameraSpeed *= 5;
	}
	was_shift_pressed = state[SDL_SCANCODE_LSHIFT];




	if (state[SDL_SCANCODE_W])
	{
		mCamera.ProcessMove(glm::vec2(0.0f, cameraSpeed * deltaTime));
	}
	if (state[SDL_SCANCODE_S])
	{
		mCamera.ProcessMove(glm::vec2(0.0f, -cameraSpeed * deltaTime));
	}
	if (state[SDL_SCANCODE_A])
	{
		mCamera.ProcessMove(glm::vec2(-cameraSpeed * deltaTime, 0.0f));
	}
	if (state[SDL_SCANCODE_D])
	{
		mCamera.ProcessMove(glm::vec2(cameraSpeed * deltaTime, 0.0f));
	}
	if (state[SDL_SCANCODE_Q])
	{
		mCamera.ProcessScale(-cameraSpeed * deltaTime);
	}
	if (state[SDL_SCANCODE_E])
	{
		mCamera.ProcessScale(cameraSpeed * deltaTime);
	}

	return quitEvent;
}


int main() {

    Fluid3d::ParticleSystem ps;
    ps.SetContainerSize(glm::vec3(0.0, -0.0, -0.2), glm::vec3(0.6, 0.6, 0.6));
    ps.AddFluidBlock(glm::vec3(0.35, 0.05, -0.0), glm::vec3(0.15, 0.15, 0.3), glm::vec3(0.0, 0.0, 3.0), 0.005 * 0.8);
    //ps.AddFluidBlock(glm::vec3(0.35, 0.05, 0.25), glm::vec3(0.15, 0.15, 0.3), glm::vec3(0.0, 0.0, -4.0), 0.01 * 0.8);
    //ps.AddFluidBlock(glm::vec3(0.2, 0.2, 0.25), glm::vec3(0.2, 0.2, 0.3), glm::vec3(0.0, 0.0, 0.0), 0.01 * 0.8);
    ps.UpdateData();
    std::cout << "partical num = " << ps.mParticalInfos.size() << std::endl;

    g_window = labhelper::init_window_SDL("OpenGL Project");

    

    Fluid3d::RenderWidget* renderer = new Fluid3d::RenderWidget(&mCamera);
    renderer->Init();
    renderer->UploadUniforms(ps);
    
   
    bool stopRendering = false;
    auto startTime = std::chrono::system_clock::now();


    while (!stopRendering) {

        //update currentTime
        std::chrono::duration<float> timeSinceStart = std::chrono::system_clock::now() - startTime;
        previousTime = currentTime;
        currentTime = timeSinceStart.count();
        deltaTime = currentTime - previousTime;


        // check events (keyboard among other)
        stopRendering = handleEvents();

		


        // Inform imgui of new frame
        ImGui_ImplSdlGL3_NewFrame(g_window);

        for (int i = 0; i < Para3d::substep; i++) {
            ps.UpdateData();
            renderer->UploadParticleInfo(ps);
            renderer->SolveParticals();
            renderer->DumpParticleInfo(ps);
        }
        renderer->Update();
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        //renderer.PollEvents();
        SDL_GL_SwapWindow(g_window);

    }

    return 0;
}