#pragma once
#include <cstdio>
#include <cassert>
#include <cstdlib>

void CHECK(bool check, const char* fileName, int lineNumber)
{
	if (!check)
	{
		printf("CHECK() failed at %s:%i\n", fileName, lineNumber);
		assert(false);
		exit(EXIT_FAILURE);
	}
}

#define VK_CHECK(value) CHECK(value == VK_SUCCESS, __FILE__, __LINE__);
#define VK_CHECK_RET(value) if ( value != VK_SUCCESS ) { CHECK(false, __FILE__, __LINE__); return value; }
#define BL_CHECK(value) CHECK(value, __FILE__, __LINE__);

VkResult createSemaphore(VkDevice device, VkSemaphore* outSemaphore) 
{
	const VkSemaphoreCreateInfo ci = { 
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO 
	};
	return vkCreateSemaphore(device, &ci, nullptr, outSemaphore);
}
