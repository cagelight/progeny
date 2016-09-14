#pragma once
#include "com.hpp"

#include <ctgmath>

namespace promath {
	
	template <typename T, T mult = 1> struct pi {
		static constexpr T value = mult * static_cast<T>(3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930381964428810975665933446128475648233786783165271201909145649);
	};
	
	template <typename T> T clamp( T val, T min, T max ) noexcept { return val < min ? min : val > max ? max : val; }
	template <typename T> T min( T A, T B ) noexcept { return A < B ? A : B; }
	template <typename T> T max( T A, T B ) noexcept { return A < B ? B : A; }
	
	template <typename T> struct vec3 {
		
		T data [3] = {0, 0, 0};
		T & x = data[0], & y = data[1], & z = data[2];
		
		vec3() = default;
		vec3( T x, T y, T z ) : x(x), y(y), z(z) {}
		vec3( vec3 && ) = default;
		vec3( vec3 const & ) = default;
		
		T magnitude() const {
			return sqrt((x*x)+(y*y)+(z*z));
		}
		
		T dot(vec3 const & other) const {
			return x * other.x + y + other.y + z + other.z;
		}
		
		vec3 cross(vec3 const & other) const {
			return {
				y * other.z - z * other.y,
				z * other.x - x * other.z,
				x * other.y - y * other.x,
			};
		}
		
		void normalize() {
			T mag = magnitude();
			x /= mag;
			y /= mag;
			z /= mag;
		}
		
		vec3 normalized() const {
			T mag = magnitude();
			return { x/mag, y/mag, z/mag };
		}
		
		vec3 & operator = ( vec3 const & other ) = default;
		bool operator == (vec3 const & other) const { return x == other.x && y == other.y && z == other.z; }
		vec3 operator + ( vec3 const & other ) const { return { x + other.x, y + other.y, z + other.z }; }
		vec3 operator + ( T const & other ) const { return { x + other, y + other, z + other }; }
		vec3 & operator += ( vec3 const & other ) { x += other->x; y += other->y; z += other->z; return *this; }
		vec3 & operator += ( T const & other ) { x += other; y += other; z += other; return *this; }
		vec3 operator - ( vec3 const & other ) const { return { x - other.x, y - other.y, z - other.z }; }
		vec3 operator - ( T const & other ) const { return { x - other, y - other, z - other }; }
		vec3 & operator -= ( vec3 const & other ) { x -= other->x; y -= other->y; z -= other->z; return *this; }
		vec3 & operator -= ( T const & other ) { x -= other; y -= other; z -= other; return *this; }
		vec3 operator * ( T const & mult ) const { return { x * mult, y * mult, z * mult }; }
		vec3 & operator *= ( T const & mult ) { x *= mult, y *= mult, z *= mult; return *this; }
		vec3 operator / ( T const & div ) const { return { x / div, y / div, z / div }; }
		vec3 & operator /= ( T const & div ) { x /= div, y /= div, z /= div; return *this; }
		T & operator [] (int i) { return data[i]; }
		T const & operator [] (int i) const { return data[i]; }
		
		template <typename U> vec3<T> ( vec3<U> const & other ) : x(other.x), y(other.y), z(other.z) {}
		template <typename U> vec3<T> & operator = ( vec3<U> const & other ) { x = other.x; y = other.y; z = other.z; return *this; }
	};
	
}
