#include "GnmBufferCache.h"

#include "GnmBuffer.h"

#include "../Violet/VltBuffer.h"
#include "../Violet/VltDevice.h"
#include "../Violet/VltContext.h"
#include "../Sce/SceGpuQueue.h"
#include "../Pssl/PsslShaderFileBinary.h"

LOG_CHANNEL(Graphic.Gnm.GnmBufferCache);

using namespace vlt;
using namespace sce;
using namespace pssl;

GnmBufferCache::GnmBufferCache(sce::SceGpuQueueDevice* device,
							   vlt::VltContext*        context,
							   GnmMemoryMonitor*       monitor) :
	m_device(device),
	m_context(context),
	m_monitor(monitor)
{
}

GnmBufferCache::~GnmBufferCache()
{
}


GnmBufferInstance* GnmBufferCache::grabBuffer(const GnmBufferCreateInfo& desc)
{
	GnmBufferInstance* buffer = nullptr;

	GnmMemoryRange range = {};
	range.start          = desc.buffer->getBaseAddress();
	range.size           = desc.buffer->getSize();

	auto iter = m_bufferMap.find(range);
	if (iter == m_bufferMap.end())
	{
		auto instance = createBuffer(desc);
		auto [iter, inserted] = m_bufferMap.emplace(std::make_pair(range, instance));
		buffer = &iter->second;
	}
	else
	{
		buffer = &iter->second;
	}

	if (buffer->memory.pendingSync())
	{
		flush(range);
	}

	return buffer;
}

void GnmBufferCache::flush(const GnmMemoryRange& range)
{
}

void GnmBufferCache::invalidate(const GnmMemoryRange& range)
{
}

GnmBufferInstance GnmBufferCache::createBuffer(const GnmBufferCreateInfo& desc)
{
	// +--------------------------------+----------------+
	// |           Gnm Buffer           | Vulkan Buffer  |
	// +--------------------------------+----------------+
	// | IndexBuffer                    | Index Buffer   |
	// | VertexBuffer                   | Vertex Buffer  |
	// | DataBuffer/RW_DataBuffer       | Storage Buffer |
	// | RegularBuffer/RW_RegularBuffer | Storage Buffer |
	// | ConstantBuffer                 | Uniform Buffer |
	// | ByteBuffer/RW_ByteBuffer       | Storage Buffer |
	// +--------------------------------+----------------+

	// Here is our buffer mapping table.
	// The table references from:
	// https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst#constanttexturestructuredbyte-buffers
	//
	// Theoretically, DataBuffer/RW_DataBuffer should be mapped to TBO (imageBuffer or samplerBuffer in GLSL),
	// and is supposed to be accessed by format conversion buffer access instructions in shader code,
	// like buffer_load_format_xxx and buffer_store_format_xxx.
	// But we can't distinguish between DataBuffer and RegularBuffer on Gnm side (see Note 1. below)
	// unless we parse the whole shader code before create buffers,
	// which will add code complexity.
	// So we use SSBO instead, which shouldn't be a matter on performance.
	//
	// While other buffers are mapped to VBO, UBO or SSBO accordingly,
	// they are supposed to be accessed by non format conversion instructions,
	// like buffer_load_xxx or buffer_atomic_xxx or etc.
	//
	// Note:
	// 1. A DataBuffer with DataFormat set to kDataFormatR32Float is the same type
	// as a RegularBuffer with stride set to 4.
	// In such case, it's legal to initialize a RegularBuffer on Gnm side then
	// declare it as a DataBuffer in shader code.
	//
	// 2. It's legal to initialize a Buffer with RW memory type (e.g. kResourceMemoryTypeGC) on Gnm side
	// while declare it as a RO Buffer in shader code, as long as the shader doesn't
	// write to the buffer.

	VkBufferUsageFlags usage  = desc.usage;
	VkAccessFlags      access = {};

	GnmMemoryRange range = {};
	range.start          = desc.buffer->getBaseAddress();
	range.size           = desc.buffer->getSize();

	// TODO:
	// Is this reliable to detect if the memory is read-write or read-only?
	ResourceMemoryType memType = desc.buffer->getResourceMemoryType();
	LOG_ASSERT(memType == kResourceMemoryTypeGC || memType == kResourceMemoryTypeRO, 
		"unsupported buffer memory type %d", memType);

	if (usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
	{
		access |= VK_ACCESS_INDEX_READ_BIT;
	}
	if (usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
	{
		access |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	}
	if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
	{
		access |= VK_ACCESS_UNIFORM_READ_BIT;
	}
	if (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
	{
		access |= VK_ACCESS_SHADER_READ_BIT;
	}
	// GPU read-write memory
	if (memType == kResourceMemoryTypeGC)
	{
		access |= VK_ACCESS_SHADER_WRITE_BIT;
	}

	VltBufferCreateInfo info = {};
	info.size                = range.size;
	info.usage               = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	info.stages              = desc.stages;
	info.access              = access;

	GnmBufferInstance buffer = {};
	buffer.buffer            = m_device->device->createBuffer(info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	buffer.idleCount         = 0;
	buffer.memory            = GnmResourceMemory(
        range,
        memType == kResourceMemoryTypeRO ? GnmMemoryProtect::GpuReadOnly : GnmMemoryProtect::GpuReadWrite);

	// We need to sync the buffer memory upon creation.
	buffer.memory.setPendingSync(true);

	return buffer;
}