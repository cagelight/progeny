#include "com.hpp"
#include "vk.hpp"
#include "control.hpp"

#include <cstdarg>

void com::init() {
	try {
		
		control::init();
	
		vk::instance::init();
		
		vk::physical_device const & pdev = vk::get_physical_devices()[0];
		
		vk::device::capability_set qset;
		qset.emplace_back(vk::device::capability::graphics | vk::device::capability::presentable);
		qset.emplace_back(vk::device::capability::transfer);
		vk::device::capability_sort(qset);
		
		vk::device::logical_device_initializer ldi {pdev, qset};
		vk::device dev {ldi};
		
	} catch (com::exception & e) {
		com::print(e.msg);
		srcprintf(FATAL, "initialization failed");
		throw_fatal;
	}
}

void com::term() noexcept {
	vk::instance::term();
	control::term();
}

void com::frame() {
	control::frame();
}

static constexpr size_t strf_startlen = 1024;
std::string com::strf(char const * fmt, ...) noexcept {
	va_list va;
	va_start(va, fmt);
	char * tmp_buf = reinterpret_cast<char *>(malloc(strf_startlen));
	tmp_buf[strf_startlen - 1] = 0;
	size_t req_size = vsnprintf(tmp_buf, strf_startlen, fmt, va);
	va_end(va);
	if (req_size >= strf_startlen) {
		tmp_buf = reinterpret_cast<char *>(realloc(tmp_buf, req_size+1));
		va_start(va, fmt);
		vsprintf(tmp_buf, fmt, va);
		va_end(va);
		tmp_buf[req_size] = 0;
	}
	std::string r = {tmp_buf};
	free(tmp_buf);
	return {r};
}

void com::print(char const * str) noexcept {
	std::printf("%s\n", str);
}
