#include <algorithm>

#include "Swapchain.hpp"
#include "UtilObjects.hpp"
#include "logger.hpp"


Swapchain::Swapchain(GraphicsContext* ptr, GLFWwindow* window) : gc_ptr(ptr) {
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

    create_render_pass();
    logger::log(LStatus::INFO, "created render pass");

    create_framebuffers(); 
    logger::log(LStatus::INFO, "created framebuffers");
}


Swapchain::~Swapchain() {
    VkDevice mydevice = gc_ptr->get_device();

    for (VkFramebuffer& buf : framebuffers) {
        vkDestroyFramebuffer(mydevice, buf, nullptr);
    }

    vkDestroyRenderPass(mydevice, render_pass, nullptr);

    for (VkImageView& view : image_views) {
        vkDestroyImageView(mydevice, view, nullptr);
    }

    vkDestroySwapchainKHR(mydevice, swapchain, nullptr);
    gc_ptr = nullptr;
}


void Swapchain::create_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
        return;
    }

    int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    extent = {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height)
    };

    extent.width = std::clamp(
        extent.width, 
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );

    extent.height = std::clamp(
        extent.height, 
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );
}


void Swapchain::create_swapchain(GLFWwindow* window) {
    VkDevice mydevice = gc_ptr->get_device();
    VkPhysicalDevice myphysdevice = gc_ptr->get_physical_device();
    VkSurfaceKHR mysurface = gc_ptr->get_surface();
    SwapChainSupportDetails swapchain_support(myphysdevice, mysurface);
    
    create_extent(swapchain_support.capabilities, window);
    format = VK_FORMAT_B8G8R8A8_SRGB;
    
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
        .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
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
        .format = format,
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
    
    VkAttachmentDescription color_attachment = {
        .format  = format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };


    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };


    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };
    

    if (vkCreateRenderPass(mydevice, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
        throw std::runtime_error("could not create render pass");
    }
}


void Swapchain::create_framebuffers() {
    VkDevice mydevice = gc_ptr->get_device();
    framebuffers.resize(images.size());
    
    for (size_t i = 0; i < framebuffers.size(); ++i) {
        VkFramebufferCreateInfo fb_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass,
            .attachmentCount = 1,
            .pAttachments = &image_views[i],
            .width = extent.width,
            .height = extent.height,
            .layers = 1
        };

        if (vkCreateFramebuffer(mydevice, &fb_info, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("could not create framebuffer");
        }
    }
}
