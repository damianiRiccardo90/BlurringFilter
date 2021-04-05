#include <fstream> 
#include <stdexcept>

using std::fstream


// Trying to use CRTP and operator overloading helpers to improve performance using only static casting (THEOBOWLYA)(Barton–Nackman trick) FAILED MISERABLY
struct IPixel
{
	friend bool operator == (const IPixel& lhs, const IPixel& rhs);
	friend bool operator != (const IPixel& lhs, const IPixel& rhs);
	IPixel& operator += (const IPixel& rhs);
	friend IPixel& operator + (IPixel& lhs, const IPixel& rhs);
	IPixel& operator -= (const IPixel& rhs);
	friend IPixel& operator - (IPixel& lhs, const IPixel& rhs);
	IPixel& operator *= (const IPixel& rhs);
	friend IPixel& operator * (IPixel& lhs, const IPixel& rhs);

	virtual bool equal_to(const IPixel& rhs) const = 0;
	virtual IPixel& add(const IPixel& rhs) = 0;
	virtual IPixel& subtract(const IPixel& rhs) = 0;
	virtual IPixel& multiply(const IPixel& rhs) = 0;
};

class RGB : public IPixel
{
public:

	RGB();
	RGB(float c);
	RGB(float r, float g, float b);

	bool equal_to(const IPixel& rhs) const override;
	RGB& add(const IPixel& rhs) override;
	RGB& subtract(const IPixel& rhs) override;
	RGB& multiply(const IPixel& rhs) override;

private:

	float red; 
	float green; 
	float blue;
};

// Preset colors
static const RGB BLACK;
static const RGB WHITE;
static const RGB RED;
static const RGB GREEN;
static const RGB BLUE;

class ARGB : public IPixel
{
public:

	ARGB();
	ARGB(float a, float c);
	ARGB(float a, float r, float g, float b);

	bool equal_to(const IPixel& rhs) const override;
	ARGB& add(const IPixel& rhs);
	ARGB& subtract(const IPixel& rhs);
	ARGB& multiply(const IPixel& rhs);

private:

	float alpha;
	float red;
	float green;
	float blue;
};

struct IBlurStrategy
{
	virtual void blur_image(float factor, IPixel* pixels) =0;
};

struct BoxBlur : public IBlurStrategy
{
	void blur_image(float factor, IPixel* pixels) override;
};

struct GaussianBlur : public IBlurStrategy
{
	void blur_image(float factor, IPixel* pixels) override;
};

enum class TGAFormat : uint8_t
{
	FMT_ORIGIN,
	FMT_NEW,
	FMT_NONE
};

enum class TGAImageType : uint8_t
{
	TYPE_COLOR_MAPPED = 0x01,
	TYPE_TRUE_COLOR = 0x02,
	TYPE_BLACK_AND_WHITE = 0x03,
	TYPE_COLOR_MAPPED_RLE = 0x09,
	TYPE_TRUE_COLOR_RLE = 0x0A,
	TYPE_BLACK_AND_WHITE_RLE = 0x0B,
	TYPE_NONE = 0xFF
};

// Based on https://en.wikipedia.org/wiki/Truevision_TGA#Header
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

// Based on https://en.wikipedia.org/wiki/Truevision_TGA#Header
struct TGAData 
{
	uint8_t* image_id = nullptr;
	uint8_t* color_map = nullptr;
	uint8_t* image_data = nullptr;
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

	void parse(const std::string& path);
	void write(const std::string& path);

	void blur(float factor, IBlurStrategy* blur_algo = new BoxBlur);

	static const std::string SIGNATURE;
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

	const uint8_t* in_buffer = nullptr;
	const uint8_t* out_buffer = nullptr;
	int buffer_size;

	TGAFormat format = TGAFormat::FMT_NONE;
	TGAImageType image_type = TGAImageType::TYPE_NONE;

	TGAHeader header;
	TGAFooter footer;
	
	IPixel* pixels = nullptr; // 1D array of pixels
};
