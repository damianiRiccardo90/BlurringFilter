#pragma once

#include <stdint.h>
#include <string>


struct RGBA
{
	RGBA();
	RGBA(float c, float a);
	RGBA(float r, float g, float b, float a);

	friend bool operator == (const RGBA& lhs, const RGBA& rhs);
	friend bool operator != (const RGBA& lhs, const RGBA& rhs);
	RGBA& operator += (const RGBA& rhs);
	friend RGBA& operator + (RGBA& lhs, const RGBA& rhs);
	RGBA& operator -= (const RGBA& rhs);
	friend RGBA& operator - (RGBA& lhs, const RGBA& rhs);
	RGBA& operator *= (const RGBA& rhs);
	friend RGBA& operator * (RGBA& lhs, const RGBA& rhs);
	RGBA& operator /= (const RGBA& rhs);
	friend RGBA& operator / (RGBA& lhs, const RGBA& rhs);

	float red; 
	float green; 
	float blue;
	float alpha;
};

enum class TGAFormat : uint8_t
{
	ORIGIN,
	NEW,
	NONE
};

enum class TGAImageType : uint8_t
{
	EMPTY = 0x00,
	COLOR_MAPPED = 0x01,
	TRUE_COLOR = 0x02,
	MONO = 0x03,
	COLOR_MAPPED_RLE = 0x09,
	TRUE_COLOR_RLE = 0x0A,
	MONO_RLE = 0x0B
};

enum class TGAHorizOrientation : uint8_t
{
	LEFT_TO_RIGHT,
	RIGHT_TO_LEFT,
	NONE
};

enum class TGAVertOrientation : uint8_t
{
	TOP_DOWN,
	BOTTOM_UP,
	NONE
};

struct TGAHeader
{
	uint8_t  id_length;
	uint8_t  color_map_type;
	uint8_t  image_type;

	// Color map specification fields
	uint16_t first_entry_index;
	uint16_t color_map_length;
	uint8_t  color_map_entry_size;

	// Image specification fields
	uint16_t x_origin;
	uint16_t y_origin;
	uint16_t image_width;
	uint16_t image_height;
	uint8_t  pixel_depth;
	uint8_t  image_descriptor;
};

struct TGAFooter
{
	uint32_t ext_area_offset;
	uint32_t dev_dir_offset;
	std::string signature;
};

class TGA
{
public:

	TGA(const std::string& path);
	~TGA();

	TGAImageType get_image_type() const;
	std::string get_image_type_name() const;
	RGBA* get_mirror_padded_image(const int pad) const;

	void parse(const std::string& path);
	void write(const std::string& path);

	void blur(float factor);

	static const std::string SIGNATURE;
	static const int SIGNATURE_SIZE;
	static const std::string TYPE_COLOR_MAPPED_NAME;
	static const std::string TYPE_TRUE_COLOR_NAME;
	static const std::string TYPE_BLACK_AND_WHITE_NAME;
	static const std::string TYPE_COLOR_MAPPED_RLE_NAME;
	static const std::string TYPE_TRUE_COLOR_RLE_NAME;
	static const std::string TYPE_BLACK_AND_WHITE_RLE_NAME;

private:

	void parse_header();
	void parse_data();
	void parse_footer();
	void write_header();
	void write_data();
	void write_footer();

	uint8_t* in_buffer = nullptr;
	uint8_t* out_buffer = nullptr;
	int buffer_size = 0;

	TGAFormat format = TGAFormat::NONE;
	TGAImageType image_type = TGAImageType::EMPTY;
	TGAHorizOrientation horiz_orient = TGAHorizOrientation::NONE;
	TGAVertOrientation vert_orient = TGAVertOrientation::NONE;

	TGAHeader header;
	TGAFooter footer;
	
	RGBA* pixels = nullptr;
};
