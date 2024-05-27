#ifndef PARTICAL_SYSTEM_3D_H
#define PARTICAL_SYSTEM_3D_H


#include <glm/glm.hpp>
#include <vector>
#include "Parameter3d.h"
#include "WCubicSpline.h"

namespace Fluid3d {
    struct ParticalInfo3d
    {
        alignas(16) glm::vec3 position;
        alignas(16) glm::vec3 velocity;
        alignas(16) glm::vec3 accleration;
        alignas(4) float_t density;
        alignas(4) float_t pressure;
        alignas(4) float_t pressDivDens2;;
        alignas(4) uint32_t blockId;
    };

    struct NeighborInfo {
        alignas(16) glm::vec3 radius;
        alignas(4) float_t distance;
        alignas(4) int32_t neighborId;
    };


    struct alignas(16) SphereInfo {
        alignas(16) glm::vec3 position;
        alignas(4) float_t radius;
        alignas(16) glm::vec3 velocity;
        alignas(4) float_t mass;
        alignas(16) glm::vec3 acceleration;
        alignas(16) glm::vec3 buoyangcy;
    };

    class ParticalSystem3D {
    public:
        ParticalSystem3D();
        ~ParticalSystem3D();

        void SetContainerSize(glm::vec3 corner, glm::vec3 size);
        int32_t AddFluidBlock(glm::vec3 corner, glm::vec3 size, glm::vec3 v0, float particalSpace);
        uint32_t GetBlockIdByPosition(glm::vec3 position);
        void UpdateData();

        void RemoveAllFluid();

        void SetFloatingBall(glm::vec3 position, float radius);
    public:
        // ���Ӳ���
        float mSupportRadius = Para3d::supportRadius;    // ֧�Ű뾶
        float mSupportRadius2 = mSupportRadius * mSupportRadius;
        float mParticalRadius = Para3d::particalRadius;   // ���Ӱ뾶
        float mParticalDiameter = Para3d::particalDiameter;
        float mVolume = std::pow(mParticalDiameter, 3);    // ���
        float mMass = Para3d::density0 * mVolume;  // ����
        float mViscosity = Para3d::viscosity;            // ճ��ϵ��
        float mExponent = Para3d::exponent;              // ѹ��ָ��
        int mStiffness = Para3d::stiffness;            // �ն�
        std::vector<ParticalInfo3d> mParticalInfos;
        int mMaxNeighbors = 512;

        std::vector<SphereInfo> FloatingSphere;

        // ��������
        glm::vec3 mLowerBound = glm::vec3(FLT_MAX);
        glm::vec3 mUpperBound = glm::vec3(-FLT_MAX);
        glm::vec3 mContainerCenter = glm::vec3(0.0f);
        glm::uvec3 mBlockNum = glm::uvec3(0);    // XYZ���м���block
        glm::vec3 mBlockSize = glm::vec3(0.0f);
        std::vector<glm::uvec2> mBlockExtens;
        std::vector<int32_t> mBlockIdOffs;

        // �˺���
        Glb::WCubicSpline3d mW = Glb::WCubicSpline3d(mSupportRadius);

    };

}

#endif // !PARTICAL_SYSTEM_3D_H