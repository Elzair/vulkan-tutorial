#include <stdlib.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <functional>
#include "common.hpp"
#include "queue.hpp"
#include "swapchain.hpp"
#include "shader.hpp"
#include "buffer.hpp"

const std::vector<Vertex> vertices = {
    {{ 0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}
};

class HelloTriangleApplication
{
public:
  void run()
  {
    initWindow();
    initVulkan();
    mainLoop();
  }

private:

  GLFWwindow*                          window;
  VDeleter<VkInstance>                 instance { vkDestroyInstance };
  VDeleter<VkDebugReportCallbackEXT>   callback { this->instance, DestroyDebugReportCallbackEXT };
  VDeleter<VkSurfaceKHR>               surface  { this->instance, vkDestroySurfaceKHR };
  VkPhysicalDevice                     physical = VK_NULL_HANDLE;
  VDeleter<VkDevice>                   device   { vkDestroyDevice };
  int                                  graphicsQueueIdx;
  int                                  presentQueueIdx;
  VkQueue                              graphicsQueue;
  VkQueue                              presentQueue;
  VDeleter<VkSwapchainKHR>             swapchain { this->device, vkDestroySwapchainKHR };
  std::vector<VkImage>                 swapchainImages;
  VkFormat                             swapchainImageFormat;
  VkExtent2D                           swapchainExtent;
  std::vector<VDeleter<VkImageView>>   swapchainImageViews;
  VDeleter<VkRenderPass>               renderPass       { this->device, vkDestroyRenderPass };
  VDeleter<VkPipelineLayout>           pipelineLayout   { this->device, vkDestroyPipelineLayout };
  VDeleter<VkPipeline>                 graphicsPipeline { this->device, vkDestroyPipeline };
  std::vector<VDeleter<VkFramebuffer>> swapchainFramebuffers;
  VDeleter<VkCommandPool>              commandPool         { this->device, vkDestroyCommandPool };
  VDeleter<VkBuffer>                   vertexBuffer        { this->device, vkDestroyBuffer };
  VDeleter<VkDeviceMemory>             vertexBufferMemory { this->device, vkFreeMemory };
  std::vector<VkCommandBuffer>         commandBuffers;
  VDeleter<VkSemaphore>                imageAvailableSemaphore { this->device, vkDestroySemaphore };
  VDeleter<VkSemaphore>                renderFinishSemaphore   { this->device, vkDestroySemaphore };
  
  void initWindow()
  {
    glfwInit();
    
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

    
    this->window = glfwCreateWindow( WIDTH, HEIGHT, "Vulkan", nullptr, nullptr );

    glfwSetWindowUserPointer( this->window, this );
    glfwSetWindowSizeCallback( this->window, HelloTriangleApplication::onWindowResized );
  }

  static void onWindowResized( GLFWwindow* window, int width, int height )
  {
    if ( width == 0 || height == 0 )
    {
      return;
    }

    HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>( glfwGetWindowUserPointer( window ) );
    app->recreateSwapChain();
  }
    
  void initVulkan()
  {
    this->createInstance();
    this->createDebugCallback();
    this->createSurface();
    this->pickPhysicalDevice();
    this->createLogicalDevice();
    this->createSwapChain();
    this->createImageViews();
    this->createRenderPass();
    this->createGraphicsPipeline();
    this->createFramebuffers();
    this->createCommandPool();
    this->createVertexBuffer();
    this->createCommandBuffers();
    this->createSemaphores();
  }

  void mainLoop()
  {
    while ( !glfwWindowShouldClose( this->window ) )
    {
      glfwPollEvents();
      this->drawFrame();
    }

    // Wait for logical device to finish
    vkDeviceWaitIdle( this->device );
  }

  void recreateSwapChain(  )
  {
    vkDeviceWaitIdle( this->device );

    this->createSwapChain();
    this->createImageViews();
    this->createRenderPass();
    this->createGraphicsPipeline();
    this->createFramebuffers();
    this->createCommandBuffers();
  }

  void drawFrame(  )
  {
    uint32_t imageIdx;
    auto result = vkAcquireNextImageKHR( this->device,
                                         this->swapchain,
                                         std::numeric_limits<uint64_t>::max(), // Disable timeout for image to become available
                                         this->imageAvailableSemaphore,
                                         VK_NULL_HANDLE,
                                         &imageIdx );
    if ( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR )
    {
      this->recreateSwapChain();
      return;
    }
    else if ( result != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to present swap chain image!" );
    }

    // Submit command buffer
    VkSemaphore waitSemaphores[]      = { this->imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[]    = { this->renderFinishSemaphore };
    

    VkSubmitInfo submitInfo = {};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &this->commandBuffers[imageIdx];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if ( vkQueueSubmit( this->graphicsQueue,
                        1,
                        &submitInfo,
                        VK_NULL_HANDLE ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to submit draw command buffer!" );
    }

    // Submit result to swap chain
    VkSwapchainKHR swapchains[] = { this->swapchain };

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = swapchains;
    presentInfo.pImageIndices      = &imageIdx;
    presentInfo.pResults           = nullptr;

    vkQueuePresentKHR( this->presentQueue, &presentInfo );
  }

  void createInstance(  )
  {
    // Ensure validation layers are available
    if ( enableValidationLayers && !this->checkValidationLayerSupport() )
    {
      throw std::runtime_error( "Validation layers requested but are not available!" );
    }
    
    VkApplicationInfo appInfo  = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName        = "No Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instCreateInfo    = {};
    instCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instCreateInfo.pApplicationInfo        = &appInfo;
    auto vkextensions                        = this->getRequiredExtensions();
    instCreateInfo.enabledExtensionCount   = vkextensions.size();
    instCreateInfo.ppEnabledExtensionNames = vkextensions.data();
    if ( enableValidationLayers )
    {
      instCreateInfo.enabledLayerCount   = validationLayers.size(  );
      instCreateInfo.ppEnabledLayerNames = validationLayers.data(  );
    }
    else
    {
      instCreateInfo.enabledLayerCount = 0;
    }

    if ( vkCreateInstance( &instCreateInfo, nullptr, &this->instance ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create instance!" );
    }
    else
    {
      std::cout << "Created instance!" << std::endl;
    }

    // Get information on extension properties
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );
    std::vector<VkExtensionProperties> extensions( extensionCount );
    vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, extensions.data(  ) );

    std::cout << "Available Extensions:" << std::endl;
    for ( const auto& extension: extensions )
    {
      std::cout << "\t" << extension.extensionName << std::endl;
    }
  }

  void createSurface(  )
  {
    if ( glfwCreateWindowSurface( this->instance, this->window, nullptr, &this->surface ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create window surface!" );
    }
  }

  void pickPhysicalDevice(  )
  {
    uint32_t devCount = 0;
    vkEnumeratePhysicalDevices( this->instance, &devCount, nullptr);
    if ( devCount == 0 )
    {
      throw std::runtime_error( "Failed to find GPUs with Vulkan support!" );
    }
    std::vector<VkPhysicalDevice> devices( devCount );
    vkEnumeratePhysicalDevices( this->instance, &devCount, devices.data() );

    for ( const auto& device : devices )
    {
      if ( this->isDeviceSuitable( device ) )
      {
        this->physical = device;
        break;
      }
    }

    if ( this->physical == VK_NULL_HANDLE )
    {
      throw std::runtime_error( "Failed to find a suitable GPU!" );
    }
  }

  void createLogicalDevice(  )
  {
    // Create queues for both the graphics and presentation families
    QueueFamilyIndices indices = findQueueFamilies( this->physical, this->surface );
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = {
      indices.graphicsFamily,
      indices.presentFamily
    };
    float queuePriority = 1.0f;

    for ( int queueFamily : uniqueQueueFamilies )
    {
      VkDeviceQueueCreateInfo queueCreateInfo = {};
      queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount       = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back( queueCreateInfo );
    }

    VkPhysicalDeviceFeatures devFeatures = {};

    // Create struct used to create a logical device
    VkDeviceCreateInfo devCreateInfo = {};
    devCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
    devCreateInfo.queueCreateInfoCount    = (uint32_t)queueCreateInfos.size();
    devCreateInfo.pEnabledFeatures        = &devFeatures;
    devCreateInfo.enabledExtensionCount   = requiredDeviceExtensions.size();
    devCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
    if ( enableValidationLayers )
    {
      devCreateInfo.enabledLayerCount   = validationLayers.size(  );
      devCreateInfo.ppEnabledLayerNames = validationLayers.data(  );
    }
    else
    {
      devCreateInfo.enabledLayerCount = 0;
    }

    if ( vkCreateDevice( this->physical, &devCreateInfo, nullptr, &this->device ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create logical device!" );
    }

    // Retrieve handles for graphics and presentation queues
    vkGetDeviceQueue( this->device, indices.graphicsFamily, 0, &this->graphicsQueue);
    vkGetDeviceQueue( this->device, indices.presentFamily,  0, &this->presentQueue );
  }

  void createSwapChain( )
  {
    auto swapchainSupport = querySwapChainSupport( this->physical, this->surface );
    auto surfaceFormat    = chooseSwapChainSurfaceFormat( swapchainSupport.formats );
    auto presentMode      = chooseSwapChainPresentMode( swapchainSupport.presentModes );
    auto extent            = chooseSwapChainExtent( swapchainSupport.capabilities,
                                                    WIDTH,
                                                    HEIGHT );

    this->swapchainImageFormat = surfaceFormat.format;
    this->swapchainExtent       = extent;

    // Ensure we have enough images to properly implement triple buffering
    // A value of 0 for maxImageCount means there is no hard limit on # images
    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if ( swapchainSupport.capabilities.maxImageCount > 0 &&
         imageCount > swapchainSupport.capabilities.maxImageCount )
    {
      imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    // Get indices of graphics and present queues
    auto indices = findQueueFamilies( this->physical, this->surface );
    this->graphicsQueueIdx = indices.graphicsFamily;
    this->presentQueueIdx  = indices.presentFamily;
    uint32_t familyIndices[] = {
      (uint32_t)indices.graphicsFamily,
      (uint32_t)indices.presentFamily
    };

    // Create struct used to create a swapchain
    VkSwapchainKHR oldSwapchain = this->swapchain;
    
    VkSwapchainCreateInfoKHR swchCreateInfo = {};
    swchCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swchCreateInfo.surface          = this->surface;
    swchCreateInfo.minImageCount    = imageCount;
    swchCreateInfo.imageFormat      = this->swapchainImageFormat;
    swchCreateInfo.imageColorSpace  = surfaceFormat.colorSpace;
    swchCreateInfo.imageExtent      = this->swapchainExtent;
    swchCreateInfo.imageArrayLayers = 1;
    swchCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if ( indices.graphicsFamily != indices.presentFamily )
    {
      swchCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
      swchCreateInfo.queueFamilyIndexCount = 2;
      swchCreateInfo.pQueueFamilyIndices   = familyIndices;
    }
    else
    {
      swchCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
      swchCreateInfo.queueFamilyIndexCount = 0;
      swchCreateInfo.pQueueFamilyIndices   = nullptr;
    }
    swchCreateInfo.preTransform   = swapchainSupport.capabilities.currentTransform;
    swchCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swchCreateInfo.presentMode    = presentMode;
    swchCreateInfo.clipped        = VK_TRUE;
    swchCreateInfo.oldSwapchain   = oldSwapchain;

    // Create swapchain
    VkSwapchainKHR newSwapchain;
    if ( vkCreateSwapchainKHR( this->device, &swchCreateInfo, nullptr, &newSwapchain ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create swapchain!" );
    }
    *&this->swapchain = newSwapchain;

    // Retrieve handles to all swapchain images
    // Since the implementation can create more images,
    // we must query the number of images
    vkGetSwapchainImagesKHR( this->device, this->swapchain,
                             &imageCount, nullptr );
    this->swapchainImages.resize( imageCount );
    vkGetSwapchainImagesKHR( this->device, this->swapchain,
                             &imageCount, this->swapchainImages.data() );
  }

  void createImageViews(  )
  {
    this->swapchainImageViews.resize( this->swapchainImages.size(),
                                        VDeleter<VkImageView>{ this->device,
                                                               vkDestroyImageView } );

    for ( uint32_t i = 0; i < this->swapchainImages.size(); i++ )
    {
      VkImageViewCreateInfo imgviewCreateInfo = {};
      imgviewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      imgviewCreateInfo.image                           = this->swapchainImages[ i ];
      imgviewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
      imgviewCreateInfo.format                          = this->swapchainImageFormat;
      imgviewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imgviewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imgviewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imgviewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imgviewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      imgviewCreateInfo.subresourceRange.baseMipLevel   = 0;
      imgviewCreateInfo.subresourceRange.levelCount     = 1;
      imgviewCreateInfo.subresourceRange.baseArrayLayer = 0;
      imgviewCreateInfo.subresourceRange.layerCount     = 1;

      if ( vkCreateImageView( this->device, &imgviewCreateInfo, nullptr, &this->swapchainImageViews[ i ] ) != VK_SUCCESS )
      {
        throw std::runtime_error( "Failed to create image views!" );
      }
    }
  }

  void createRenderPass()
  {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format         = this->swapchainImageFormat;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc = {};
    subpassDesc.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments    = &colorAttachmentRef;

    // Add subpass dependency
    VkSubpassDependency dependency = {};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments    = &colorAttachment;
    renderPassCreateInfo.subpassCount    = 1;
    renderPassCreateInfo.pSubpasses      = &subpassDesc;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies   = &dependency;

    if ( vkCreateRenderPass( this->device, &renderPassCreateInfo, nullptr, &this->renderPass ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create render pass!" );
    }
  }

  void createGraphicsPipeline(  )
  {
    auto vertexShaderCode   = readFile( "vert.spv" );
    auto fragmentShaderCode = readFile( "frag.spv" );

    // Create shader modules
    VDeleter<VkShaderModule> vertexShader{this->device, vkDestroyShaderModule};
    VDeleter<VkShaderModule> fragmentShader{this->device, vkDestroyShaderModule};
    createShaderModule( this->device, vertexShaderCode, vertexShader );
    createShaderModule( this->device, fragmentShaderCode, fragmentShader );

    // Add to graphics pipeline
    VkPipelineShaderStageCreateInfo vertexShaderStageInfo = {};
    vertexShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertexShader;
    vertexShaderStageInfo.pName  = "main";
    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = {};
    fragmentShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageInfo.module = fragmentShader;
    fragmentShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
      vertexShaderStageInfo,
      fragmentShaderStageInfo
    };

    // Describe the format of the input vertex data
    auto bindingDescription    = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount   = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputCreateInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

    // Specify topology of input vertices
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // Create Viewport
    VkViewport viewport = {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float) this->swapchainExtent.width;
    viewport.height   = (float) this->swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Create Scissor Rectangle
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = this->swapchainExtent;

    // Combine Viewport and Scissor Rectangle into Viewport State
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports    = &viewport;
    viewportStateCreateInfo.scissorCount  = 1;
    viewportStateCreateInfo.pScissors     = &scissor;

    // Create Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable        = VK_FALSE;
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerCreateInfo.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizerCreateInfo.lineWidth               = 1.0f;
    rasterizerCreateInfo.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizerCreateInfo.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizerCreateInfo.depthBiasEnable         = VK_FALSE;
    rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizerCreateInfo.depthBiasClamp          = 0.0f;
    rasterizerCreateInfo.depthBiasSlopeFactor    = 0.0f;

    // Create Multisampling
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
    multisamplingCreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingCreateInfo.sampleShadingEnable   = VK_FALSE;
    multisamplingCreateInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisamplingCreateInfo.minSampleShading      = 1.0f;
    multisamplingCreateInfo.pSampleMask           = nullptr;
    multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisamplingCreateInfo.alphaToOneEnable      = VK_FALSE;

    // Configure Color Blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable         = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    // Create Color Blending for all framebuffers
    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
    colorBlendCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable     = VK_FALSE;
    colorBlendCreateInfo.logicOp           = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount   = 1;
    colorBlendCreateInfo.pAttachments      = &colorBlendAttachment;
    colorBlendCreateInfo.blendConstants[0] = 0.0f;
    colorBlendCreateInfo.blendConstants[1] = 0.0f;
    colorBlendCreateInfo.blendConstants[2] = 0.0f;
    colorBlendCreateInfo.blendConstants[3] = 0.0f;

    // Create Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount         = 0;
    pipelineLayoutCreateInfo.pSetLayouts            = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges    = 0;

    if (vkCreatePipelineLayout( this->device, &pipelineLayoutCreateInfo, nullptr,
                                &this->pipelineLayout ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create pipeline layout!" );
    }

    // Create Graphics Pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount          = 2;
    pipelineCreateInfo.pStages             = shaderStages;
    pipelineCreateInfo.pVertexInputState   = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pViewportState      = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState   = &multisamplingCreateInfo;
    pipelineCreateInfo.pDepthStencilState  = nullptr;
    pipelineCreateInfo.pColorBlendState    = &colorBlendCreateInfo;
    pipelineCreateInfo.pDynamicState       = nullptr;
    pipelineCreateInfo.layout              = this->pipelineLayout;
    pipelineCreateInfo.renderPass          = this->renderPass;
    pipelineCreateInfo.subpass             = 0;
    pipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex   = -1;

    if ( vkCreateGraphicsPipelines( this->device,
                                    VK_NULL_HANDLE,
                                    1,
                                    &pipelineCreateInfo,
                                    nullptr,
                                    &this->graphicsPipeline ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create graphics pipeline!" );
    }
  }

  void createFramebuffers()
  {
    this->swapchainFramebuffers.resize( this->swapchainImageViews.size(),
                                         VDeleter<VkFramebuffer>{ this->device,
                                                                  vkDestroyFramebuffer } );

    // Create framebuffers for image views
    for ( size_t i = 0; i < this->swapchainImageViews.size(); i++ )
    {
      VkImageView attachments[  ] = {
        this->swapchainImageViews[ i ]
      };

      VkFramebufferCreateInfo framebufferCreateInfo = {};
      framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferCreateInfo.renderPass = this->renderPass;
      framebufferCreateInfo.attachmentCount = 1;
      framebufferCreateInfo.pAttachments    = attachments;
      framebufferCreateInfo.width           = this->swapchainExtent.width;
      framebufferCreateInfo.height          = this->swapchainExtent.height;
      framebufferCreateInfo.layers          = 1;

      if ( vkCreateFramebuffer( this->device,
                                &framebufferCreateInfo,
                                nullptr,
                                &this->swapchainFramebuffers[ i ] ) != VK_SUCCESS )
      {
        throw std::runtime_error( "Failed to create framebuffer!" );
      }
    }
  }

  void createCommandPool(  )
  {
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = this->graphicsQueueIdx; 
    poolCreateInfo.flags            = 0;

    if ( vkCreateCommandPool( this->device, &poolCreateInfo,
                              nullptr, &this->commandPool ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create command pool!" );
    }
  }

  void createVertexBuffer(  )
  {
    VkDeviceSize bufferSize = sizeof( vertices[0] ) * vertices.size();

    // Create Staging Buffer
    VDeleter<VkBuffer>       stagingBuffer        { this->device, vkDestroyBuffer };
    VDeleter<VkDeviceMemory> stagingBufferMemory { this->device, vkFreeMemory };
    createBuffer( this->device,
                  this->physical,
                  bufferSize,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer,
                  stagingBufferMemory );
    
    // Memory map Staging Buffer
    void* data;
    vkMapMemory( this->device, stagingBufferMemory, 0, bufferSize, 0, &data );
    std::memcpy( data, vertices.data(), (size_t)bufferSize );
    vkUnmapMemory( this->device, stagingBufferMemory );

    // Copy contents of Staging Buffer into Vertex Buffer
    createBuffer( this->device,
                  this->physical,
                  bufferSize,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  this->vertexBuffer,
                  this->vertexBufferMemory );
    copyBuffer( this->device, this->graphicsQueue,
                this->commandPool, stagingBuffer,
                this->vertexBuffer, bufferSize );
  }

  void createCommandBuffers(  )
  {
    // Free old command buffers (if called from recreateSwapChain())
    if ( this->commandBuffers.size(  ) > 0 )
    {
      vkFreeCommandBuffers( this->device,
                            this->commandPool,
                            this->commandBuffers.size(),
                            this->commandBuffers.data() );
    }
    this->commandBuffers.resize( this->swapchainFramebuffers.size() );

    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool        = this->commandPool;
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = (uint32_t)this->commandBuffers.size();

    if ( vkAllocateCommandBuffers( this->device, &allocateInfo,
                                   this->commandBuffers.data() ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to allocate command buffers!" );
    }

    for ( size_t i = 0; i < this->commandBuffers.size(  ); i++ )
    {
      VkCommandBufferBeginInfo beginInfo = {};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
      beginInfo.pInheritanceInfo = nullptr;

      vkBeginCommandBuffer( this->commandBuffers[i], &beginInfo );
      // Start Render Pass
      VkRenderPassBeginInfo renderPassCreateInfo = {};
      renderPassCreateInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassCreateInfo.renderPass        = this->renderPass;
      renderPassCreateInfo.framebuffer       = this->swapchainFramebuffers[i];
      renderPassCreateInfo.renderArea.offset = {0, 0};
      renderPassCreateInfo.renderArea.extent = this->swapchainExtent;
      VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
      renderPassCreateInfo.clearValueCount   = 1;
      renderPassCreateInfo.pClearValues      = &clearColor;

      vkCmdBeginRenderPass(this->commandBuffers[i],
                           &renderPassCreateInfo,
                           VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(this->commandBuffers[i],
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        this->graphicsPipeline);

      // Bind vertex buffer
      VkBuffer vertexBuffers[] = { this->vertexBuffer };
      VkDeviceSize offsets[]    = { 0 };
      vkCmdBindVertexBuffers( this->commandBuffers[ i ], 0, 1,
                              vertexBuffers, offsets );
      
      vkCmdDraw(this->commandBuffers[i],
                vertices.size(),
                1, 0, 0);

      vkCmdEndRenderPass(this->commandBuffers[i]);

      if ( vkEndCommandBuffer( this->commandBuffers[i] ) != VK_SUCCESS )
      {
        throw std::runtime_error("failed to record command buffer!");
      }
    }

  }

  void createSemaphores(  )
  {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if ( vkCreateSemaphore( this->device, &semaphoreCreateInfo,
                            nullptr, &this->imageAvailableSemaphore ) != VK_SUCCESS ||
         vkCreateSemaphore( this->device, &semaphoreCreateInfo,
                            nullptr, &this->renderFinishSemaphore ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create semaphore!" );
    }
  }

  bool checkValidationLayerSupport(  )
  {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties( &layerCount, nullptr );
    std::vector<VkLayerProperties> availableLayers( layerCount );
    vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data(  ) );

    for ( const char* layer : validationLayers )
    {
      bool found = false;

      for ( const auto& properties : availableLayers )
      {
        if ( std::strcmp( layer, properties.layerName ) == 0 )
        {
          found = true;
          break;
        }
      }

      if ( !found )
      {
        return false;
      }
    }

    return true;
  }

  std::vector<const char*> getRequiredExtensions(  )
  {
    std::vector<const char*> extensions;

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

    for ( auto i = 0; i < glfwExtensionCount; i++ )
    {
      extensions.push_back( glfwExtensions[ i ] );
    }

    if ( enableValidationLayers )
    {
      extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
    }

    return extensions;
  }

  bool isDeviceSuitable( VkPhysicalDevice device )
  {
    auto indices = findQueueFamilies( device, this->surface );

    bool requiredExtensionsSupported = checkDeviceExtensionSupport( device );

    bool adequateSwapChain = false;
    if ( requiredExtensionsSupported )
    {
      auto swapchainSupport = querySwapChainSupport( device, this->surface );
      adequateSwapChain = !swapchainSupport.formats.empty() &&
                            !swapchainSupport.presentModes.empty();
    }
    
    return indices.isComplete() && requiredExtensionsSupported && adequateSwapChain;
  }

#ifndef WIN32
#define __stdcall
#endif
  static VkBool32 __stdcall debugCallback(
    VkDebugReportFlagsEXT      flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t                   obj,
    std::size_t                loc,
    int32_t                    code,
    const char*                layerPrefix,
    const char*                msg,
    void*                      userData)
  {
    std::cerr << "Validation layer: " << msg << std::endl;
    return VK_FALSE;
  }

  void createDebugCallback()
  {
    if ( !enableValidationLayers )
    {
      return;
    }
    
    VkDebugReportCallbackCreateInfoEXT crinfo = {};
    crinfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    crinfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    crinfo.pfnCallback = this->debugCallback;

    if ( CreateDebugReportCallbackEXT( this->instance,
                                       &crinfo,
                                       nullptr,
                                       &this->callback ) )
    {
      throw std::runtime_error( "Failed to setu up debug callback!" );
    }
  }
};

int main( )
{
  HelloTriangleApplication app;

  try
  {
    app.run();
  }
  catch (const std::runtime_error& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
