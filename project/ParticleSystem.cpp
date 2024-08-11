#include "ParticleSystem.h"
#include <iostream>
#include <algorithm>
#include "Global.h"

namespace Fluid3d {
    ParticalSystem3D::ParticalSystem3D() {

    }

    ParticalSystem3D::~ParticalSystem3D() {
    }

    void ParticalSystem3D::SetContainerSize(glm::vec3 corner, glm::vec3 size) {
        mLowerBound = corner - mSupportRadius + mParticalDiameter;
        mUpperBound = corner + size + mSupportRadius - mParticalDiameter;
        mContainerCenter = (mLowerBound + mUpperBound) / 2.0f;
        size = mUpperBound - mLowerBound;

        // ���������block����
        mBlockNum.x = floor(size.x / mSupportRadius);
        mBlockNum.y = floor(size.y / mSupportRadius);
        mBlockNum.z = floor(size.z / mSupportRadius);

        // һ��block�Ĵ�С
        mBlockSize = glm::vec3(size.x / mBlockNum.x, size.y / mBlockNum.y, size.z / mBlockNum.z);

        mBlockIdOffs.resize(27);
        int p = 0;
        for (int k = -1; k <= 1; k++) {
            for (int j = -1; j <= 1; j++) {
                for (int i = -1; i <= 1; i++) {
                    mBlockIdOffs[p] = mBlockNum.x * mBlockNum.y * k + mBlockNum.x * j + i;
                    p++;
                }
            }
        }

        mParticalInfos.clear();
    }

    int32_t ParticalSystem3D::AddFluidBlock(glm::vec3 corner, glm::vec3 size, glm::vec3 v0, float particalSpace) {
        glm::vec3 blockLowerBound = corner;
        glm::vec3 blockUpperBound = corner + size;

        if (blockLowerBound.x < mLowerBound.x ||
            blockLowerBound.y < mLowerBound.y ||
            blockLowerBound.z < mLowerBound.z ||
            blockUpperBound.x > mUpperBound.x ||
            blockUpperBound.y > mUpperBound.y ||
            blockUpperBound.z > mUpperBound.z) {
            return 0;
        }

        glm::uvec3 particalNum = glm::uvec3(size.x / particalSpace, size.y / particalSpace, size.z / particalSpace);
        std::vector<ParticalInfo3d> particals(particalNum.x * particalNum.y * particalNum.z);

        Glb::RandomGenerator rand;
        int p = 0;
        for (int idX = 0; idX < particalNum.x; idX++) {
            for (int idY = 0; idY < particalNum.y; idY++) {
                for (int idZ = 0; idZ < particalNum.z; idZ++) {
                    float x = (idX + rand.GetUniformRandom()) * particalSpace;
                    float y = (idY + rand.GetUniformRandom()) * particalSpace;
                    float z = (idZ + rand.GetUniformRandom()) * particalSpace;
                    particals[p].position = corner + glm::vec3(x, y, z);
                    particals[p].blockId = GetBlockIdByPosition(particals[p].position);
                    particals[p].velocity = v0;
                    p++;
                }
            }
        }

        mParticalInfos.insert(mParticalInfos.end(), particals.begin(), particals.end());
        return particals.size();
    }


    void ParticalSystem3D::RemoveAllFluid()
    {
        mParticalInfos = std::vector<ParticalInfo3d>();
        std::vector<ParticalInfo3d> emptyPars(1);
        mParticalInfos.insert(mParticalInfos.end(), emptyPars.begin(), emptyPars.end());
    }

    void ParticalSystem3D::SetFloatingBall(glm::vec3 position, float radius)
    {
        FloatingSphere.resize(1);
        SphereInfo Sphere;
        Sphere.acceleration = glm::vec3(0.0);
        Sphere.mass = 1.8;
        Sphere.velocity = glm::vec3(0.0);
        Sphere.position = position;
        Sphere.radius = radius;
        Sphere.buoyangcy = glm::vec3(0.0f);

        FloatingSphere[0] = Sphere;
    }


    uint32_t ParticalSystem3D::GetBlockIdByPosition(glm::vec3 position) {
        if (position.x < mLowerBound.x ||
            position.y < mLowerBound.y ||
            position.z < mLowerBound.z ||
            position.x > mUpperBound.x ||
            position.y > mUpperBound.y ||
            position.z > mUpperBound.z) {
            return -1;
        }

        glm::vec3 deltePos = position - mLowerBound;
        uint32_t c = floor(deltePos.x / mBlockSize.x);
        uint32_t r = floor(deltePos.y / mBlockSize.y);
        uint32_t h = floor(deltePos.z / mBlockSize.z);
        return h * mBlockNum.x * mBlockNum.y + r * mBlockNum.x + c;
    }

    void ParticalSystem3D::UpdateData() {
        // ��block����
        std::sort(mParticalInfos.begin(), mParticalInfos.end(),
            [=](ParticalInfo3d& first, ParticalInfo3d& second) {
                return first.blockId < second.blockId;
            }
        );

        // ����block����
        mBlockExtens = std::vector<glm::uvec2>(mBlockNum.x * mBlockNum.y * mBlockNum.z, glm::uvec2(0, 0));
        int curBlockId = 0;
        int left = 0;
        int right;
        for (right = 0; right < mParticalInfos.size(); right++) {
            if (mParticalInfos[right].blockId != curBlockId) {
                mBlockExtens[curBlockId] = glm::uvec2(left, right);        // ����ҿ�
                left = right;
                curBlockId = mParticalInfos[right].blockId;
            }
        }
        mBlockExtens[curBlockId] = glm::uvec2(left, right);
    }

    
}
