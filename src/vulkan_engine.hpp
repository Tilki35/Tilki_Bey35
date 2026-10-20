#pragma once
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include "shaders.hpp"
#include "options.hpp"
#include "types.hpp"
#include "adaptive.hpp"
#include "pacing.hpp"

#define VK_FUNCTIONS(X) \
 X(vkCreateInstance) X(vkDestroyInstance) X(vkEnumeratePhysicalDevices) \
 X(vkGetPhysicalDeviceProperties) X(vkGetPhysicalDeviceQueueFamilyProperties) \
 X(vkGetPhysicalDeviceMemoryProperties) X(vkCreateDevice) X(vkDestroyDevice) \
 X(vkEnumerateInstanceExtensionProperties) X(vkEnumerateDeviceExtensionProperties) \
 X(vkGetDeviceQueue) X(vkCreateBuffer) X(vkDestroyBuffer) X(vkGetBufferMemoryRequirements) \
 X(vkAllocateMemory) X(vkFreeMemory) X(vkBindBufferMemory) X(vkMapMemory) X(vkUnmapMemory) \
 X(vkCreateDescriptorSetLayout) X(vkDestroyDescriptorSetLayout) \
 X(vkCreatePipelineLayout) X(vkDestroyPipelineLayout) X(vkCreateShaderModule) \
 X(vkDestroyShaderModule) X(vkCreateComputePipelines) X(vkDestroyPipeline) \
 X(vkCreateDescriptorPool) X(vkDestroyDescriptorPool) X(vkAllocateDescriptorSets) \
 X(vkUpdateDescriptorSets) X(vkCreateCommandPool) X(vkDestroyCommandPool) \
 X(vkAllocateCommandBuffers) X(vkBeginCommandBuffer) X(vkEndCommandBuffer) \
 X(vkCmdBindPipeline) X(vkCmdBindDescriptorSets) X(vkCmdPushConstants) \
 X(vkCmdDispatch) X(vkCmdPipelineBarrier) X(vkCreateFence) X(vkDestroyFence) \
 X(vkQueueSubmit) X(vkWaitForFences) X(vkDeviceWaitIdle) X(vkResetCommandBuffer) X(vkResetFences) \
 X(vkCreateQueryPool) X(vkDestroyQueryPool) X(vkCmdResetQueryPool) X(vkCmdWriteTimestamp) X(vkGetQueryPoolResults)
#define DECLARE(name) PFN_##name name=nullptr;
VK_FUNCTIONS(DECLARE)
#undef DECLARE
void check(VkResult r) { if(r!=VK_SUCCESS) throw std::runtime_error("errors.vulkan_error|"+std::to_string(r)); }
struct Buffer { VkBuffer handle{}; VkDeviceMemory memory{}; void* mapped{}; };
struct Engine {
    VkInstance instance{}; VkDevice device{}; VkQueue queue{};
    VkDescriptorSetLayout descriptorLayout{}; VkPipelineLayout layout{};
    VkPipeline flowPipeline{},refinePipeline{},interpolatePipeline{}; VkDescriptorPool descriptors{};
    VkCommandPool commands{}; std::array<Buffer,5> buffers{};
    VkDescriptorSet descriptorSet{}; VkCommandBuffer command{}; VkFence fence{};
    VkQueryPool timestamps{}; uint32_t timestampBits=0; float timestampPeriod=0;
    double gpuTimestampMs=-1;
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    std::string deviceName; bool software=false;
#ifdef _WIN32
    HMODULE library{};
#else
    void* library{};
#endif
    Engine()=default;
    Engine(const Engine&)=delete;
    ~Engine() {
        if(device) {
            vkDeviceWaitIdle(device);
            if(timestamps) vkDestroyQueryPool(device,timestamps,nullptr);
            if(fence) vkDestroyFence(device,fence,nullptr);
            if(commands) vkDestroyCommandPool(device,commands,nullptr);
            if(descriptors) vkDestroyDescriptorPool(device,descriptors,nullptr);
            if(flowPipeline) vkDestroyPipeline(device,flowPipeline,nullptr);
            if(refinePipeline) vkDestroyPipeline(device,refinePipeline,nullptr);
            if(interpolatePipeline) vkDestroyPipeline(device,interpolatePipeline,nullptr);
            if(layout) vkDestroyPipelineLayout(device,layout,nullptr);
            if(descriptorLayout) vkDestroyDescriptorSetLayout(device,descriptorLayout,nullptr);
            for(auto& b:buffers) {
                if(b.mapped) vkUnmapMemory(device,b.memory);
                if(b.handle) vkDestroyBuffer(device,b.handle,nullptr);
                if(b.memory) vkFreeMemory(device,b.memory,nullptr);
            }
            vkDestroyDevice(device,nullptr);
        }
        if(instance) vkDestroyInstance(instance,nullptr);
#ifdef _WIN32
        if(library) FreeLibrary(library);
#else
        if(library) dlclose(library);
#endif
    }
    void init(const std::string& applicationName="XFrame") {
#ifdef _WIN32
        auto loader=library=LoadLibraryA("vulkan-1.dll");
        if(!loader) throw std::runtime_error("errors.vulkan_loader_missing_install_the_official_gpu_driver");
#define LOAD(name) { auto proc=GetProcAddress(loader,#name); static_assert(sizeof(proc)==sizeof(name)); std::memcpy(&name,&proc,sizeof(name)); if(!name) throw std::runtime_error(std::string("errors.vulkan_function|")+#name); }
#else
#ifdef __APPLE__
        auto loader=library=dlopen("libvulkan.1.dylib",RTLD_NOW|RTLD_LOCAL);
#else
        auto loader=library=dlopen("libvulkan.so.1",RTLD_NOW|RTLD_LOCAL);
#endif
        if(!loader) throw std::runtime_error("errors.vulkan_loader_missing");
#define LOAD(name) name=reinterpret_cast<PFN_##name>(dlsym(loader,#name)); if(!name) throw std::runtime_error(std::string("errors.vulkan_function|")+#name);
#endif
        VK_FUNCTIONS(LOAD)
#undef LOAD
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app.pApplicationName=applicationName.c_str(); app.apiVersion=VK_API_VERSION_1_1;
        VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO}; ici.pApplicationInfo=&app;
        uint32_t extensionCount=0; check(vkEnumerateInstanceExtensionProperties(nullptr,&extensionCount,nullptr));
        std::vector<VkExtensionProperties> extensions(extensionCount);
        check(vkEnumerateInstanceExtensionProperties(nullptr,&extensionCount,extensions.data()));
        const char* portability=VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
        for(const auto& e:extensions) if(std::strcmp(e.extensionName,portability)==0) {
            ici.flags|=VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR; ici.enabledExtensionCount=1; ici.ppEnabledExtensionNames=&portability;break;
        }
        check(vkCreateInstance(&ici,nullptr,&instance));
        uint32_t count=0; check(vkEnumeratePhysicalDevices(instance,&count,nullptr));
        if(!count) throw std::runtime_error("errors.no_vulkan_device");
        std::vector<VkPhysicalDevice> devices(count); check(vkEnumeratePhysicalDevices(instance,&count,devices.data()));
        VkPhysicalDevice selected{}; uint32_t family=0; int best=-1;
        for(auto pd:devices) {
            VkPhysicalDeviceProperties prop{}; vkGetPhysicalDeviceProperties(pd,&prop);
            if(prop.apiVersion<VK_API_VERSION_1_1) continue;
            uint32_t n=0; vkGetPhysicalDeviceQueueFamilyProperties(pd,&n,nullptr);
            std::vector<VkQueueFamilyProperties> qs(n); vkGetPhysicalDeviceQueueFamilyProperties(pd,&n,qs.data());
            for(uint32_t i=0;i<n;i++) if(qs[i].queueCount && (qs[i].queueFlags&VK_QUEUE_COMPUTE_BIT)) {
                int score=prop.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 3 : prop.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU ? 0 : 1;
                if(score>best) {
                    best=score; selected=pd; family=i; deviceName=prop.deviceName; software=prop.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU;
                    timestampBits=qs[i].timestampValidBits; timestampPeriod=prop.limits.timestampPeriod;
                }
                break;
            }
        }
        if(!selected) throw std::runtime_error("errors.no_compute_queue");
        vkGetPhysicalDeviceMemoryProperties(selected,&memoryProperties);
        float priority=1;
        VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        qci.queueFamilyIndex=family; qci.queueCount=1; qci.pQueuePriorities=&priority;
        VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO}; dci.queueCreateInfoCount=1; dci.pQueueCreateInfos=&qci;
        extensionCount=0;check(vkEnumerateDeviceExtensionProperties(selected,nullptr,&extensionCount,nullptr));
        extensions.resize(extensionCount);check(vkEnumerateDeviceExtensionProperties(selected,nullptr,&extensionCount,extensions.data()));
        const char* subset="VK_KHR_portability_subset";
        for(const auto& e:extensions) if(std::strcmp(e.extensionName,subset)==0) {dci.enabledExtensionCount=1;dci.ppEnabledExtensionNames=&subset;break;}
        check(vkCreateDevice(selected,&dci,nullptr,&device)); vkGetDeviceQueue(device,family,0,&queue);
        for(auto& b:buffers) {
            VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            ci.size=W*H*sizeof(Pixel); ci.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            check(vkCreateBuffer(device,&ci,nullptr,&b.handle));
            VkMemoryRequirements req{}; vkGetBufferMemoryRequirements(device,b.handle,&req);
            uint32_t index=UINT32_MAX;
            for(uint32_t i=0;i<memoryProperties.memoryTypeCount;i++)
                if((req.memoryTypeBits&(1u<<i)) && (memoryProperties.memoryTypes[i].propertyFlags &
                   (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
                   (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) { index=i; break; }
            if(index==UINT32_MAX) throw std::runtime_error("errors.no_host_coherent_buffer_memory_staging_path_not_implemented");
            VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; ai.allocationSize=req.size; ai.memoryTypeIndex=index;
            check(vkAllocateMemory(device,&ai,nullptr,&b.memory)); check(vkBindBufferMemory(device,b.handle,b.memory,0));
            check(vkMapMemory(device,b.memory,0,VK_WHOLE_SIZE,0,&b.mapped));
        }
        std::array<VkDescriptorSetLayoutBinding,5> bindings{};
        for(uint32_t i=0;i<5;i++) bindings[i]={i,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};
        VkDescriptorSetLayoutCreateInfo sl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; sl.bindingCount=5; sl.pBindings=bindings.data();
        check(vkCreateDescriptorSetLayout(device,&sl,nullptr,&descriptorLayout));
        VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT,0,20};
        VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pl.setLayoutCount=1; pl.pSetLayouts=&descriptorLayout; pl.pushConstantRangeCount=1; pl.pPushConstantRanges=&push;
        check(vkCreatePipelineLayout(device,&pl,nullptr,&layout));
        flowPipeline=pipeline(flowSpv,sizeof(flowSpv)); refinePipeline=pipeline(refineSpv,sizeof(refineSpv));
        interpolatePipeline=pipeline(interpolateSpv,sizeof(interpolateSpv));
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,5};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO}; pool.maxSets=1; pool.poolSizeCount=1; pool.pPoolSizes=&size;
        check(vkCreateDescriptorPool(device,&pool,nullptr,&descriptors));
        VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO}; cp.queueFamilyIndex=family; cp.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        check(vkCreateCommandPool(device,&cp,nullptr,&commands));
        if(timestampBits && timestampPeriod>0) {
            VkQueryPoolCreateInfo query{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO}; query.queryType=VK_QUERY_TYPE_TIMESTAMP; query.queryCount=2;
            check(vkCreateQueryPool(device,&query,nullptr,&timestamps));
        }
    }
    VkPipeline pipeline(const uint32_t* words,size_t bytes) {
        VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO}; sm.codeSize=bytes; sm.pCode=words;
        VkShaderModule module{}; check(vkCreateShaderModule(device,&sm,nullptr,&module));
        VkComputePipelineCreateInfo ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        ci.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ci.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT; ci.stage.module=module; ci.stage.pName="main"; ci.layout=layout;
        VkPipeline result{}; auto status=vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&ci,nullptr,&result);
        vkDestroyShaderModule(device,module,nullptr); check(status); return result;
    }
    double run(const std::vector<Pixel>& a,const std::vector<Pixel>& b,std::vector<Pixel>& output,int flags=ARTIFACT_REDUCTION,float alpha=.5f,int radius=12,bool reuseFlow=false) {
        // Reuse device, buffers, descriptor set, command buffer and fence across pairs.
        if(!reuseFlow) {
            std::memcpy(buffers[0].mapped,a.data(),a.size()*sizeof(Pixel));
            std::memcpy(buffers[1].mapped,b.data(),b.size()*sizeof(Pixel));
        }
        VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO}; da.descriptorPool=descriptors;
        da.descriptorSetCount=1; da.pSetLayouts=&descriptorLayout;
        if(!descriptorSet) {
            check(vkAllocateDescriptorSets(device,&da,&descriptorSet));
            std::array<VkDescriptorBufferInfo,5> infos{}; std::array<VkWriteDescriptorSet,5> writes{};
            for(uint32_t i=0;i<5;i++) {
                infos[i]={buffers[i].handle,0,VK_WHOLE_SIZE}; writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[i].dstSet=descriptorSet; writes[i].dstBinding=i; writes[i].descriptorCount=1;
                writes[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; writes[i].pBufferInfo=&infos[i];
            }
            vkUpdateDescriptorSets(device,5,writes.data(),0,nullptr);
        }
        VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ca.commandPool=commands; ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY; ca.commandBufferCount=1;
        if(!command) check(vkAllocateCommandBuffers(device,&ca,&command));
        else check(vkResetCommandBuffer(command,0));
        VkCommandBuffer cmd=command;
        if(!reuseFlow) {
            std::memset(buffers[2].mapped,0,W*H*sizeof(Pixel));
            std::memset(buffers[4].mapped,0,W*H*sizeof(Pixel));
        }
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO}; begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(cmd,&begin));
        if(timestamps) { vkCmdResetQueryPool(cmd,timestamps,0,2); vkCmdWriteTimestamp(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,timestamps,0); }
        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT; barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&barrier,0,nullptr,0,nullptr);
        vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&descriptorSet,0,nullptr);
        struct Params { int32_t width,height,radius,flags; float alpha; } params{W,H,radius,flags,alpha};
        static_assert(sizeof(Params)==20);
        vkCmdPushConstants(cmd,layout,VK_SHADER_STAGE_COMPUTE_BIT,0,20,&params);
        if(needsFlow(flags) && !reuseFlow) {
            vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,flowPipeline);
            vkCmdDispatch(cmd,((W+7)/8+7)/8,((H+7)/8+7)/8,1);
        }
        barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT; barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&barrier,0,nullptr,0,nullptr);
        if(needsFlow(flags) && (flags&DENSE_FLOW) && !reuseFlow) {
            vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,refinePipeline);
            vkCmdDispatch(cmd,(W+7)/8,(H+7)/8,1);
            vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&barrier,0,nullptr,0,nullptr);
        }
        vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,interpolatePipeline);
        vkCmdDispatch(cmd,(W+7)/8,(H+7)/8,1);
        barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT; barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,nullptr,0,nullptr);
        if(timestamps) vkCmdWriteTimestamp(cmd,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,timestamps,1);
        check(vkEndCommandBuffer(cmd));
        VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        if(!fence) check(vkCreateFence(device,&fc,nullptr,&fence)); else check(vkResetFences(device,1,&fence));
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO}; submit.commandBufferCount=1; submit.pCommandBuffers=&cmd;
        auto start=std::chrono::steady_clock::now();
        auto status=vkQueueSubmit(queue,1,&submit,fence);
        if(status==VK_SUCCESS) status=vkWaitForFences(device,1,&fence,VK_TRUE,60'000'000'000ull);
        if(status!=VK_SUCCESS) vkDeviceWaitIdle(device);
        check(status);
        double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        gpuTimestampMs=-1;
        if(timestamps) {
            uint64_t ticks[2]{};
            check(vkGetQueryPoolResults(device,timestamps,0,2,sizeof(ticks),ticks,sizeof(uint64_t),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WAIT_BIT));
            uint64_t delta=ticks[1]-ticks[0];
            if(timestampBits<64) delta&=(uint64_t(1)<<timestampBits)-1;
            gpuTimestampMs=double(delta)*timestampPeriod/1e6;
        }
        output.resize(W*H); std::memcpy(output.data(),buffers[3].mapped,output.size()*sizeof(Pixel)); return ms;
    }
};
