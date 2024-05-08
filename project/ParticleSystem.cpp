#include "Parameter3d.h"
#include "ParticleSystem.h"
#include "Global.h"

namespace Fluid3d
{
	ParticleSystem::ParticleSystem()
	{
	}

	ParticleSystem::~ParticleSystem()
	{
	}

	void ParticleSystem::SetContainerSize(glm::vec3 corner, glm::vec3 size)
	{
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


		//��������ƫ�������ֳ�27���飬ÿ������������
		mBlockIdOffs.resize(27);
		int p = 0;
		//k����z�����ϵ�ƫ�ƣ�j����y�����ƫ�ƣ�i����x�����ƫ��
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


	int32_t ParticleSystem::AddFluidBlock(glm::vec3 corner, glm::vec3 size, glm::vec3 v0, float particalSpace)
	{
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

		//particleSpace �����Ӽ�ĳ�ʼ��� ��ֹ���ӷ����ص�
		glm::uvec3 particalNum = glm::uvec3(size.x / particalSpace, size.y / particalSpace, size.z / particalSpace);
		std::vector<ParticleInfo3d > particals(particalNum.x * particalNum.y * particalNum.z);

		Glb::RandomGenerator rand;
		int p = 0;
		for (int idX = 0; idX < particalNum.x; idX++) {
			for (int idY = 0; idY < particalNum.y; idY++) {
				for (int idZ = 0; idZ < particalNum.z; idZ++) {
					//��������ƫ�ƣ�0��1��
					//�ƻ������ԺͶԳ���
					float x = (idX + rand.GetUniformRandom()) * particalSpace;
					float y = (idY + rand.GetUniformRandom()) * particalSpace;
					float z = (idZ + rand.GetUniformRandom()) * particalSpace;
					//position
					particals[p].position = corner + glm::vec3(x, y, z);

					//block id
					particals[p].blockId = GetBlockIdByPosition(particals[p].position);

					//initial velocity
					particals[p].velocity = v0;
					p++;
				}
			}
		}


		mParticalInfos.insert(mParticalInfos.end(), particals.begin(), particals.end());
		return particals.size();
	}


	uint32_t ParticleSystem::GetBlockIdByPosition(glm::vec3 position)
	{
		if (position.x < mLowerBound.x ||
			position.y < mLowerBound.y ||
			position.z < mLowerBound.z ||
			position.x > mUpperBound.x ||
			position.y > mUpperBound.y ||
			position.z > mUpperBound.z) {
			return -1;
		}

		glm::vec3 deltePos = position - mLowerBound;

		//x Y Z����ֱ��ڵڼ���
		uint32_t c = floor(deltePos.x / mBlockSize.x);
		uint32_t r = floor(deltePos.y / mBlockSize.y);
		uint32_t h = floor(deltePos.z / mBlockSize.z);


		return h * (mBlockNum.x * mBlockNum.y) + r * mBlockNum.x + c;
	}


	void ParticleSystem::UpdateData()
	{
		//��block����
		std::sort(mParticalInfos.begin(), mParticalInfos.end(),
			[=](ParticleInfo3d& first, ParticleInfo3d& second) {
				return first.blockId < second.blockId;
			}
		);

		//����block������index
		mBlockExtens = std::vector<glm::uvec2>(mBlockNum.x * mBlockNum.y * mBlockNum.z, glm::uvec2(0, 0));

		int curBlockId = 0;
		int left = 0;
		int right;
		for (right = 0; right < mParticalInfos.size(); right++)
		{
			if (mParticalInfos[right].blockId != curBlockId) {
				mBlockExtens[curBlockId] = glm::uvec2(left, right);        // ����ҿ�
				left = right;
				curBlockId = mParticalInfos[right].blockId;
			}
		}
		mBlockExtens[curBlockId] = glm::uvec2(left, right);
	}
}
