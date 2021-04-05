#include "BlurringFilter.h"
#include <format>
#include <string>


bool operator == (const IPixel& lhs, const IPixel& rhs) 
{ 
	return lhs.equal_to(rhs); 
}

bool operator != (const IPixel& lhs, const IPixel& rhs) 
{ 
	return !(lhs == rhs); 
}

IPixel& IPixel::operator += (const IPixel& rhs) 
{ 
	return this->add(rhs); 
}

IPixel& operator + (IPixel& lhs, const IPixel& rhs) 
{ 
	return lhs += rhs; 
}

IPixel& IPixel::operator -= (const IPixel& rhs) 
{ 
	return this->subtract(rhs); 
}

IPixel& operator - (IPixel& lhs, const IPixel& rhs) 
{ 
	return lhs -= rhs; 
}

IPixel& IPixel::operator *= (const IPixel& rhs) 
{ 
	return this->multiply(rhs); 
}

IPixel& operator * (IPixel& lhs, const IPixel& rhs) 
{ 
	return lhs *= rhs; 
}

RGB::RGB() : red(0.f), green(0.f), blue(0.f) {}
RGB::RGB(float c) : red(c), green(c), blue(c) {}
RGB::RGB(float r, float g, float b) : red(r), green(g), blue(b) {}

bool RGB::equal_to(const IPixel& rhs) const
{
	const RGB& casted_rhs = static_cast<const RGB&>(rhs);
	return red == casted_rhs.red && green == casted_rhs.green && blue == casted_rhs.blue;
}

RGB& RGB::add(const IPixel& rhs)
{
	const RGB& casted_rhs = static_cast<const RGB&>(rhs);
	red += casted_rhs.red;
	green += casted_rhs.green;
	blue += casted_rhs.blue;
	return *this;
}

RGB& RGB::subtract(const IPixel& rhs)
{
	const RGB& casted_rhs = static_cast<const RGB&>(rhs);
	red -= casted_rhs.red;
	green -= casted_rhs.green;
	blue -= casted_rhs.blue;
	return *this;
}

RGB& RGB::multiply(const IPixel& rhs)
{
	const RGB& casted_rhs = static_cast<const RGB&>(rhs);
	red *= casted_rhs.red;
	green *= casted_rhs.green;
	blue *= casted_rhs.blue;
	return *this;
}

const RGB BLACK = RGB(0);
const RGB WHITE = RGB(1);
const RGB RED   = RGB(1, 0, 0);
const RGB GREEN = RGB(0, 1, 0);
const RGB BLUE  = RGB(0, 0, 1);

ARGB::ARGB() : alpha(0.f), r(0.f), g(0.f), b(0.f) {}
ARGB::ARGB(float a, float c) : alpha(a), red(c), green(c), blue(c) {}
ARGB::ARGB(float a, float r, float g, float b) : alpha(a), red(r), green(g), blue(b) {}

bool ARGB::equal_to(const IPixel& rhs) const
{
	const ARGB& casted_rhs = static_cast<const ARGB&>(rhs);
	return alpha == casted_rhs.alpha && red == casted_rhs.red &&
		   green == casted_rhs.green && blue == casted_rhs.blue;
}

ARGB& ARGB::add(const IPixel& rhs)
{
	const ARGB& casted_rhs = static_cast<const ARGB&>(rhs);
	red += casted_rhs.red;
	green += casted_rhs.green;
	blue += casted_rhs.blue;
	return *this;
}

ARGB& ARGB::subtract(const IPixel& rhs)
{
	const ARGB& casted_rhs = static_cast<const ARGB&>(rhs);
	red -= casted_rhs.red;
	green -= casted_rhs.green;
	blue -= casted_rhs.blue;
	return *this;
}

ARGB& ARGB::multiply(const IPixel& rhs)
{
	const ARGB& casted_rhs = static_cast<const ARGB&>(rhs);
	red *= casted_rhs.red;
	green *= casted_rhs.green;
	blue *= casted_rhs.blue;
	return *this;
}

void BoxBlur::blur_image(float factor, IPixel* pixels)
{
	//TODO Implement BoxBlur::blur_image
}

void GaussianBlur::blur_image(float factor, IPixel* pixels)
{
	//TODO Implement GaussianBlur::blur_image
}

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
	case TGAImageType::TYPE_COLOR_MAPPED: 
		return TYPE_COLOR_MAPPED_NAME;
	case TGAImageType::TYPE_TRUE_COLOR: 
		return TYPE_TRUE_COLOR_NAME;
	case TGAImageType::TYPE_BLACK_AND_WHITE: 
		return TYPE_BLACK_AND_WHITE_NAME;
	case TGAImageType::TYPE_COLOR_MAPPED_RLE: 
		return TYPE_COLOR_MAPPED_RLE_NAME;
	case TGAImageType::TYPE_TRUE_COLOR_RLE: 
		return TYPE_TRUE_COLOR_RLE_NAME;
	case TGAImageType::TYPE_BLACK_AND_WHITE_RLE: 
		return TYPE_BLACK_AND_WHITE_RLE_NAME;
	case TGAImageType::TYPE_NONE:
	default:
		return "";
	}
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

void TGA::blur(float factor, IBlurStrategy* blur_algo)
{
	if (blur_algo)
	{
		blur_algo->blur_image(factor, pixels);
	}
}

const std::string TGA::SIGNATURE                     = "TRUEVISION-XFILE";
const std::string TGA::TYPE_COLOR_MAPPED_NAME        = "Color mapped";
const std::string TGA::TYPE_TRUE_COLOR_NAME          = "True color";
const std::string TGA::TYPE_BLACK_AND_WHITE_NAME     = "Black and white";
const std::string TGA::TYPE_COLOR_MAPPED_RLE_NAME    = "Color mapped run-length encoded";
const std::string TGA::TYPE_TRUE_COLOR_RLE_NAME      = "True color run-length encoded";
const std::string TGA::TYPE_BLACK_AND_WHITE_RLE_NAME = "Black and white run-length encoded";

void TGA::parse_header()
{
	//TODO Control if the code is robust enough for the errors that could arise
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

	image_type = header.image_type;

	if (header.pixel_depth != 24 && header.pixel_depth != 32)
	{
		throw std::domain_error(std::format("{d}bit pixel depth images are not supported", header.pixel_depth));
	}
	if (get_image_type() != TGAImageType::TYPE_TRUE_COLOR /*TODO&& get_image_type() != TGAImageType::TYPE_TRUE_COLOR_RLE*/)
	{
		throw std::domain_error(std::format("{} image type is not supported", get_image_type_name()));
	}
}

void TGA::parse_data()
{
	//TODO Control if the code is robust enough for the errors that could arise
	if (get_image_type() == TGAImageType::TYPE_TRUE_COLOR)
	{

	}
	/* TODO
	else if (get_image_type() == TGAImageType::TYPE_TRUE_COLOR_RLE)
	{

	}
	*/
}

void TGA::parse_footer()
{
	//TODO Control if the code is robust enough for the errors that could arise
	footer.signature = in_buffer[buffer_size - 18];
	format = strcmp(footer.signature, SIGNATURE) ? TGAFormat::FMT_NEW : TGAFormat::FMT_ORIGIN;
	if (format == TGAFormat::FMT_NEW)
	{
		footer.ext_area_offset = static_cast<uint32_t>(in_buffer[buffer_size - 26])
							   | static_cast<uint32_t>(in_buffer[buffer_size - 25] << 8)
							   | static_cast<uint32_t>(in_buffer[buffer_size - 24] << 16)
							   | static_cast<uint32_t>(in_buffer[buffer_size - 23] << 24);
		footer.dev_dir_offset = static_cast<uint32_t>(in_buffer[buffer_size - 22])
							   | static_cast<uint32_t>(in_buffer[buffer_size - 21] << 8)
							   | static_cast<uint32_t>(in_buffer[buffer_size - 20] << 16)
							   | static_cast<uint32_t>(in_buffer[buffer_size - 19] << 24);
	}

	if ()
	{
		throw std::domain_error("MEGA FAGGOTTO");
	}
}

void TGA::write_header()
{
	//TODO Control if the code is robust enough for the errors that could arise
}

void TGA::write_data()
{
	//TODO Control if the code is robust enough for the errors that could arise
}

void TGA::write_footer()
{
	//TODO Control if the code is robust enough for the errors that could arise
	if (format == TGAFormat::FMT_NEW)
	{
		// Write footer
	}
}


int main(int argc, char **argv)
{
	//TODO Add option parameters and error checking for them as well
	try
	{
		Image *img = new Image(512, 512);
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
