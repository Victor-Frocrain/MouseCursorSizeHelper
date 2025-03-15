/*
 * This file is part of the MouseCursorSizeHelper project.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "MouseCursorSizeHelper.h"

#include <stdint.h>
#include <cmath>

/**
 * Get the real current mouse cursor size with scales.
 *
 * @return The pair of the real mouse cursor width and height.
 */
std::pair<float, float> MouseCursorSizeHelper::GetCurrentMouseCursorSize()
{
	SIZEDATA SizeData;
	SizeData.width = DEFAULT_IMAGE_CURSOR_SIZE;
	SizeData.height = DEFAULT_IMAGE_CURSOR_SIZE;
	SizeData.isRealSize = false;

	std::vector<uint32_t> PixelArray = GetPixelArrayOfCurrentMouseImage(&SizeData);

	// Compute the origin real size of mouse cursor
	std::pair<float, float> CursorSize = ComputeCursorSizeFromPixelArray(PixelArray, SizeData);

	if (!SizeData.isRealSize)
	{
		// Scale mouse cursor size by DPI
		ScaleCursorSizeByDPI(&CursorSize);

		// Scale mouse cursor size by defined system mouse size
		ScaleCursorSizeByMouseSystemScale(&CursorSize);
	}

	// Ceil mouse cursor size
	CeilPair(&CursorSize);

	return CursorSize;
}

/**
 * Initialize FirstLastIndexes structure.
 *
 * @return The initialized FirstLastIndexes structure.
 */
MouseCursorSizeHelper::FIRSTLASTINDEXES MouseCursorSizeHelper::InitFirstLastIndexesStruct()
{
	FIRSTLASTINDEXES IndexesStruct;

	IndexesStruct.lastYValue = -1; // Negative value to take first line in account for height calculation
	IndexesStruct.firstIndexHeight = 0;
	IndexesStruct.lastIndexHeight = 0;
	IndexesStruct.firstIndexWidth = 0;
	IndexesStruct.lastIndexWidth = 0;
	IndexesStruct.isFirstIndexHeightFound = false;
	IndexesStruct.isFirstIndexWidthFound = false;

	return IndexesStruct;
}

/**
 * Get the index of the smallest frame in array of pictures.
 *
 * @param Pictures the array of pictures.
 * @return The index of the smallest frame in array of pictures.
 */
int MouseCursorSizeHelper::GetIndexOfSmallestPicture(const std::vector<ICONDIRENTRY>& Pictures)
{
	int Index = -1;
	uint64_t ImageSize = 0xffffffffffffffff; // The MAX value of uint64_t

	for (int i = 0; i < Pictures.size(); i++)
	{
		uint64_t EntrySize = uint64_t(Pictures[i].bWidth * Pictures[i].bHeight);
		if (ImageSize > EntrySize)
		{
			Index = i;
			ImageSize = EntrySize;
		}
	}

	return Index;
}

/**
 * Get the index of the desired frame in array of pictures.
 *
 * @param Pictures the array of pictures.
 * @param SizeData the size informations.
 * @return The index of the desired frame in array of pictures.
 * If the desired frame is not founnd, the index of the smallest one is returned.
 */
int MouseCursorSizeHelper::GetIndexOfDesiredFrame(const std::vector<ICONDIRENTRY>& Pictures, SIZEDATA* SizeData)
{
	int Index = -1;
	float CursorBaseSize = GetRegistryValueFloat(REG_CURSOR_SOURCES, REG_KEY_CURSOR_BASE_SIZE, -1);
	float AppliedDPI = GetDPIScale() / 100.0F;

	if (CursorBaseSize != -1)
	{
		float DesiredSize = CursorBaseSize * AppliedDPI;
		for (int i = 0; i < Pictures.size(); i++)
		{
			if (Pictures[i].bWidth == uint8_t(DesiredSize) && Pictures[i].bHeight == uint8_t(DesiredSize))
			{
				Index = i;
				SizeData->isRealSize = true;
				break;
			}
		}
	}
	else
	{
		Index = GetIndexOfSmallestPicture(Pictures);
	}

	return Index;
}

/**
 * Invert the lines order of an array to iinvert its height.
 *
 * @param Array the array to be processed.
 * @param SizeData the size informations.
 */
void MouseCursorSizeHelper::InvertArrayHeight(std::vector<uint32_t>* Array, const SIZEDATA& SizeData)
{
	std::vector<uint32_t> TempArray;
	TempArray.assign(Array->begin(), Array->end());

	for (int y = 0, yInverted = SizeData.height - 1; y < SizeData.height && yInverted >= 0; y++, yInverted--) {
		for (int x = 0; x < SizeData.width; x++) {
			Array->at(size_t(y * SizeData.width + x)) = TempArray[size_t(yInverted * SizeData.width + x)];
		}
	}
}

/**
 * Extract the pixels from the cursor file.
 *
 * @param File the file of the cursor icon.
 * @param BmpHeader the bitmap header with metadatas of the cursor icon.
 * @param SizeData the size informations.
 * @return The pixel array of the mouse cursor picture.
 */
std::vector<uint32_t> MouseCursorSizeHelper::ExtractPixels(std::ifstream& File, const BITMAPINFOHEADER& BmpHeader, SIZEDATA* SizeData)
{
	std::vector<uint32_t> Pixels = {};

	// Validate size and format
	if (BmpHeader.biBitCount == 32 && BmpHeader.biCompression == BI_RGB) {
		SizeData->width = BmpHeader.biWidth;
		SizeData->height = abs(BmpHeader.biHeight) / 2; // Half the height is for the mask
		Pixels.resize(size_t(SizeData->width * SizeData->height));

		// Read the pixels (colors and alpha channel)
		File.read(reinterpret_cast<char*>(Pixels.data()), Pixels.size() * sizeof(uint32_t));

		// Read the mask (1 byte per pixel)
		int MaskWidth = ((SizeData->width + 31) / 32) * BYTES_PER_PIXEL; // Width rounded to the nearest multiple of 32 bits
		std::vector<uint8_t> Mask(MaskWidth * SizeData->height);
		File.read(reinterpret_cast<char*>(Mask.data()), Mask.size());

		// Combine pixels and mask to set transparency
		for (int y = 0; y < SizeData->height; y++) {
			for (int x = 0; x < SizeData->width; x++) {
				int MaskByteIndex = (y * MaskWidth) + (x / 8);
				int MaskBitIndex = 7 - (x % 8);
				bool IsTransparent = (Mask[MaskByteIndex] & (1 << MaskBitIndex)) != 0;

				if (IsTransparent) {
					Pixels[size_t(y * SizeData->width + x)] = 0; // Completely transparent pixel
				}
			}
		}

		// Invert the height to put the array right side up
		InvertArrayHeight(&Pixels, *SizeData);
	}

	return Pixels;
}

/**
 * Get the datas of the cursor file
 *
 * @param File the file of the cursor icon.
 * @param Header the header with metadatas of the cursor icon.
 * @param SizeData the size informations.
 * @return The pixel array of the mouse cursor picture.
 */
std::vector<uint32_t> MouseCursorSizeHelper::GetCursorFileDatas(std::ifstream& File, const ICONDIR& Header, SIZEDATA* SizeData)
{
	std::vector<uint32_t> PixelArray = {};

	// The type of file is a .cur file
	if (Header.idType == 2)
	{
		std::vector<ICONDIRENTRY> Pictures(Header.idCount);
		File.read(reinterpret_cast<char*>(Pictures.data()), Header.idCount * sizeof(ICONDIRENTRY));

		// Read data for the smallest frame of the file
		int SmallestFrameIndex = GetIndexOfDesiredFrame(Pictures, SizeData);
		if (SmallestFrameIndex >= 0 && SmallestFrameIndex < Pictures.size())
		{
			ICONDIRENTRY& Entry = Pictures[SmallestFrameIndex];
			File.seekg(Entry.dwImageOffset, std::ios::beg);

			BITMAPINFOHEADER BmpHeader;
			File.read(reinterpret_cast<char*>(&BmpHeader), sizeof(BITMAPINFOHEADER));

			PixelArray = ExtractPixels(File, BmpHeader, SizeData);
		}
	}

	return PixelArray;
}

/**
 * Get bits array of mouse cursor image from its BITMAP with Windows library.
 *
 * @param SizeData the size informations.
 * @return The pixel array of the mouse cursor picture.
 */
std::vector<uint32_t> MouseCursorSizeHelper::GetPixelArrayOfCurrentMouseImage(SIZEDATA* SizeData)
{
	std::vector<uint32_t> PixelArray = {};
	std::string CursorFileName = GetRegistryValueString(REG_CURSOR_SOURCES, REG_KEY_CURSOR_FILE);

	PurifyPath(&CursorFileName);
	if (!CursorFileName.empty())
	{
		std::ifstream File(CursorFileName, std::ios::binary);

		if (!File.fail() && File.is_open()) {

			// Read the header (ICONDIR)
			ICONDIR Header;
			File.read(reinterpret_cast<char*>(&Header), sizeof(ICONDIR));

			PixelArray = GetCursorFileDatas(File, Header, SizeData);

			File.close();
		}
	}

	return PixelArray;
}

/**
 * Compute the mouse cursor size from its image pixels array.
 *
 * @param PixelArray the pixels array of the mouse cursor image.
 * @param SizeData the size informations.
 * @return The computed original real size of mouse cursor (without scales).
 */
std::pair<float, float> MouseCursorSizeHelper::ComputeCursorSizeFromPixelArray(const std::vector<uint32_t>& PixelArray, const SIZEDATA& SizeData)
{
	std::pair<float, float> CursorSize = std::pair<float, float>(DEFAULT_ORIGIN_MOUSE_WIDTH, DEFAULT_ORIGIN_MOUSE_HEIGHT);

	if (!PixelArray.empty())
	{
		FIRSTLASTINDEXES FirstLastIndexes = InitFirstLastIndexesStruct();
		float Width = 0;
		float Height = 0;

		// Compute the first and last indexes from the valid pixels
		for (int y = 0; y < SizeData.height; y++) {
			for (int x = 0; x < SizeData.width; x++) {
				uint32_t Pixel = PixelArray[size_t(y * SizeData.width + x)];
				uint8_t Alpha = (Pixel >> 24) & 0xFF;

				GetFirstAndLastIndexesFromPixel(Alpha, &FirstLastIndexes, x, y);
			}
			// Compute valid width of the line
			float LineWidth = float(FirstLastIndexes.lastIndexWidth - FirstLastIndexes.firstIndexWidth + 1);
			if (Width < LineWidth)
			{
				Width = LineWidth;
			}
		}

		// Compute valid Height
		Height = float(FirstLastIndexes.lastIndexHeight - FirstLastIndexes.firstIndexHeight + 1);
		CursorSize = std::pair<float, float>(Width, Height);
	}

	return CursorSize;
}

/**
 * Compute the first and last index depending on the pixel color.
 * These indexes will be used to compute the real size of the cursor.
 * 
 *
 * @param Color the color of the current pixel to process.
 * @param IndexesStruct the structure of indexes to compute.
 * @param IndexX the index of the current pixel on the line.
 * @param IndexY the index of the current pixel on the column.
 */
void MouseCursorSizeHelper::GetFirstAndLastIndexesFromPixel(const uint8_t& Alpha, FIRSTLASTINDEXES* IndexesStruct, const int& IndexX, const int& IndexY)
{
	// If pixel is not 100% transparent
	if (Alpha > 0)
	{
		// Compute the first and last index where there is a valid pixel in line
		if (!IndexesStruct->isFirstIndexWidthFound)
		{
			IndexesStruct->firstIndexWidth = IndexX;
			IndexesStruct->isFirstIndexWidthFound = true;
		}
		else
		{
			IndexesStruct->lastIndexWidth = IndexX;
		}

		// Compute the first and last index where there is a valid pixel for height
		if (IndexY != IndexesStruct->lastYValue)
		{
			if (!IndexesStruct->isFirstIndexHeightFound)
			{
				IndexesStruct->firstIndexHeight = IndexY;
				IndexesStruct->isFirstIndexHeightFound = true;
			}
			else
			{
				IndexesStruct->lastIndexHeight = IndexY;
			}
			IndexesStruct->lastYValue = IndexY;
		}
	}
}

/**
 * Scale the real mouse cursor size depending on the mouse cursor size multiplier defined on the system.
 *
 * @param CursorSize the real mouse cursor size to scale.
 */
void MouseCursorSizeHelper::ScaleCursorSizeByMouseSystemScale(std::pair<float, float>* CursorSize)
{
	float MouseScale = GetMouseCursorScale();

	CursorSize->first = CursorSize->first + (MouseScale - 1) * (CursorSize->first / 2);
	CursorSize->second = CursorSize->second + (MouseScale - 1) * (CursorSize->second / 2);
}

/**
 * Scale the real mouse cursor size depending on the DPI defined on the system.
 *
 * @param CursorSize the real mouse cursor size to scale.
 */
void MouseCursorSizeHelper::ScaleCursorSizeByDPI(std::pair<float, float>* CursorSize)
{
	float AppliedDPI = GetDPIScale() / 100.0F;

	CursorSize->first *= AppliedDPI;
	CursorSize->second *= AppliedDPI;
}

/**
 * Get the mouse cursor size multiplier defined on the system, using registry.
 *
 * @return The mouse cursor size multiplier defined on the system.
 */
float MouseCursorSizeHelper::GetMouseCursorScale()
{
	return GetRegistryValueFloat(REG_ACCESSIBILITY_GROUP, REG_KEY_CURSOR_SIZE, DEFAULT_MOUSE_SCALE);
}

/**
 * Get the DPI defined on the system, using registry.
 *
 * @return The DPI defined on the system.
 */
float MouseCursorSizeHelper::GetDPIScale()
{
	float AppliedDPI = GetRegistryValueFloat(REG_CURRENT_DPI_SCALE, REG_KEY_APPLIED_DPI, DEFAULT_APPLIED_DPI);

	return float(DPI_FACTOR) * AppliedDPI;
}

/**
 * Ceil the pair of float passed as parameter.
 * For example: {3.44F, 10.12F} become {3.0F, 10.0F}.
 * 
 * @param Pair the pair to ceil.
 */
void MouseCursorSizeHelper::CeilPair(std::pair<float, float>* Pair)
{
	Pair->first = ceil(Pair->first);
	Pair->second = ceil(Pair->second);
}

/**
 * Get the value in float format of registry key passed as parameter.
 *
 * @param RegLocation the location of the registry key.
 * @param RegKey the registry key to read.
 * @param DefaultValue the default value to make equal the variable if the registry key is not found.
 * @return The value of the registry key in float if found. The default value otherwise.
 */
float MouseCursorSizeHelper::GetRegistryValueFloat(const LPCSTR& RegLocation, const LPCSTR& RegKey, const float& DefaultValue)
{
	float resultValue = DefaultValue;

#ifdef _WIN32
	DWORD Value = 0;
	DWORD DataSize = sizeof(Value);

	long ResultRegCode = RegGetValueA(HKEY_CURRENT_USER, RegLocation, RegKey, RRF_RT_REG_DWORD, NULL, &Value, &DataSize);

	// If the registry exists and value gotten
	if (ResultRegCode == ERROR_SUCCESS)
	{
		resultValue = float(Value);
	}
#endif // _WIN32

	return resultValue;
}

/**
 * Get the value in string format of registry key passed as parameter.
 *
 * @param RegLocation the location of the registry key.
 * @param RegKey the registry key to read.
 * @return The value of the registry key in string format if found. An empty string otherwise.
 */
std::string MouseCursorSizeHelper::GetRegistryValueString(const LPCSTR& RegLocation, const LPCSTR& RegKey)
{
	std::string Value = "";

#ifdef _WIN32
	HKEY hSubKey;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, RegLocation, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
	{
		DWORD type;
		DWORD size;
		if (RegQueryValueExA(hSubKey, RegKey, NULL, &type, NULL, &size) == ERROR_SUCCESS)
		{
			std::string ValueTemp(size, 0);
			if (RegQueryValueExA(hSubKey, RegKey, NULL, &type, LPBYTE(ValueTemp.data()), &size) == ERROR_SUCCESS)
			{
				Value = ValueTemp;
			}
		}
		RegCloseKey(hSubKey);
	}
#endif // _WIN32

	return Value;
}

/**
 * Get the value of environment variable.
 *
 * @param EnvName the name of the environment variable to read.
 * @return The value of the environment variable if it exists. An empty string otherwise.
 */
std::string MouseCursorSizeHelper::GetValueOfEnvVariable(const std::string& EnvName)
{
	std::string EnvValue = "";

	char buffer[MAX_PATH];
	DWORD size = GetEnvironmentVariableA(LPCSTR(EnvName.c_str()), buffer, MAX_PATH);
	if (size != 0) {
		EnvValue = std::string(buffer);
	}

	return EnvValue;
}

/**
 * Get the key value of all environment variables contained in path.
 *
 * @param Path the path in which to look for environment variables.
 * @return The key value of all environment variables contained in path.
 */
std::map<std::string, std::string> MouseCursorSizeHelper::GetAllEnvNameValueInPath(const std::string& Path)
{
	std::map<std::string, std::string> EnvKeyValue = {};

	int count = 0;

	// Count number occurences of '%'
	for (char c : Path)
	{
		count += c == '%';
	}

	// The number of tags is valid
	if (count % 2 == 0)
	{
		std::string EnvName = "";
		bool CanWrite = false;

		// Get the variable names in path
		for (char c : Path)
		{
			if (c == '%')
			{
				CanWrite = !CanWrite;
			}
			else if (CanWrite)
			{
				EnvName += c;
			}

			if (!EnvName.empty() && !CanWrite)
			{
				EnvKeyValue.insert({ EnvName, GetValueOfEnvVariable(EnvName) });
				EnvName = "";
			}
		}
	}

	return EnvKeyValue;
}

/**
 * Replace all evironment variables contained in path by their values. This makes the path valid.
 *
 * @param Path the path to process.
 */
void MouseCursorSizeHelper::PurifyPath(std::string* Path)
{
#ifdef _WIN32
	std::map<std::string, std::string> EnvKeyValue = GetAllEnvNameValueInPath(*Path);

	for (std::pair<std::string, std::string> EnvPair : EnvKeyValue)
	{
		if (!EnvPair.first.empty() && !EnvPair.second.empty())
		{
			std::string KeyName = "%" + EnvPair.first + "%";
			size_t KeyPosition = Path->find(KeyName);
			if (KeyPosition >= 0)
			{
				Path->replace(KeyPosition, KeyName.size(), EnvPair.second);
			}
		}
	}
#endif // _WIN32
}