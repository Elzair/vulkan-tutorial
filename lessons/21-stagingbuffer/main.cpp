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
  int                                  graphics_queue_idx;
  int                                  present_queue_idx;
  VkQueue                              graphics_queue;
  VkQueue                              present_queue;
  VDeleter<VkSwapchainKHR>             swapchain { this->device, vkDestroySwapchainKHR };
  std::vector<VkImage>                 swapchain_images;
  VkFormat                             swapchain_image_format;
  VkExtent2D                           swapchain_extent;
  std::vector<VDeleter<VkImageView>>   swapchain_image_views;
  VDeleter<VkRenderPass>               render_pass       { this->device, vkDestroyRenderPass };
  VDeleter<VkPipelineLayout>           pipeline_layout   { this->device, vkDestroyPipelineLayout };
  VDeleter<VkPipeline>                 graphics_pipeline { this->device, vkDestroyPipeline };
  std::vector<VDeleter<VkFramebuffer>> swapchain_framebuffers;
  VDeleter<VkCommandPool>              command_pool         { this->device, vkDestroyCommandPool };
  VDeleter<VkBuffer>                   vertex_buffer        { this->device, vkDestroyBuffer };
  VDeleter<VkDeviceMemory>             vertex_buffer_memory { this->device, vkFreeMemory };
  std::vector<VkCommandBuffer>         command_buffers;
  VDeleter<VkSemaphore>                image_available_semaphore { this->device, vkDestroySemaphore };
  VDeleter<VkSemaphore>                render_finish_semaphore   { this->device, vkDestroySemaphore };
  
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
    uint32_t image_idx;
    auto result = vkAcquireNextImageKHR( this->device,
                                         this->swapchain,
                                         std::numeric_limits<uint64_t>::max(), // Disable timeout for image to become available
                                         this->image_available_semaphore,
                                         VK_NULL_HANDLE,
                                         &image_idx );
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
    VkSemaphore wait_semaphores[]      = { this->image_available_semaphore };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signal_semaphores[]    = { this->render_finish_semaphore };
    

    VkSubmitInfo submit_info = {};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = wait_semaphores;
    submit_info.pWaitDstStageMask    = wait_stages;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &this->command_buffers[image_idx];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = signal_semaphores;

    if ( vkQueueSubmit( this->graphics_queue,
                        1,
                        &submit_info,
                        VK_NULL_HANDLE ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to submit draw command buffer!" );
    }

    // Submit result to swap chain
    VkSwapchainKHR swapchains[] = { this->swapchain };

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = signal_semaphores;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = swapchains;
    present_info.pImageIndices      = &image_idx;
    present_info.pResults           = nullptr;

    vkQueuePresentKHR( this->present_queue, &present_info );
  }

  void createInstance(  )
  {
    // Ensure validation layers are available
    if ( enable_validation_layers && !this->checkValidationLayerSupport() )
    {
      throw std::runtime_error( "Validation layers requested but are not available!" );
    }
    
    VkApplicationInfo app_info  = {};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    app_info.pEngineName        = "No Engine";
    app_info.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
    app_info.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo inst_create_info    = {};
    inst_create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_create_info.pApplicationInfo        = &app_info;
    auto vkextensions                        = this->getRequiredExtensions();
    inst_create_info.enabledExtensionCount   = vkextensions.size();
    inst_create_info.ppEnabledExtensionNames = vkextensions.data();
    if ( enable_validation_layers )
    {
      inst_create_info.enabledLayerCount   = validation_layers.size(  );
      inst_create_info.ppEnabledLayerNames = validation_layers.data(  );
    }
    else
    {
      inst_create_info.enabledLayerCount = 0;
    }

    if ( vkCreateInstance( &inst_create_info, nullptr, &this->instance ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create instance!" );
    }
    else
    {
      std::cout << "Created instance!" << std::endl;
    }

    // Get information on extension properties
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties( nullptr, &extension_count, nullptr );
    std::vector<VkExtensionProperties> extensions( extension_count );
    vkEnumerateInstanceExtensionProperties( nullptr, &extension_count, extensions.data(  ) );

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
    uint32_t dev_count = 0;
    vkEnumeratePhysicalDevices( this->instance, &dev_count, nullptr);
    if ( dev_count == 0 )
    {
      throw std::runtime_error( "Failed to find GPUs with Vulkan support!" );
    }
    std::vector<VkPhysicalDevice> devices( dev_count );
    vkEnumeratePhysicalDevices( this->instance, &dev_count, devices.data() );

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
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<int> unique_queue_families = {
      indices.graphics_family,
      indices.present_family
    };
    float queue_priority = 1.0f;

    for ( int queue_family : unique_queue_families )
    {
      VkDeviceQueueCreateInfo queue_create_info = {};
      queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_info.queueFamilyIndex = queue_family;
      queue_create_info.queueCount       = 1;
      queue_create_info.pQueuePriorities = &queue_priority;
      queue_create_infos.push_back( queue_create_info );
    }

    VkPhysicalDeviceFeatures dev_features = {};

    // Create struct used to create a logical device
    VkDeviceCreateInfo dev_create_info = {};
    dev_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_create_info.pQueueCreateInfos       = queue_create_infos.data();
    dev_create_info.queueCreateInfoCount    = (uint32_t)queue_create_infos.size();
    dev_create_info.pEnabledFeatures        = &dev_features;
    dev_create_info.enabledExtensionCount   = required_device_extensions.size();
    dev_create_info.ppEnabledExtensionNames = required_device_extensions.data();
    if ( enable_validation_layers )
    {
      dev_create_info.enabledLayerCount   = validation_layers.size(  );
      dev_create_info.ppEnabledLayerNames = validation_layers.data(  );
    }
    else
    {
      dev_create_info.enabledLayerCount = 0;
    }

    if ( vkCreateDevice( this->physical, &dev_create_info, nullptr, &this->device ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create logical device!" );
    }

    // Retrieve handles for graphics and presentation queues
    vkGetDeviceQueue( this->device, indices.graphics_family, 0, &this->graphics_queue);
    vkGetDeviceQueue( this->device, indices.present_family,  0, &this->present_queue );
  }

  void createSwapChain( )
  {
    auto swapchain_support = querySwapChainSupport( this->physical, this->surface );
    auto surface_format    = chooseSwapChainSurfaceFormat( swapchain_support.formats );
    auto present_mode      = chooseSwapChainPresentMode( swapchain_support.present_modes );
    auto extent            = chooseSwapChainExtent( swapchain_support.capabilities,
                                                    WIDTH,
                                                    HEIGHT );

    this->swapchain_image_format = surface_format.format;
    this->swapchain_extent       = extent;

    // Ensure we have enough images to properly implement triple buffering
    // A value of 0 for maxImageCount means there is no hard limit on # images
    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if ( swapchain_support.capabilities.maxImageCount > 0 &&
         image_count > swapchain_support.capabilities.maxImageCount )
    {
      image_count = swapchain_support.capabilities.maxImageCount;
    }

    // Get indices of graphics and present queues
    auto indices = findQueueFamilies( this->physical, this->surface );
    this->graphics_queue_idx = indices.graphics_family;
    this->present_queue_idx  = indices.present_family;
    uint32_t family_indices[] = {
      (uint32_t)indices.graphics_family,
      (uint32_t)indices.present_family
    };

    // Create struct used to create a swapchain
    VkSwapchainKHR old_swapchain = this->swapchain;
    
    VkSwapchainCreateInfoKHR swch_create_info = {};
    swch_create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swch_create_info.surface          = this->surface;
    swch_create_info.minImageCount    = image_count;
    swch_create_info.imageFormat      = this->swapchain_image_format;
    swch_create_info.imageColorSpace  = surface_format.colorSpace;
    swch_create_info.imageExtent      = this->swapchain_extent;
    swch_create_info.imageArrayLayers = 1;
    swch_create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if ( indices.graphics_family != indices.present_family )
    {
      swch_create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
      swch_create_info.queueFamilyIndexCount = 2;
      swch_create_info.pQueueFamilyIndices   = family_indices;
    }
    else
    {
      swch_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
      swch_create_info.queueFamilyIndexCount = 0;
      swch_create_info.pQueueFamilyIndices   = nullptr;
    }
    swch_create_info.preTransform   = swapchain_support.capabilities.currentTransform;
    swch_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swch_create_info.presentMode    = present_mode;
    swch_create_info.clipped        = VK_TRUE;
    swch_create_info.oldSwapchain   = old_swapchain;

    // Create swapchain
    VkSwapchainKHR new_swapchain;
    if ( vkCreateSwapchainKHR( this->device, &swch_create_info, nullptr, &new_swapchain ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create swapchain!" );
    }
    *&this->swapchain = new_swapchain;

    // Retrieve handles to all swapchain images
    // Since the implementation can create more images,
    // we must query the number of images
    vkGetSwapchainImagesKHR( this->device, this->swapchain,
                             &image_count, nullptr );
    this->swapchain_images.resize( image_count );
    vkGetSwapchainImagesKHR( this->device, this->swapchain,
                             &image_count, this->swapchain_images.data() );
  }

  void createImageViews(  )
  {
    this->swapchain_image_views.resize( this->swapchain_images.size(),
                                        VDeleter<VkImageView>{ this->device,
                                                               vkDestroyImageView } );

    for ( uint32_t i = 0; i < this->swapchain_images.size(); i++ )
    {
      VkImageViewCreateInfo imgview_create_info = {};
      imgview_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      imgview_create_info.image                           = this->swapchain_images[ i ];
      imgview_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
      imgview_create_info.format                          = this->swapchain_image_format;
      imgview_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imgview_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imgview_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imgview_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imgview_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      imgview_create_info.subresourceRange.baseMipLevel   = 0;
      imgview_create_info.subresourceRange.levelCount     = 1;
      imgview_create_info.subresourceRange.baseArrayLayer = 0;
      imgview_create_info.subresourceRange.layerCount     = 1;

      if ( vkCreateImageView( this->device, &imgview_create_info, nullptr, &this->swapchain_image_views[ i ] ) != VK_SUCCESS )
      {
        throw std::runtime_error( "Failed to create image views!" );
      }
    }
  }

  void createRenderPass()
  {
    VkAttachmentDescription color_attachment = {};
    color_attachment.format         = this->swapchain_image_format;
    color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc = {};
    subpass_desc.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments    = &color_attachment_ref;

    // Add subpass dependency
    VkSubpassDependency dependency = {};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments    = &color_attachment;
    render_pass_create_info.subpassCount    = 1;
    render_pass_create_info.pSubpasses      = &subpass_desc;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies   = &dependency;

    if ( vkCreateRenderPass( this->device, &render_pass_create_info, nullptr, &this->render_pass ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create render pass!" );
    }
  }

  void createGraphicsPipeline(  )
  {
    auto vertex_shader_code   = readFile( "vert.spv" );
    auto fragment_shader_code = readFile( "frag.spv" );

    // Create shader modules
    VDeleter<VkShaderModule> vertex_shader{this->device, vkDestroyShaderModule};
    VDeleter<VkShaderModule> fragment_shader{this->device, vkDestroyShaderModule};
    createShaderModule( this->device, vertex_shader_code, vertex_shader );
    createShaderModule( this->device, fragment_shader_code, fragment_shader );

    // Add to graphics pipeline
    VkPipelineShaderStageCreateInfo vertex_shader_stage_info = {};
    vertex_shader_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_info.module = vertex_shader;
    vertex_shader_stage_info.pName  = "main";
    VkPipelineShaderStageCreateInfo fragment_shader_stage_info = {};
    fragment_shader_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = fragment_shader;
    fragment_shader_stage_info.pName  = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
      vertex_shader_stage_info,
      fragment_shader_stage_info
    };

    // Describe the format of the input vertex data
    auto binding_description    = Vertex::getBindingDescription();
    auto attribute_descriptions = Vertex::getAttributeDescriptions();
    
    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {};
    vertex_input_create_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.vertexBindingDescriptionCount   = 1;
    vertex_input_create_info.pVertexBindingDescriptions      = &binding_description;
    vertex_input_create_info.vertexAttributeDescriptionCount = attribute_descriptions.size();
    vertex_input_create_info.pVertexAttributeDescriptions    = attribute_descriptions.data();

    // Specify topology of input vertices
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {};
    input_assembly_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    // Create Viewport
    VkViewport viewport = {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float) this->swapchain_extent.width;
    viewport.height   = (float) this->swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Create Scissor Rectangle
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = this->swapchain_extent;

    // Combine Viewport and Scissor Rectangle into Viewport State
    VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
    viewport_state_create_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports    = &viewport;
    viewport_state_create_info.scissorCount  = 1;
    viewport_state_create_info.pScissors     = &scissor;

    // Create Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {};
    rasterizer_create_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_create_info.depthClampEnable        = VK_FALSE;
    rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_create_info.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer_create_info.lineWidth               = 1.0f;
    rasterizer_create_info.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer_create_info.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_create_info.depthBiasEnable         = VK_FALSE;
    rasterizer_create_info.depthBiasConstantFactor = 0.0f;
    rasterizer_create_info.depthBiasClamp          = 0.0f;
    rasterizer_create_info.depthBiasSlopeFactor    = 0.0f;

    // Create Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling_create_info = {};
    multisampling_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_create_info.sampleShadingEnable   = VK_FALSE;
    multisampling_create_info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling_create_info.minSampleShading      = 1.0f;
    multisampling_create_info.pSampleMask           = nullptr;
    multisampling_create_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_create_info.alphaToOneEnable      = VK_FALSE;

    // Configure Color Blending
    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable         = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    // Create Color Blending for all framebuffers
    VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
    color_blend_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_create_info.logicOpEnable     = VK_FALSE;
    color_blend_create_info.logicOp           = VK_LOGIC_OP_COPY;
    color_blend_create_info.attachmentCount   = 1;
    color_blend_create_info.pAttachments      = &color_blend_attachment;
    color_blend_create_info.blendConstants[0] = 0.0f;
    color_blend_create_info.blendConstants[1] = 0.0f;
    color_blend_create_info.blendConstants[2] = 0.0f;
    color_blend_create_info.blendConstants[3] = 0.0f;

    // Create Pipeline Layout
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount         = 0;
    pipeline_layout_create_info.pSetLayouts            = nullptr;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges    = 0;

    if (vkCreatePipelineLayout( this->device, &pipeline_layout_create_info, nullptr,
                                &this->pipeline_layout ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create pipeline layout!" );
    }

    // Create Graphics Pipeline
    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount          = 2;
    pipeline_create_info.pStages             = shader_stages;
    pipeline_create_info.pVertexInputState   = &vertex_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    pipeline_create_info.pViewportState      = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterizer_create_info;
    pipeline_create_info.pMultisampleState   = &multisampling_create_info;
    pipeline_create_info.pDepthStencilState  = nullptr;
    pipeline_create_info.pColorBlendState    = &color_blend_create_info;
    pipeline_create_info.pDynamicState       = nullptr;
    pipeline_create_info.layout              = this->pipeline_layout;
    pipeline_create_info.renderPass          = this->render_pass;
    pipeline_create_info.subpass             = 0;
    pipeline_create_info.basePipelineHandle  = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex   = -1;

    if ( vkCreateGraphicsPipelines( this->device,
                                    VK_NULL_HANDLE,
                                    1,
                                    &pipeline_create_info,
                                    nullptr,
                                    &this->graphics_pipeline ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create graphics pipeline!" );
    }
  }

  void createFramebuffers()
  {
    this->swapchain_framebuffers.resize( this->swapchain_image_views.size(),
                                         VDeleter<VkFramebuffer>{ this->device,
                                                                  vkDestroyFramebuffer } );

    // Create framebuffers for image views
    for ( size_t i = 0; i < this->swapchain_image_views.size(); i++ )
    {
      VkImageView attachments[  ] = {
        this->swapchain_image_views[ i ]
      };

      VkFramebufferCreateInfo framebuffer_create_info = {};
      framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_create_info.renderPass = this->render_pass;
      framebuffer_create_info.attachmentCount = 1;
      framebuffer_create_info.pAttachments    = attachments;
      framebuffer_create_info.width           = this->swapchain_extent.width;
      framebuffer_create_info.height          = this->swapchain_extent.height;
      framebuffer_create_info.layers          = 1;

      if ( vkCreateFramebuffer( this->device,
                                &framebuffer_create_info,
                                nullptr,
                                &this->swapchain_framebuffers[ i ] ) != VK_SUCCESS )
      {
        throw std::runtime_error( "Failed to create framebuffer!" );
      }
    }
  }

  void createCommandPool(  )
  {
    VkCommandPoolCreateInfo pool_create_info = {};
    pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = this->graphics_queue_idx; 
    pool_create_info.flags            = 0;

    if ( vkCreateCommandPool( this->device, &pool_create_info,
                              nullptr, &this->command_pool ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create command pool!" );
    }
  }

  void createVertexBuffer(  )
  {
    VkDeviceSize buffer_size = sizeof( vertices[0] ) * vertices.size();

    // Create Staging Buffer
    VDeleter<VkBuffer>       staging_buffer        { this->device, vkDestroyBuffer };
    VDeleter<VkDeviceMemory> staging_buffer_memory { this->device, vkFreeMemory };
    createBuffer( this->device,
                  this->physical,
                  buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  staging_buffer,
                  staging_buffer_memory );
    
    // Memory map Staging Buffer
    void* data;
    vkMapMemory( this->device, staging_buffer_memory, 0, buffer_size, 0, &data );
    std::memcpy( data, vertices.data(), (size_t)buffer_size );
    vkUnmapMemory( this->device, staging_buffer_memory );

    // Copy contents of Staging Buffer into Vertex Buffer
    createBuffer( this->device,
                  this->physical,
                  buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  this->vertex_buffer,
                  this->vertex_buffer_memory );
    copyBuffer( this->device, this->graphics_queue,
                this->command_pool, staging_buffer,
                this->vertex_buffer, buffer_size );
  }

  void createCommandBuffers(  )
  {
    // Free old command buffers (if called from recreateSwapChain())
    if ( this->command_buffers.size(  ) > 0 )
    {
      vkFreeCommandBuffers( this->device,
                            this->command_pool,
                            this->command_buffers.size(),
                            this->command_buffers.data() );
    }
    this->command_buffers.resize( this->swapchain_framebuffers.size() );

    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool        = this->command_pool;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = (uint32_t)this->command_buffers.size();

    if ( vkAllocateCommandBuffers( this->device, &allocate_info,
                                   this->command_buffers.data() ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to allocate command buffers!" );
    }

    for ( size_t i = 0; i < this->command_buffers.size(  ); i++ )
    {
      VkCommandBufferBeginInfo begin_info = {};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
      begin_info.pInheritanceInfo = nullptr;

      vkBeginCommandBuffer( this->command_buffers[i], &begin_info );
      // Start Render Pass
      VkRenderPassBeginInfo render_pass_create_info = {};
      render_pass_create_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      render_pass_create_info.renderPass        = this->render_pass;
      render_pass_create_info.framebuffer       = this->swapchain_framebuffers[i];
      render_pass_create_info.renderArea.offset = {0, 0};
      render_pass_create_info.renderArea.extent = this->swapchain_extent;
      VkClearValue clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
      render_pass_create_info.clearValueCount   = 1;
      render_pass_create_info.pClearValues      = &clear_color;

      vkCmdBeginRenderPass(this->command_buffers[i],
                           &render_pass_create_info,
                           VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(this->command_buffers[i],
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        this->graphics_pipeline);

      // Bind vertex buffer
      VkBuffer vertex_buffers[] = { this->vertex_buffer };
      VkDeviceSize offsets[]    = { 0 };
      vkCmdBindVertexBuffers( this->command_buffers[ i ], 0, 1,
                              vertex_buffers, offsets );
      
      vkCmdDraw(this->command_buffers[i],
                vertices.size(),
                1, 0, 0);

      vkCmdEndRenderPass(this->command_buffers[i]);

      if ( vkEndCommandBuffer( this->command_buffers[i] ) != VK_SUCCESS )
      {
        throw std::runtime_error("failed to record command buffer!");
      }
    }

  }

  void createSemaphores(  )
  {
    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if ( vkCreateSemaphore( this->device, &semaphore_create_info,
                            nullptr, &this->image_available_semaphore ) != VK_SUCCESS ||
         vkCreateSemaphore( this->device, &semaphore_create_info,
                            nullptr, &this->render_finish_semaphore ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create semaphore!" );
    }
  }

  bool checkValidationLayerSupport(  )
  {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties( &layer_count, nullptr );
    std::vector<VkLayerProperties> available_layers( layer_count );
    vkEnumerateInstanceLayerProperties( &layer_count, available_layers.data(  ) );

    for ( const char* layer : validation_layers )
    {
      bool found = false;

      for ( const auto& properties : available_layers )
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

    unsigned int glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions( &glfw_extension_count );

    for ( auto i = 0; i < glfw_extension_count; i++ )
    {
      extensions.push_back( glfw_extensions[ i ] );
    }

    if ( enable_validation_layers )
    {
      extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
    }

    return extensions;
  }

  bool isDeviceSuitable( VkPhysicalDevice device )
  {
    auto indices = findQueueFamilies( device, this->surface );

    bool required_extensions_supported = checkDeviceExtensionSupport( device );

    bool adequate_swap_chain = false;
    if ( required_extensions_supported )
    {
      auto swapchain_support = querySwapChainSupport( device, this->surface );
      adequate_swap_chain = !swapchain_support.formats.empty() &&
                            !swapchain_support.present_modes.empty();
    }
    
    return indices.isComplete() && required_extensions_supported && adequate_swap_chain;
  }

#ifndef WIN32
#define __stdcall
#endif
  static VkBool32 __stdcall debugCallback(
    VkDebugReportFlagsEXT      flags,
    VkDebugReportObjectTypeEXT obj_type,
    uint64_t                   obj,
    std::size_t                loc,
    int32_t                    code,
    const char*                layer_prefix,
    const char*                msg,
    void*                      user_data)
  {
    std::cerr << "Validation layer: " << msg << std::endl;
    return VK_FALSE;
  }

  void createDebugCallback()
  {
    if ( !enable_validation_layers )
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
