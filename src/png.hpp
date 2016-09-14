#pragma once

#include "com.hpp"

class png {

public:
	
	png() = delete;
	png(unsigned int width, unsigned int height);
	png(char const *);
	
	~png();

	void write(char const * path) const;
	
	unsigned int const & width {width_};
	unsigned int const & height {height_};
	byte * const * raw_ptrs() {return row_ptrs;}
	
private:
	
	byte * * row_ptrs;
	unsigned int width_, height_;
	
};
