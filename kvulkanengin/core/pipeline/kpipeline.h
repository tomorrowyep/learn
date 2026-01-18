#ifndef __KPIPELINE_H__
#define __KPIPELINE_H__

#include <vulkan.h>
#include "pipelinecreatedesc.h"

class KPipeline
{
public:
    KPipeline() = default;
    ~KPipeline();

    KPipeline(const KPipeline&) = delete;
    KPipeline& operator=(const KPipeline&) = delete;

    bool Init(VkDevice device, const KPipelineCreateDesc& desc);
    void Destroy();

    VkPipeline GetHandle() const { return m_pipeline; }

private:
    VkDevice    m_device{ VK_NULL_HANDLE };
    VkPipeline  m_pipeline{ VK_NULL_HANDLE };
};

#endif
