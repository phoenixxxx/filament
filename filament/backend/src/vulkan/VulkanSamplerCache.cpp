/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <backend/platforms/VulkanPlatform.h>

#include "vulkan/VulkanSamplerCache.h"
#include "vulkan/utils/Conversion.h"

#include <utils/Panic.h>

using namespace bluevk;

namespace filament::backend {

VulkanSamplerCache::VulkanSamplerCache(VulkanPlatform* platform)
    : mPlatform(platform) {}

VkSampler VulkanSamplerCache::getSampler(const SamplerMetaData& data) noexcept {
    auto iter = mCache.find(data);
    if (UTILS_LIKELY(iter != mCache.end())) {
        return iter->second;
    }

    VkSampler sampler;
    if (data.externalFormat == EXTERNAL_SAMPLER_FORMAT_INVALID) {
        VkSamplerCreateInfo samplerInfo{ .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = fvkutils::getFilter(data.samplerParams.filterMag),
            .minFilter = fvkutils::getFilter(data.samplerParams.filterMin),
            .mipmapMode = fvkutils::getMipmapMode(data.samplerParams.filterMin),
            .addressModeU = fvkutils::getWrapMode(data.samplerParams.wrapS),
            .addressModeV = fvkutils::getWrapMode(data.samplerParams.wrapT),
            .addressModeW = fvkutils::getWrapMode(data.samplerParams.wrapR),
            .anisotropyEnable = data.samplerParams.anisotropyLog2 == 0 ? VK_FALSE : VK_TRUE,
            .maxAnisotropy = (float) (1u << data.samplerParams.anisotropyLog2),
            .compareEnable = fvkutils::getCompareEnable(data.samplerParams.compareMode),
            .compareOp = fvkutils::getCompareOp(data.samplerParams.compareFunc),
            .minLod = 0.0f,
            .maxLod = fvkutils::getMaxLod(data.samplerParams.filterMin),
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE };
        VkResult result = vkCreateSampler(mPlatform->getDevice(), &samplerInfo, VKALLOC, &sampler);
        FILAMENT_CHECK_POSTCONDITION(result == VK_SUCCESS)
                << "Unable to create sampler."
                << " error=" << static_cast<int32_t>(result);
    } else {
        sampler = mPlatform->createExternalSampler(data.YcbcrConversion, data.samplerParams,
                data.externalFormat);
    }
    mCache.insert({ data, sampler });
    return sampler;
}

void VulkanSamplerCache::terminate() noexcept {
    for (auto pair : mCache) {
        vkDestroySampler(mPlatform->getDevice(), pair.second, VKALLOC);
    }
    mCache.clear();
}

} // namespace filament::backend
