#include "com.hpp"
#include "png.hpp"

#include <cstdio>
#include <png.h>

png::png(char const * path) {
	/*
	png_structp pngs;
	png_infop pngi;
	
	std::ifstream f {path, std::ios::binary};
	if (!f.good()) {
		srcthrow("could not open \"%s\" for reading, or insufficient permissions", path);
	} 
	*/
	
	// TODO
}

png::png(unsigned int width, unsigned int height) : width_(width), height_(height) {
	row_ptrs = new byte * [height];
	for (unsigned int i = 0; i < height; i++) {
		row_ptrs[i] = new byte[width * 32] {};
	}
}

png::~png() {
	for (unsigned int i = 0; i < height; i++) {
		delete row_ptrs[i];
	}
	delete row_ptrs;
}

void png::write(char const * path) const {
	png_structp pngs;
	png_infop pngi;
	
	FILE * fp = fopen(path, "wb");
	if (!fp) {
		srcthrow("could not open \"%s\" for writing, or insufficient permissions", path);
	}
	
	pngs = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngs) srcthrow("could not create png write struct");
	
	pngi = png_create_info_struct(pngs);
	
	if (setjmp(png_jmpbuf(pngs))) {
		png_destroy_write_struct(&pngs, &pngi);
		fclose(fp);
		srcthrow("error during png write");
	}
	
	png_init_io(pngs, fp);
	png_set_IHDR(pngs, pngi, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(pngs, pngi);
	png_write_image(pngs, row_ptrs);
	png_write_end(pngs, pngi);
	
	png_destroy_write_struct(&pngs, &pngi);
	fclose(fp);
}
