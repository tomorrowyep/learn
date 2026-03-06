#ifndef __KRESOURCEPOOL_H__
#define __KRESOURCEPOOL_H__

#include "krenderresource.h"
#include "core/context/kcorecontext.h"
#include <unordered_map>
#include <vector>

namespace kEngine
{

class KResourcePool
{
public:
	KResourcePool() = default;
	~KResourcePool() = default;

	void Init(KCoreContext* ctx);
	void Destroy();

	PhysicalResource AcquireTexture(const TextureDesc& desc);
	void ReleaseTexture(PhysicalResource& resource);

	PhysicalResource AcquireBuffer(const BufferDesc& desc);
	void ReleaseBuffer(PhysicalResource& resource);

	void ResetFrame();

private:
	size_t HashTextureDesc(const TextureDesc& desc) const;
	size_t HashBufferDesc(const BufferDesc& desc) const;

	PhysicalResource CreateTexture(const TextureDesc& desc);
	PhysicalResource CreateBuffer(const BufferDesc& desc);
	void DestroyResource(PhysicalResource& resource);

private:
	KCoreContext* m_ctx = nullptr;

	struct PooledTexture
	{
		PhysicalResource resource;
		TextureDesc      desc;
		bool             inUse = false;
	};

	struct PooledBuffer
	{
		PhysicalResource resource;
		BufferDesc       desc;
		bool             inUse = false;
	};

	std::vector<PooledTexture> m_textures;
	std::vector<PooledBuffer>  m_buffers;
};

} // namespace kEngine

#endif // !__KRESOURCEPOOL_H__
