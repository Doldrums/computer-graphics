#pragma once

#include "utils/error_handler.h"

#include <algorithm>
#include <linalg.h>
#include <vector>


using namespace linalg::aliases;

namespace cg
{
	template<typename T>
	class resource
	{
	public:
		resource(size_t size);
		resource(size_t x_size, size_t y_size);
		~resource();

		const T* get_data();
		T& item(size_t item);
		T& item(size_t x, size_t y);

		size_t get_size_in_bytes() const;
		size_t get_number_of_elements() const;
		size_t get_stride() const;

	private:
		std::vector<T> data;
		size_t item_size = sizeof(T);
		size_t stride;
	};

	template<typename T>
	inline resource<T>::resource(size_t size)
	{
		this->data.resize(size);
		this->stride = size;
	}
	template<typename T>
	inline resource<T>::resource(size_t x_size, size_t y_size)
	{
		this->data.resize(x_size * y_size);
		this->stride = x_size;
	}
	template<typename T>
	inline resource<T>::~resource()
	{
	}

	template<typename T>
	inline const T* resource<T>::get_data()
	{
		return this->data.data();
	}
	template<typename T>
	inline T& resource<T>::item(size_t item)
	{
		return this->data.at(item);
	}
	template<typename T>
	inline T& resource<T>::item(size_t x, size_t y)
	{
		return this->data.at(y * get_stride() + x);
	}
	template<typename T>
	inline size_t resource<T>::get_size_in_bytes() const
	{
		return item_size * this->data.size();
	}
	template<typename T>
	inline size_t resource<T>::get_number_of_elements() const
	{
		return this->data.size();
	}

	template<typename T>
	inline size_t resource<T>::get_stride() const
	{
		return this->stride;
	}

	struct color
	{
		static color from_float3(const float3& in)
		{
			return color{
					.r = in.x,
					.g = in.y,
					.b = in.z,
			};
		};
		float3 to_float3() const
		{
			return float3{
					r,
					g,
					b,
			};
		}
		float r;
		float g;
		float b;
	};

	struct unsigned_color
	{
		static unsigned_color from_color(const color& color)
		{
			return from_float3(color.to_float3());
		};
		static unsigned_color from_float3(const float3& color)
		{
			return unsigned_color{
					.r = static_cast<uint8_t>(std::clamp(color.x * 255.f, 0.f, 255.f)),
					.g = static_cast<uint8_t>(std::clamp(color.y * 255.f, 0.f, 255.f)),
					.b = static_cast<uint8_t>(std::clamp(color.z * 255.f, 0.f, 255.f)),
			};
		};
		float3 to_float3() const
		{
			return float3{
					static_cast<float>(r) / 255.f,
					static_cast<float>(g) / 255.f,
					static_cast<float>(b) / 255.f,
			};
		};
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};

	struct vertex
	{
		float x;
		float y;
		float z;
		float nx;
		float ny;
		float nz;
		float tx;
		float ty;
		float ambient_r;
		float ambient_g;
		float ambient_b;
		float diffuse_r;
		float diffuse_g;
		float diffuse_b;
		float emissive_r;
		float emissive_g;
		float emissive_b;
	};

}// namespace cg
