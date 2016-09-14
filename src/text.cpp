#include "com.hpp"
#include "text.hpp"
#include "png.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <fstream>

#include "binpack.hpp"

static FT_Library ft_lib;

struct test_set {
	unsigned int x, y;
	FT_ULong code;
	unsigned int width, height;
	FT_Glyph_Metrics metrics;
	
	test_set(unsigned int x, unsigned int y, FT_ULong c, FT_GlyphSlot const & g) : x(x), y(y), code(c), width(g->bitmap.width), height(g->bitmap.rows), metrics(g->metrics) {}
};

void text::test() {
	FT_Error err = FT_Init_FreeType(&ft_lib);
	if (err) {
		srcthrow("freetype init failed");
	}
	
	FT_Face face;
	err = FT_New_Face(ft_lib, "/usr/share/fonts/TTF/DejaVuSansMono.ttf", 0, &face);
	if (err) {
		srcthrow("freetype failed to find ttf file");
	}
	
	err = FT_Set_Char_Size(face, 0, 48 * 64, 100, 100);
	if (err) {
		srcthrow("");
	}
	
	//assert(face->glyph->bitmap.pitch > 0);
	
	std::vector<test_set> tests;
	for (FT_ULong i = 0x0021; i < 0x007F; i++) {
		FT_UInt index = FT_Get_Char_Index( face, i );
		err = FT_Load_Glyph(face, index, 0);
		if (err) {
			srcthrow("");
		}
		err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		if (err) {
			srcthrow("");
		}
		tests.emplace_back(0, 0, index, face->glyph);
	}
	
	std::sort(tests.begin(), tests.end(), [](test_set & a, test_set & b){return a.width * a.height > b.width * b.height;});
	
	bsp_packer<test_set, unsigned int> tnode {0, 0};
	
	for (test_set & t : tests) {
		auto c = tnode.pack(&t, t.width, t.height);
		t.x = c.x;
		t.y = c.y;
	}
	
	auto s = tnode.size();
	printf("%u, %u\n", s.x, s.y);
	
	png p {s.x, s.y};
	auto ptrs = p.raw_ptrs();
	
	for (test_set & t : tests) {
		FT_Load_Glyph(face, t.code, 0);
		FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		assert (face->glyph->bitmap.pitch > 0);
		for (int row = 0; row < t.height; row++) {
			for (int col = 0; col < t.width; col++) {
				ptrs[row + t.y][(t.x + col)*4 + 0] = face->glyph->bitmap.buffer[row * face->glyph->bitmap.pitch + col];
				ptrs[row + t.y][(t.x + col)*4 + 3] = 0xFF;
			}
		}
	}
	
	FT_Done_Face(face);
	FT_Done_FreeType(ft_lib);
	
	p.write("fonts.png");
}
