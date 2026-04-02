#include <algorithm>

#include "core/Swapchain.hpp"
#include "utils/UtilObjects.hpp"
#include "utils/logger.hpp"


Swapchain::Swapchain(GraphicsContext *const ptr, GLFWwindow *const window) : gc_ptr(ptr), depth_buffer(ptr) {
    if (gc_ptr == nullptr) {
        throw std::runtime_error("could not create swapchain: the graphics context is invalid");
    }
    VkDevice mydevice = gc_ptr->get_device();

    create_swapchain(window);
    logger::log(LStatus::INFO, "created swapchain");
    
    extract_images();
    logger::log(LStatus::INFO, "got {} images from swapchain", get_images_cnt());

    create_image_views();
    logger::log(LStatus::INFO, "created image views");

    depth_buffer.load(depth_format, extent);

    create_render_pass();
    logger::log(LStatus::INFO, "created render pass");

    create_framebuffers(); 
    logger::log(LStatus::INFO, "created framebuffers");


    create_semaphores();
    logger::log(LStatus::INFO, "created semaphores for syncing graphics queue");

}


Swapchain::~Swapchain() {
    VkDevice mydevice = gc_ptr->get_device();

    for (VkSemaphore& sem : render_finished) {
        vkDestroySemaphore(mydevice, sem, nullptr);
    }

    for (VkFramebuffer& buf : framebuffers) {
        vkDestroyFramebuffer(mydevice, buf, nullptr);
    }

    vkDestroyRenderPass(mydevice, render_pass, nullptr);
    
    depth_buffer.unload();

    for (VkImageView& view : image_views) {
        vkDestroyImageView(mydevice, view, nullptr);
    }

    vkDestroySwapchainKHR(mydevice, swapchain, nullptr);
}


void Swapchain::setup_extent_and_format(GLFWwindow* window) {

    SwapChainSupportDetails support(gc_ptr->get_physical_device(), gc_ptr->get_surface());

    int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    extent = {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height)
    };

    extent.width = std::clamp(
        extent.width, 
        support.capabilities.minImageExtent.width,
        support.capabilities.maxImageExtent.width
    );

    extent.height = std::clamp(
        extent.height, 
        support.capabilities.minImageExtent.height,
        support.capabilities.maxImageExtent.height
    );

    depth_format = VK_FORMAT_UNDEFINED;
    std::array<VkFormat, 2> formats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
    for (VkFormat candidate : formats) {
        VkFormatProperties2 format_properties = { 
            .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 
        };
        vkGetPhysicalDeviceFormatProperties2(
            gc_ptr->get_physical_device(), candidate, &format_properties
        );
        if (format_properties.formatProperties.optimalTilingFeatures & 
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            depth_format = candidate;
            break;
        }
    }
    if (depth_format == VK_FORMAT_UNDEFINED) {
        throw std::runtime_error("could not find supported depth format");
    }
    color_format = VK_FORMAT_B8G8R8A8_SRGB;
}


void Swapchain::create_swapchain(GLFWwindow *const window) {
    VkDevice mydevice = gc_ptr->get_device();
    VkPhysicalDevice myphysdevice = gc_ptr->get_physical_device();
    VkSurfaceKHR mysurface = gc_ptr->get_surface();
    SwapChainSupportDetails swapchain_support(myphysdevice, mysurface);
    
    setup_extent_and_format(window);
    
    uint32_t image_cnt = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount) {
        image_cnt = std::min(image_cnt, swapchain_support.capabilities.maxImageCount);
    }
    
    std::array<uint32_t, 2> qindices = {
        gc_ptr->get_graphics_family_index(),
        gc_ptr->get_present_family_index(),
    };

    VkSwapchainCreateInfoKHR swapchain_cinfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = mysurface,
        .minImageCount = image_cnt,
        .imageFormat = color_format,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = 2,
        .pQueueFamilyIndices = qindices.data(),
        .preTransform = swapchain_support.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    if (vkCreateSwapchainKHR(mydevice, &swapchain_cinfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("could not create swap chain");
    }
}


void Swapchain::extract_images() {
    VkDevice mydevice = gc_ptr->get_device();
    uint32_t image_cnt;
    vkGetSwapchainImagesKHR(mydevice, swapchain, &image_cnt, nullptr);
    images.resize(image_cnt);
    vkGetSwapchainImagesKHR(mydevice, swapchain, &image_cnt, images.data());
}


void Swapchain::create_image_views() {
    size_t cnt = images.size();
    VkImageViewCreateInfo imv_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = VK_NULL_HANDLE,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = color_format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    image_views.resize(cnt);
    for (size_t i = 0; i < cnt; ++i) {
        imv_info.image = images[i];
        if (vkCreateImageView(gc_ptr->get_device(), &imv_info, nullptr, &image_views[i]) != VK_SUCCESS) {
            throw std::runtime_error("could not create image");
        }
    }
}



void Swapchain::create_render_pass() {
    VkDevice mydevice = gc_ptr->get_device();
    
    std::array<VkAttachmentDescription, 2> attachment_descs = {
        VkAttachmentDescription {
            .format  = color_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        },
        VkAttachmentDescription {
            .format  = depth_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        }
    };

    std::array<VkAttachmentReference, 2> refs = {
        VkAttachmentReference {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        },
        VkAttachmentReference {
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        }
    };
    
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &refs[0],
        .pDepthStencilAttachment = &refs[1]
    };


    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | 
                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };


    VkRenderPassCreateInfo render_pass_ci = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 2,
        .pAttachments = attachment_descs.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };
    

    if (vkCreateRenderPass(mydevice, &render_pass_ci, nullptr, &render_pass) != VK_SUCCESS) {
        throw std::runtime_error("could not create render pass");
    }
}


void Swapchain::create_framebuffers() {
    VkDevice mydevice = gc_ptr->get_device();
    framebuffers.resize(images.size());

    std::array<VkImageView, 2> attachments;
    attachments[1] = depth_buffer.get_view();

    for (size_t i = 0; i < framebuffers.size(); ++i) {
        attachments[0] = image_views[i];
        VkFramebufferCreateInfo fb_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass,
            .attachmentCount = 2,
            .pAttachments = attachments.data(),
            .width = extent.width,
            .height = extent.height,
            .layers = 1
        };

        if (vkCreateFramebuffer(mydevice, &fb_info, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("could not create framebuffer");
        }
    }
}

void Swapchain::create_semaphores() {
    render_finished.resize(get_images_cnt());
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    for (VkSemaphore& sem : render_finished) {
        if (vkCreateSemaphore(gc_ptr->get_device(), &semaphore_info, nullptr, &sem) != VK_SUCCESS) {
            throw std::runtime_error("could not create semaphore");
        }
    }
}

uint32_t Swapchain::acquire_next_image(VkSemaphore sem) {
    uint32_t index;
    
    vkAcquireNextImageKHR(
        gc_ptr->get_device(),
        swapchain,
        UINT64_MAX,
        sem,
        VK_NULL_HANDLE,
        &index
    );

    return index;
}

