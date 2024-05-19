#ifndef SKY_BOX_H
#define SKY_BOX_H

#include <vector>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>



namespace Fluid3d {
    class SkyBox {
    public:
        SkyBox();
        ~SkyBox();

        void Create();
        void Destroy();

        int32_t LoadImages(std::vector<std::string> paths);

        void BuildShader();

        void Draw(GLuint nullVao, glm::mat4 view, glm::mat4 proj);

        GLuint GetId();

    private:
        GLuint mId = 0;
        GLuint mShader = 0;

    };
}


#endif // !SKY_BOX_H