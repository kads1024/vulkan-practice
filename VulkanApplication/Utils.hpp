#pragma once

#include <iostream>
#include <cassert>
#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK(func)									\
{														\
	const VkResult result = func;						\
	if (result != VK_SUCCESS) {							\
		std::cerr << "Error calling function " << #func	\
		<< " at " << __FILE__ << ":"					\
		<< __LINE__ << ". Result is "					\
		<< string_VkResult(result)						\
		<< std::endl;									\
		assert(false);									\
	}													\
}