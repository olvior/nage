#include "sync.h"

void sync_initialise(Renderer* renderer)
{
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        CHECK_VK_FATAL(
            vkCreateFence(renderer->device, &fence_info, NULL, &renderer->fences[i])
        );

        CHECK_VK_FATAL(
            vkCreateSemaphore(
                renderer->device,
                &semaphore_info,
                NULL,
                &renderer->semaphores_render[i]
            )
        );

        CHECK_VK_FATAL(
            vkCreateSemaphore(
                renderer->device,
                &semaphore_info,
                NULL,
                &renderer->semaphores_swapchain[i]
            )
        );
	}
}

void sync_cleanup(Renderer* renderer)
{
    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(renderer->device, renderer->semaphores_render[i], NULL);
        vkDestroySemaphore(renderer->device, renderer->semaphores_swapchain[i], NULL);
        vkDestroyFence(renderer->device, renderer->fences[i], NULL);
    }
}

