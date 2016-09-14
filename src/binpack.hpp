#pragma once

#include "pmath.hpp"

template <typename T, typename Q = unsigned int> class bsp_packer {
	
public:
	
	bsp_packer(Q init_width, Q init_height) {
		root = new bsp_packer_node(init_width, init_height);
	}
	
	~bsp_packer() {
		delete root;
	}
	
	struct bsp_coords {
		bsp_coords(Q x, Q y) : x(x), y(y) {}
		bsp_coords() : bsp_coords(0, 0) {}
		Q x;
		Q y;
	};
	
	bsp_coords pack (T * v, Q width, Q height) {
		bsp_coords c;
		bool ok;
		root->pack_iter(ok, v, width, height, c);
		
		if (!ok) { // expand
			if (root->width + width > root->height + height) { // expand by height
				bsp_packer_node * new_root = new bsp_packer_node {promath::max(root->width, width), root->height + height};
				new_root->child_a = root;
				new_root->child_b = new bsp_packer_node {nullptr, promath::max(root->width, width), height, 0, root->height};
				new_root->pack_iter(ok, v, width, height, c);
				assert(ok);
				root = new_root;
			} else { // expand by width
				bsp_packer_node * new_root = new bsp_packer_node {root->width + width, promath::max(root->height, height)};
				new_root->child_a = root;
				new_root->child_b = new bsp_packer_node {nullptr, width, promath::max(root->height, height), root->width, 0};
				new_root->pack_iter(ok, v, width, height, c);
				assert(ok);
				root = new_root;
			}
		}
		
		return c;
	}
	
	bsp_coords size() {
		return {root->width, root->height};
	}
	
private:

	struct bsp_packer_node {
		
		bsp_packer_node(T const * v, Q width, Q height, Q x, Q y) : width(width), height(height), coords(x, y), value(v) {}
		bsp_packer_node(T const * v, Q width, Q height) : bsp_packer_node(v, width, height, 0, 0) {}
		bsp_packer_node(Q width, Q height) : bsp_packer_node(nullptr, width, height, 0, 0) {}
		
		~bsp_packer_node() {
			if (child_a && child_b) {delete child_a; delete child_b;}
		}
		
		Q const width;
		Q const height;
		bsp_coords coords;
		
		union {
			T const * value = nullptr;
			bsp_packer_node * child_a;
		};
		bsp_packer_node * child_b = nullptr;
		
		void pack_iter (bool & ok, T const * const & v, Q const & width, Q const & height, bsp_coords & pos) {
			if ((value && !child_b) || (width > this->width || height > this->height)) { //node already has value or new node is too big to fit
				ok = false;
				return;
			}
			
			if (child_a && child_b) { // if have children, delegate to them
				child_a->pack_iter(ok, v, width, height, pos);
				if (ok) return;
				child_b->pack_iter(ok, v, width, height, pos);
				return;
			}
			
			// have space but no children...
				
			if (width == this->width && height == this->height) { // don't need to make children, we fit the new node perfectly, assume the value and good to go
				value = v;
				ok = true;
				pos = coords;
				return;
			}
			
			if (width != this->width && height != this->height) { // children won't be enough, one of the children needs to have further children
				child_a = new bsp_packer_node { nullptr, width, this->height, coords.x, coords.y };
				child_b = new bsp_packer_node { nullptr, this->width - width, this->height, coords.x + width, coords.y };
				
				child_a->pack_iter(ok, v, width, height, pos);
				assert(ok == true);
				return;
			}
			
			if (width == this->width) { // width matches, single split
				child_a = new bsp_packer_node { v, width, height, coords.x, coords.y };
				child_b = new bsp_packer_node { nullptr, width, this->height - height, coords.x, coords.y + height };
				ok = true;
				pos = child_a->coords;
				return;
			}
			
			if (height == this->height) { // height matches, single split
				child_a = new bsp_packer_node { v, width, height, coords.x, coords.y };
				child_b = new bsp_packer_node { nullptr, this->width - width, this->height, coords.x + width, coords.y };
				ok = true;
				pos = child_a->coords;
				return;
			}
			
			srcthrow("getting here should be impossible");
		}
		
	};
	
	bsp_packer_node * root;
	
};

