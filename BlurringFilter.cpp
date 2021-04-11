#include "BlurringFilter.h"
#include <format>
#include <string>


RGBA::RGBA() : red(0.f), green(0.f), blue(0.f), alpha(1.f) {}
RGBA::RGBA(float c, float a) : red(c), green(c), blue(c), alpha(a) {}
RGBA::RGBA(float r, float g, float b, float a) : red(r), green(g), blue(b), alpha(a) {}

friend bool operator == (const RGBA& lhs, const RGBA& rhs)
{
	return lhs.red == rhs.red && lhs.green == rhs.green &&
		   lhs.blue == rhs.blue && lhs.alpha == rhs.alpha;
}

friend bool operator != (const RGBA& lhs, const RGBA& rhs)
{
	return !(lhs == rhs);
}

RGBA::RGBA& RGBA::operator += (const RGBA& rhs)
{
	red += rhs.red;
	green += rhs.green;
	blue += rhs.blue;
	return *this;
}

friend RGBA& operator + (RGBA& lhs, const RGBA& rhs)
{
	return lhs += rhs;
}

RGBA::RGBA& RGBA::operator -= (const RGBA& rhs)
{
	red -= rhs.red;
	green -= rhs.green;
	blue -= rhs.blue;
	return *this;
}

friend RGBA& operator - (RGBA& lhs, const RGBA& rhs)
{
	return lhs -= rhs;
}

RGBA::RGBA& RGBA::operator *= (const RGBA& rhs)
{
	red *= rhs.red;
	green *= rhs.green;
	blue *= rhs.blue;
	return *this;
}

friend RGBA& operator * (RGBA& lhs, const RGBA& rhs)
{
	return lhs *= rhs;
}

const RGBA BLACK = RGBA(0, 1);
const RGBA WHITE = RGBA(1, 1);
const RGBA RED   = RGBA(1, 0, 0, 1);
const RGBA GREEN = RGBA(0, 1, 0, 1);
const RGBA BLUE  = RGBA(0, 0, 1, 1);

TGA::TGA(const std::string& path)
{
	parse(path);
}

TGA::~TGA()
{
	// This prevents memory leak on exceptions thrown (and follows RAII)
	delete[] in_buffer;
	delete[] out_buffer;
	delete[] pixels;
}

TGAImageType TGA::get_image_type() const
{
	return static_cast<TGAImageType>(header.image_type);
}

std::string TGA::get_image_type_name() const
{
	switch (header.image_type)
	{
	case TGAImageType::COLOR_MAPPED: 
		return TYPE_COLOR_MAPPED_NAME;
	case TGAImageType::TRUE_COLOR: 
		return TYPE_TRUE_COLOR_NAME;
	case TGAImageType::MONO: 
		return TYPE_BLACK_AND_WHITE_NAME;
	case TGAImageType::COLOR_MAPPED_RLE: 
		return TYPE_COLOR_MAPPED_RLE_NAME;
	case TGAImageType::TRUE_COLOR_RLE: 
		return TYPE_TRUE_COLOR_RLE_NAME;
	case TGAImageType::MONO_RLE: 
		return TYPE_BLACK_AND_WHITE_RLE_NAME;
	case TGAImageType::EMPTY:
	default:
		return "";
	}
}

RGBA* TGA::get_mirror_padded_image(const int pad) const
{
	//TODO Optimize using unsigned int and storing the info already casted in the header struct

	const int image_height = static_cast<int>(header.image_height);
	const int image_width = static_cast<int>(header.image_width);
	if (pad > image_height || pad > image_width)
	{
		throw std::invalid_argument("Pad size cannot exceed the dimensions of the image");
	}

	const int padded_img_height = image_height + 2 * pad;
	const int padded_img_width = image_width + 2 * pad;
	RGBA* padded_img = new RGBA[padded_img_width * padded_img_height];
	if (padded_img && pixels)
	{
		for (int i = 0; i < padded_img_height; i++)
		{
			int mirr_i = 0;
			if (i < pad)
			{
				// Top pad
				mirr_i = pad - i;
			}
			else if (i < pad + image_height)
			{
				// Middle
				mirr_i = i - pad;
			}
			else
			{
				// Bottom pad
				mirr_i = image_height - 1 - (i - (pad + image_height) + 1);
			}
			for (int j = 0; j < padded_img_width; j++)
			{
				int mirr_j = 0;
				if (j < pad)
				{
					// Left pad
					mirr_j = pad - j;
				}
				else if (j < pad + image_width)
				{
					// Middle
					mirr_j = j - pad;
				}
				else
				{
					// Right pad
					mirr_j = image_width - 1 - (j - (pad + image_width) + 1);
				}

				padded_img[i * padded_img_width + j] = pixels[mirr_i * image_width + mirr_j];
			}
		}
	}
	return padded_img;
}

void TGA::parse(const std::string& path)
{
	std::ifstream ifs(path, std::ios::binary | std::ios::ate);
	if (ifs.fail())
	{
		throw std::ios_base::failure("Unable to open file for reading");
	}
	else
	{
		//TODO Check file extension

		// The file is open with the ios::ate flag, so this call will directly obtain the size of the file
		buffer_size = ifs.tellg();
		// We can now use the size to allocate a buffer into which we'll store the file data
		in_buffer = new uint8_t[buffer_size];
		ifs.seekg(0, ios::beg);
		ifs.read(in_buffer, buffer_size);
		ifs.close();

		// The order of these 3 function calls is mandatory
		parse_footer();
		parse_header();
		parse_data();

		delete[] in_buffer;
	}
}

void TGA::write(const std::string& path)
{
	std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
	if (ofs.fail())
	{
		throw std::ios_base::failure("Unable to open file for writing");
	}
	else
	{
		out_buffer = new uint8_t[buffer_size];

		// The order of these 3 function calls is NOT mandatory
		write_header();
		write_data();
		write_footer();

		ofs.write(out_buffer, buffer_size);
		ofs.close();

		delete[] out_buffer;
	}
}

// Blur the image using box blur algorithm (with precomputed SAT optimization)
void TGA::blur(float factor)
{
	if (factor < 0.f || factor > 1.f)
	{
		throw std::invalid_argument("Invalid blur factor (it needs to be in the 0 < f < 1 range)");
	}

	const int image_height = static_cast<int>(header.image_height);
	const int image_width = static_cast<int>(header.image_width);
	
	// Linear interpolation between RANGE1(factor): 0 < x < 1 and RANGE2(kernel diameter): 
	// 0 < y < min(image_width, image_height)
	int kernel_size_range = min(image_height, image_width);
	int kernel_size = static_cast<int>(round(kernel_size_range * factor));

	if (kernel_size % 2 == 0)
	{
		// Having only odd values is required
		kernel_size--;
	}

	if (kernel_size <= 0)
	{
		return;
	}

	const int pad = ceil(kernel_size / 2);
	RGBA* padded_img = get_mirror_padded_image(pad);
	if (padded_img && pixels)
	{
		// Summed Area Table algorithm (using dynamic programming)
		const int padded_img_height = image_height + 2 * pad;
		const int padded_img_width = image_width + 2 * pad;
		for (int i = 0; i < padded_img_height; i++)
		{
			for (int j = 0; j < padded_img_width; j++)
			{
				if (i > 0 && j > 0)
				{
					padded_img[i * padded_img_width + j] = padded_img[i * padded_img_width + j] +
														   padded_img[(i - 1) * padded_img_width + j] +
														   padded_img[i * padded_img_width + (j - 1)] -
														   padded_img[(i - 1) * padded_img_width + (j - 1)];
				}
				else if (i > 0 && j == 0)
				{
					padded_img[i * padded_img_width + j] = padded_img[i * padded_img_width + j] +
														   padded_img[(i - 1) * padded_img_width + j];
				}
				else if (j > 0 && i == 0)
				{
					padded_img[i * padded_img_width + j] = padded_img[i * padded_img_width + j] +
														   padded_img[i * padded_img_width + (j - 1)];
				}
				else
				{
					padded_img[i * padded_img_width + j] = padded_img[i * padded_img_width + j];
				}
			}
		}

		// Filtering the image using a constant value (of 1) kernel
		for (int i = 0; i < image_width; i++)
		{
			for (int j = 0; j < image_height; j++)
			{
				pixels[i * image_width + j] = ;
			}
		}
	}
}

const std::string TGA::SIGNATURE                     = "TRUEVISION-XFILE";
const int TGA::SIGNATURE_SIZE                        = 16;
const std::string TGA::TYPE_COLOR_MAPPED_NAME        = "Color mapped";
const std::string TGA::TYPE_TRUE_COLOR_NAME          = "True color";
const std::string TGA::TYPE_BLACK_AND_WHITE_NAME     = "Black and white";
const std::string TGA::TYPE_COLOR_MAPPED_RLE_NAME    = "Color mapped run-length encoded";
const std::string TGA::TYPE_TRUE_COLOR_RLE_NAME      = "True color run-length encoded";
const std::string TGA::TYPE_BLACK_AND_WHITE_RLE_NAME = "Black and white run-length encoded";

void TGA::parse_header()
{
	//TODO Control if the code is robust enough for the errors that could arise
	if (in_buffer)
	{
		header.id_length            = in_buffer[0];
		header.color_map_type       = in_buffer[1];
		header.image_type           = in_buffer[2];
		header.first_entry_index    = static_cast<uint16_t>(in_buffer[3]) | static_cast<uint16_t>(in_buffer[4] << 8);
		header.color_map_length     = static_cast<uint16_t>(in_buffer[5]) | static_cast<uint16_t>(in_buffer[6] << 8);
		header.color_map_entry_size = in_buffer[7];
		header.x_origin             = static_cast<uint16_t>(in_buffer[8]) | static_cast<uint16_t>(in_buffer[9] << 8);
		header.y_origin             = static_cast<uint16_t>(in_buffer[10]) | static_cast<uint16_t>(in_buffer[11] << 8);
		header.image_width          = static_cast<uint16_t>(in_buffer[12]) | static_cast<uint16_t>(in_buffer[13] << 8);
		header.image_height         = static_cast<uint16_t>(in_buffer[14]) | static_cast<uint16_t>(in_buffer[15] << 8);
		header.pixel_depth          = in_buffer[16];
		header.image_descriptor     = in_buffer[17];
	}

	image_type = static_cast<TGAImageType>(header.image_type);
	horiz_orient = (header.image_descriptor & 0x10) == 0 ? TGAHorizOrientation::RIGHT_TO_LEFT : TGAHorizOrientation::LEFT_TO_RIGHT;
	vert_orient = (header.image_descriptor & 0x20) == 0 ? TGAVertOrientation::BOTTOM_UP : TGAVertOrientation::TOP_DOWN;

	// Error checking
	if (get_image_type() == TGAImageType::EMPTY)
	{
		throw std::domain_error("Empty image type, cannot complete read operation");
	}
	if ((get_image_type() == TGAImageType::COLOR_MAPPED || get_image_type() == TGAImageType::COLOR_MAPPED_RLE) && header.color_map_type == 0)
	{
		throw std::domain_error("Missing color map data, cannot complete read operation");
	}
	if (header.image_width == 0 || header.image_height == 0)
	{
		throw std::domain_error("Invalid pixel height/weight count, cannot complete read operation");
	}
	if (header.pixel_depth != 8 && header.pixel_depth != 16 && header.pixel_depth != 24 && header.pixel_depth != 32)
	{
		throw std::domain_error("Invalid pixel depth value, cannot complete read operation");
	}
	if (header.pixel_depth != 24 && header.pixel_depth != 32)
	{
		throw std::domain_error(std::format("{d}bit pixel depth images are not currently supported", header.pixel_depth));
	}
	if (get_image_type() != TGAImageType::TRUE_COLOR /*TODO&& get_image_type() != TGAImageType::TRUE_COLOR_RLE*/)
	{
		throw std::domain_error(std::format("{} image type is not currently supported", get_image_type_name()));
	}
}

void TGA::parse_data()
{
	//TODO Control if the code is robust enough for the errors that could arise

	// Computing the offset (from the start of the file) to the first byte of image data
	int start_offset = 18; // Starting from the first byte after the header
	start_offset += static_cast<int>(header.id_length); // Skipping image id field
	if (header.color_map_type != 0) // Skipping color map data field
	{
		// When need to ceil to get the right amount of bytes because the number of bits may be 15
		start_offset += static_cast<int>(header.color_map_length) * ceil(static_cast<int>(header.color_map_entry_size) / 8);
	}

	if (get_image_type() == TGAImageType::TRUE_COLOR)
	{
		const int image_width = static_cast<int>(header.image_width);
		const int image_height = static_cast<int>(header.image_height);
		const int bytes_per_pixel = header.pixel_depth / 8;

		pixels = new RGBA[image_width * image_height];

		int i = vert_orient == TGAVertOrientation::TOP_DOWN ? 0 : image_height - 1;
		while (i != vert_orient == TGAVertOrientation::TOP_DOWN ? image_height : -1)
		{
			int j = horiz_orient == TGAHorizOrientation::LEFT_TO_RIGHT ? 0 : image_width - 1;
			while (j != horiz_orient == TGAHorizOrientation::LEFT_TO_RIGHT ? image_width : -1)
			{
				const int curr_pixel_offset = start_offset + (i * image_width + j) * bytes_per_pixel;
				if (pixels)
				{
					// The order in which the color bytes are displaced is BGRA TODO ARE YOU SURE??
					pixels[i * image_width + j] = RGBA(
						(curr_pixel_offset + 2) / 255.f,
						(curr_pixel_offset + 1) / 255.f,
						curr_pixel_offset / 255.f,
						bytes_per_pixel == 4 ? (curr_pixel_offset + 3) / 255.f : 1.f
					);
				}

				horiz_orient == TGAHorizOrientation::LEFT_TO_RIGHT ? j++ : j--;
			}

			vert_orient == TGAVertOrientation::TOP_DOWN ? i++ : i--;
		}
	}
	/* TODO
	else if (get_image_type() == TGAImageType::TRUE_COLOR_RLE)
	{

	}
	*/
}

void TGA::parse_footer()
{
	//TODO Control if the code is robust enough for the errors that could arise
	if (in_buffer)
	{
		footer.signature = std::string(static_cast<const char*>(in_buffer[buffer_size - 18]), SIGNATURE_SIZE);
		format = strcmp(footer.signature, SIGNATURE) ? TGAFormat::NEW : TGAFormat::ORIGIN;
		if (format == TGAFormat::NEW)
		{
			footer.ext_area_offset = static_cast<uint32_t>(in_buffer[buffer_size - 26])
				                   | static_cast<uint32_t>(in_buffer[buffer_size - 25] << 8)
				                   | static_cast<uint32_t>(in_buffer[buffer_size - 24] << 16)
				                   | static_cast<uint32_t>(in_buffer[buffer_size - 23] << 24);
			footer.dev_dir_offset  = static_cast<uint32_t>(in_buffer[buffer_size - 22])
				                   | static_cast<uint32_t>(in_buffer[buffer_size - 21] << 8)
				                   | static_cast<uint32_t>(in_buffer[buffer_size - 20] << 16)
				                   | static_cast<uint32_t>(in_buffer[buffer_size - 19] << 24);
		}
	}

	if ()
	{
		throw std::domain_error("MEGA FAGGOTTO");
	}
}

void TGA::write_header()
{
	//TODO Control if the code is robust enough for the errors that could arise
	if (out_buffer)
	{
		out_buffer[0] = header.id_length;
		if (get_image_type() == TGAImageType::TRUE_COLOR || get_image_type() == TGAImageType::TRUE_COLOR_RLE)
		{
			out_buffer[1] = 0x00; // Setting this to zero to ensure compatibility
			out_buffer[3] = 0x00;
			out_buffer[4] = 0x00;
			out_buffer[5] = 0x00;
			out_buffer[6] = 0x00;
			out_buffer[7] = 0x00;
		}
		else
		{
			out_buffer[1] = header.color_map_type;
			out_buffer[3] = static_cast<uint8_t>(header.first_entry_index & 0x00FF);
			out_buffer[4] = static_cast<uint8_t>((header.first_entry_index >> 8) & 0x00FF);
			out_buffer[5] = static_cast<uint8_t>(header.color_map_length & 0x00FF);
			out_buffer[6] = static_cast<uint8_t>((header.color_map_length >> 8) & 0x00FF);
			out_buffer[7] = header.color_map_entry_size;
		}
		out_buffer[2]  = header.image_type;
		out_buffer[8]  = static_cast<uint8_t>(header.x_origin & 0x00FF);
		out_buffer[9]  = static_cast<uint8_t>((header.x_origin >> 8) & 0x00FF);
		out_buffer[10] = static_cast<uint8_t>(header.y_origin & 0x00FF);
		out_buffer[11] = static_cast<uint8_t>((header.y_origin >> 8) & 0x00FF);
		out_buffer[12] = static_cast<uint8_t>(header.image_width & 0x00FF);
		out_buffer[13] = static_cast<uint8_t>((header.image_width >> 8) & 0x00FF);
		out_buffer[14] = static_cast<uint8_t>(header.image_height & 0x00FF);
		out_buffer[15] = static_cast<uint8_t>((header.image_height >> 8) & 0x00FF);
		out_buffer[16] = header.pixel_depth;
		out_buffer[17] = header.image_descriptor;
	}
}

void TGA::write_data()
{
	//TODO Control how to handle alpha byte (or bits)
	//TODO Control if the code is robust enough for the errors that could arise

	// Computing the offset (from the start of the file) to the first byte of image data
	int start_offset = 18; // Starting from the first byte after the header
	start_offset += static_cast<int>(header.id_length); // Skipping image id field
	if (header.color_map_type != 0) // Skipping color map data field
	{
		// When need to ceil to get the right amount of bytes because the number of bits may be 15
		start_offset += static_cast<int>(header.color_map_length) * ceil(static_cast<int>(header.color_map_entry_size) / 8);
	}

	if (get_image_type() == TGAImageType::TRUE_COLOR)
	{
		const int image_width = static_cast<int>(header.image_width);
		const int image_height = static_cast<int>(header.image_height);
		const int bytes_per_pixel = header.pixel_depth / 8;

		int i = vert_orient == TGAVertOrientation::TOP_DOWN ? 0 : image_height - 1;
		while (i != vert_orient == TGAVertOrientation::TOP_DOWN ? image_height : -1)
		{
			int j = horiz_orient == TGAHorizOrientation::LEFT_TO_RIGHT ? 0 : image_width - 1;
			while (j != horiz_orient == TGAHorizOrientation::LEFT_TO_RIGHT ? image_width : -1)
			{
				const int curr_pixel_offset = start_offset + (i * image_width + j) * bytes_per_pixel;
				if (out_buffer && pixels)
				{
					// The order in which the color bytes are displaced is BGRA TODO ARE YOU SURE??
					out_buffer[curr_pixel_offset] = static_cast<uint8_t>(std::min(1.f, pixels[i * image_width + j].blue) * 255.f);
					out_buffer[curr_pixel_offset + 1] = static_cast<uint8_t>(std::min(1.f, pixels[i * image_width + j].green) * 255.f);
					out_buffer[curr_pixel_offset + 2] = static_cast<uint8_t>(std::min(1.f, pixels[i * image_width + j].red) * 255.f);
					if (bytes_per_pixel == 4)
					{
						out_buffer[curr_pixel_offset + 3] = static_cast<uint8_t>(std::min(1.f, pixels[i * image_width + j].alpha) * 255.f);
					}
					else
					{
						out_buffer[curr_pixel_offset + 3] = 0xFF;
					}
				}

				horiz_orient == TGAHorizOrientation::LEFT_TO_RIGHT ? j++ : j--;
			}

			vert_orient == TGAVertOrientation::TOP_DOWN ? i++ : i--;
		}
	}
	/* TODO
	else if (get_image_type() == TGAImageType::TRUE_COLOR_RLE)
	{

	}
	*/
}

void TGA::write_footer()
{
	//TODO Control if the code is robust enough for the errors that could arise
	if (format == TGAFormat::NEW)
	{
		if (out_buffer)
		{
			out_buffer[buffer_size - 26] = static_cast<uint8_t>(footer.ext_area_offset & 0x000000FF);
			out_buffer[buffer_size - 25] = static_cast<uint8_t>((footer.ext_area_offset >> 8) & 0x000000FF);
			out_buffer[buffer_size - 24] = static_cast<uint8_t>((footer.ext_area_offset >> 16) & 0x000000FF);
			out_buffer[buffer_size - 23] = static_cast<uint8_t>((footer.ext_area_offset >> 24) & 0x000000FF);
			out_buffer[buffer_size - 22] = static_cast<uint8_t>(footer.dev_dir_offset & 0x000000FF);
			out_buffer[buffer_size - 21] = static_cast<uint8_t>((footer.dev_dir_offset >> 8) & 0x000000FF);
			out_buffer[buffer_size - 20] = static_cast<uint8_t>((footer.dev_dir_offset >> 16) & 0x000000FF);
			out_buffer[buffer_size - 19] = static_cast<uint8_t>((footer.dev_dir_offset >> 24) & 0x000000FF);
			for (int i = 0, int j = buffer_size - 18; i < SIGNATURE_SIZE; i++, j++)
			{
				out_buffer[j] = static_cast<uint8_t>(footer.signature[i]);
			}
			out_buffer[buffer_size - 2] = '.';
			out_buffer[buffer_size - 1] = 0x00;
		}
	}
}


int main(int argc, char **argv)
{
	//TODO Add option parameters and error checking for them as well
	try
	{
		TGA* img = new TGA(/*ROBO BITCH*/);
		return 0;

		// MEGA FAGGOT
	}
	catch (std::exception &e)
	{
		std::cerr << "Caught: " << e.what() << std::endl;
		std::cerr << "Type: " << typeid(e).name() << std::endl;

		return 1;
	};
}
