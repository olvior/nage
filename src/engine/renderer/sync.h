#include "renderer.h"
#include "../utils.h"

void sync_initialise(Renderer* renderer);
void sync_cleanup(Renderer* renderer);

VkSemaphoreSubmitInfo get_semaphore_submit_info(VkPipelineStageFlags2 stage_mask,
        VkSemaphore semaphore);
