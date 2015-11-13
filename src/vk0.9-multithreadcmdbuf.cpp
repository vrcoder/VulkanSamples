/*
 * Vulkan Samples Kit
 *
 * Copyright (C) 2015 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
VULKAN_SAMPLE_SHORT_DESCRIPTION
Use per-thread command buffers to draw 3 triangles
*/

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>

#define DEPTH_PRESENT false
struct Vertex
{
    float posX, posY, posZ, posW;    // Position data
    float r, g, b, a;                // Color
};
#define XYZ1(_x_, _y_, _z_)         (_x_), (_y_), (_z_), 1.f
static const Vertex triData[] =
{
   { XYZ1( -0.25, -0.25, 0 ), XYZ1( 1.f, 0.f, 0.f ) },
   { XYZ1(  0.25, -0.25, 0 ), XYZ1( 1.f, 0.f, 0.f ) },
   { XYZ1(  0,  0.25, 0 ), XYZ1( 1.f, 0.f, 0.f ) },
   { XYZ1( -0.75, -0.25, 0 ), XYZ1( 1.f, 0.f, 0.f ) },
   { XYZ1( -0.25, -0.25, 0 ), XYZ1( 1.f, 0.f, 0.f ) },
   { XYZ1(  -0.5,  0.25, 0 ), XYZ1( 1.f, 0.f, 0.f ) },
   { XYZ1(  0.25, -0.25, 0 ), XYZ1( 1.f, 0.f, 0.f ) },
   { XYZ1(  0.75, -0.25, 0 ), XYZ1( 1.f, 0.f, 0.f ) },
   { XYZ1(  0.5,  0.25, 0 ), XYZ1( 1.f, 0.f, 0.f ) },
};

struct {
        VkBuffer buf;
        VkDeviceMemory mem;
        VkDescriptorBufferInfo buffer_info;
    } vertex_buffer[3];


/* For this sample, we'll start with GLSL so the shader function is plain */
/* and then use the glslang GLSLtoSPV utility to convert it to SPIR-V for */
/* the driver.  We do this for clarity rather than using pre-compiled     */
/* SPIR-V                                                                 */

static const char *vertShaderText =
    "#version 140\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n"
    "layout (location = 0) in vec4 pos;\n"
    "layout (location = 1) in vec4 inColor;\n"
    "layout (location = 0) out vec4 outColor;\n"
    "void main() {\n"
    "    outColor = inColor;\n"
    "    gl_Position = pos;\n"
    "}\n";

static const char *fragShaderText=
    "#version 140\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n"
    "layout (location = 0) in vec4 color;\n"
    "layout (location = 0) out vec4 outColor;\n"
    "void main() {\n"
    "   outColor = color;\n"
    "}\n";

int main(int argc, char **argv)
{
    VkResult U_ASSERT_ONLY res;
    struct sample_info info = {};
    char sample_title[] = "MT Cmd Buffer Sample";

    init_global_layer_properties(info);
    info.instance_extension_names.push_back(VK_EXT_KHR_SWAPCHAIN_EXTENSION_NAME);
    info.device_extension_names.push_back(VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_device(info);
    info.width = info.height = 500;
    init_connection(info);
    init_window(info);
    init_swapchain_extension(info);
    init_command_pool(info);
    init_command_buffer(info);
    execute_begin_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext                  = NULL;
    pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pPipelineLayoutCreateInfo.pPushConstantRanges    = NULL;
    pPipelineLayoutCreateInfo.setLayoutCount         = 0;
    pPipelineLayoutCreateInfo.pSetLayouts            = NULL;

    res = vkCreatePipelineLayout(info.device,
                                 &pPipelineLayoutCreateInfo, NULL,
                                 &info.pipeline_layout);
    assert(res == VK_SUCCESS);
    init_renderpass(info, DEPTH_PRESENT);
    init_shaders(info, vertShaderText, fragShaderText);
    init_framebuffers(info, DEPTH_PRESENT);

    /* The binding and attributes should be the same for all 3 vertex buffers, so init here */
    info.vi_binding.binding = 0;
    info.vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    info.vi_binding.stride = sizeof(triData[0]);

    info.vi_attribs[0].binding = 0;
    info.vi_attribs[0].location = 0;
    info.vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[0].offset = 0;
    info.vi_attribs[1].binding = 0;
    info.vi_attribs[1].location = 1;
    info.vi_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[1].offset = 16;

    init_pipeline_cache(info);
    init_pipeline(info, DEPTH_PRESENT);
    res = vkEndCommandBuffer(info.cmd);
    const VkCommandBuffer cmd_bufs[] = { info.cmd };

    VkFence nullFence = VK_NULL_HANDLE;

    VkSubmitInfo submit_info[1] = {};
    submit_info[0].waitSemaphoreCount = 0;
    submit_info[0].pWaitSemaphores = NULL;
    submit_info[0].commandBufferCount = 1;
    submit_info[0].pCommandBuffers = cmd_bufs;
    submit_info[0].signalSemaphoreCount = 0;
    submit_info[0].pSignalSemaphores = NULL;

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.queue, 1, submit_info, nullFence);
    assert(!res);

    res = vkQueueWaitIdle(info.queue);
    assert(!res);

    VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo;
    presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    presentCompleteSemaphoreCreateInfo.pNext = NULL;
    presentCompleteSemaphoreCreateInfo.flags = 0;

    res = vkCreateSemaphore(info.device,
                            &presentCompleteSemaphoreCreateInfo,
                            NULL,
                            &info.presentCompleteSemaphore);
    assert(res == VK_SUCCESS);

    // Get the index of the next available swapchain image:
    res = info.fpAcquireNextImageKHR(info.device, info.swap_chain,
                                      UINT64_MAX,
                                      info.presentCompleteSemaphore,
                                      &info.current_buffer);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
    // return codes
    assert(res == VK_SUCCESS);

    /* VULKAN_KEY_START */

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = NULL;
    buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buf_info.size = 3*sizeof(triData[0]);
    buf_info.queueFamilyIndexCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    res = vkCreateBuffer(info.device, &buf_info, NULL, &vertex_buffer[0].buf);
    assert(res == VK_SUCCESS);

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(info.device, vertex_buffer[0].buf, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    alloc_info.allocationSize = mem_reqs.size;
    bool pass;
    pass = memory_type_from_properties(info,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      &alloc_info.memoryTypeIndex);
    assert(pass);

    res = vkAllocateMemory(info.device, &alloc_info, NULL, &(vertex_buffer[0].mem));
    assert(res == VK_SUCCESS);

    uint8_t *pData;
    res = vkMapMemory(info.device, vertex_buffer[0].mem, 0, 0, 0, (void **) &pData);
    assert(res == VK_SUCCESS);

    memcpy(pData, triData, 3*sizeof(triData[0]));

    vkUnmapMemory(info.device, vertex_buffer[0].mem);

    res = vkBindBufferMemory(info.device,
            vertex_buffer[0].buf,
            vertex_buffer[0].mem, 0);
    assert(res == VK_SUCCESS);

    VkCommandBuffer threadCmdBufs[3];
    VkCommandBufferAllocateInfo cmd = {};
    cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd.pNext = NULL;
    cmd.commandPool = info.cmd_pool;
    cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd.bufferCount = 3;

    res = vkAllocateCommandBuffers(info.device, &cmd, threadCmdBufs);
    assert(res == VK_SUCCESS);

    VkCommandBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.renderPass = VK_NULL_HANDLE;  /* May only set renderPass and framebuffer */
    cmd_buf_info.framebuffer = VK_NULL_HANDLE; /* for secondary command buffers           */
    cmd_buf_info.flags = 0;
    cmd_buf_info.subpass = 0;
    cmd_buf_info.occlusionQueryEnable = VK_FALSE;
    cmd_buf_info.queryFlags = 0;
    cmd_buf_info.pipelineStatistics = 0;
    res = vkBeginCommandBuffer(threadCmdBufs[0], &cmd_buf_info);
    assert(res == VK_SUCCESS);

    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.2f;
    clear_values[0].color.float32[1] = 0.2f;
    clear_values[0].color.float32[2] = 0.2f;
    clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil.depth     = 1.0f;
    clear_values[1].depthStencil.stencil   = 0;

    VkRenderPassBeginInfo rp_begin;
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = info.render_pass;
    rp_begin.framebuffer = info.framebuffers[info.current_buffer];
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = info.width;
    rp_begin.renderArea.extent.height = info.height;
    rp_begin.clearValueCount = 2;
    rp_begin.pClearValues = clear_values;

    vkCmdBeginRenderPass(threadCmdBufs[0], &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(threadCmdBufs[0], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  info.pipeline);
    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(threadCmdBufs[0], 0, 1, &vertex_buffer[0].buf, offsets);

    VkViewport viewport;
    viewport.height = (float) info.height;
    viewport.width = (float) info.width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    viewport.x = 0;
    viewport.y = 0;
    vkCmdSetViewport(threadCmdBufs[0], NUM_VIEWPORTS, &viewport);

    VkRect2D scissor;
    scissor.extent.width = info.width;
    scissor.extent.height = info.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(threadCmdBufs[0], NUM_SCISSORS, &scissor);

    vkCmdDraw(threadCmdBufs[0], 3, 1, 0, 0);
    vkCmdEndRenderPass(threadCmdBufs[0]);

    VkImageMemoryBarrier prePresentBarrier = {};
    prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    prePresentBarrier.pNext = NULL;
    prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    prePresentBarrier.dstAccessMask = 0;
    prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SOURCE_KHR;
    prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    prePresentBarrier.subresourceRange.baseMipLevel = 0;
    prePresentBarrier.subresourceRange.levelCount = 1;
    prePresentBarrier.subresourceRange.baseArrayLayer = 0;
    prePresentBarrier.subresourceRange.layerCount = 1;
    prePresentBarrier.image = info.buffers[info.current_buffer].image;
    VkImageMemoryBarrier *pmemory_barrier = &prePresentBarrier;
    vkCmdPipelineBarrier(threadCmdBufs[0], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         0, 1, (const void * const*)&pmemory_barrier);

    res = vkEndCommandBuffer(threadCmdBufs[0]);
    const VkCommandBuffer thread_cmd_bufs[] = { threadCmdBufs[0] };

    execute_queue_cmdbuf(info, thread_cmd_bufs);
    execute_present_image(info);

    wait_seconds(1);
    /* VULKAN_KEY_END */

    vkFreeCommandBuffers(info.device, info.cmd_pool, 3, threadCmdBufs);
    vkDestroyBuffer(info.device, vertex_buffer[0].buf, NULL);
    vkFreeMemory(info.device, vertex_buffer[0].mem, NULL);
    vkDestroySemaphore(info.device, info.presentCompleteSemaphore, NULL);
    destroy_pipeline(info);
    destroy_pipeline_cache(info);
    destroy_framebuffers(info);
    destroy_shaders(info);
    destroy_renderpass(info);
    vkDestroyPipelineLayout(info.device, info.pipeline_layout, NULL);
    destroy_swap_chain(info);
    destroy_command_buffer(info);
    destroy_command_pool(info);
    destroy_window(info);
    destroy_device(info);
    destroy_instance(info);
}