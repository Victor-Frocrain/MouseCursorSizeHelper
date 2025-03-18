/*
 * This file is part of the MouseCursorSizeHelper project.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef MOUSE_CURSOR_SIZE_HELPER_H
#define MOUSE_CURSOR_SIZE_HELPER_H

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

#include <iostream>
#include <vector>
#include <fstream>
#include <map>

constexpr int DEFAULT_IMAGE_CURSOR_SIZE = 32;
constexpr float DEFAULT_ORIGIN_MOUSE_WIDTH = 12;
constexpr float DEFAULT_ORIGIN_MOUSE_HEIGHT = 19;
constexpr float DEFAULT_MOUSE_SCALE = 1;
constexpr float DEFAULT_APPLIED_DPI = 96;

constexpr LPCSTR REG_CURSOR_SOURCES = "Control Panel\\Cursors";
constexpr LPCSTR REG_KEY_CURSOR_FILE = "Arrow";
constexpr LPCSTR REG_KEY_CURSOR_BASE_SIZE = "CursorBaseSize";
constexpr LPCSTR REG_ACCESSIBILITY_GROUP = "Software\\Microsoft\\Accessibility";
constexpr LPCSTR REG_KEY_CURSOR_SIZE = "CursorSize";
constexpr int BYTES_PER_PIXEL = 4;
constexpr double DPI_FACTOR = 100.0 / DEFAULT_APPLIED_DPI;

/**
  * This class was created to get the real size of the mouse cursor
  * with the DPI scale and the mouse system scale. To get this size,
  * call MouseCursorSizeHelper::GetCurrentMouseCursorSize.
  * 
  * @author Victor FROCRAIN
  * @date 04/03/2025
  */
class MouseCursorSizeHelper
{
public:
    /**
    * Get the real current mouse cursor size with scales.
    *
    * @return The pair of the real mouse cursor width and height.
    */
    static std::pair<float, float> GetCurrentMouseCursorSize();

private:
    struct BITMAPINFOHEADER {
        uint32_t biSize;                // Header size
        int32_t biWidth;                // Picture width
        int32_t biHeight;               // Picture height (negative for a top-down)
        uint16_t biPlanes;              // Number of planes
        uint16_t biBitCount;            // Bits per pixel
        uint32_t biCompression;         // Compression type (BI_RGB for no one)
        uint32_t biSizeImage;           // Image size in bytes
        int32_t biXPelsPerMeter;        // Horizontal resolution
        int32_t biYPelsPerMeter;        // Vertical resolution
        uint32_t biClrUsed;             // Number of used colors
        uint32_t biClrImportant;        // Number of important colors
    };

    struct ICONDIRENTRY {
        uint8_t bWidth;                 // Picture width
        uint8_t bHeight;		        // Picture Height
        uint8_t bColorCount;	        // Number of colors (0 if greater than 256)
        uint8_t bReserved;		        // Always 0
        uint16_t wPlanes;		        // Number of color planes
        uint16_t wBitCount;		        // Bits per pixel
        uint32_t dwBytesInRes;	        // Size of data bytes picture
        uint32_t dwImageOffset;	        // Picture datas offset in file
    };

    struct ICONDIR {
        uint16_t idReserved;	        // Always 0
        uint16_t idType;		        // 1 for icon, 2 for cursor
        uint16_t idCount;		        // Number of pictures in file
    };

    struct FIRSTLASTINDEXES {
        int lastYValue;                 // Last Y value saved
        int firstIndexHeight;           // First index of height
        int lastIndexHeight;            // Last index of height
        int firstIndexWidth;            // First index of width
        int lastIndexWidth;             // Last index of width
        bool isFirstIndexHeightFound;   // The first index of height was found
        bool isFirstIndexWidthFound;    // The first index of width was found
    };

    struct SIZEDATA {
        int width;                      // Picture width
        int height;                     // Picture height
        bool isRealSize;                // The corresponding size was found
    };

    static FIRSTLASTINDEXES InitFirstLastIndexesStruct();
    static int GetIndexOfSmallestPicture(const std::vector<ICONDIRENTRY>& Pictures);
    static int GetIndexOfDesiredFrame(const std::vector<ICONDIRENTRY>& Pictures, SIZEDATA* SizeData);
    static void InvertArrayHeight(std::vector<uint32_t>* Array, const SIZEDATA& SizeData);
    static std::vector<uint32_t> ExtractPixels(std::ifstream& File, const BITMAPINFOHEADER& BmpHeader, SIZEDATA* SizeData);
    static std::vector<uint32_t> GetCursorFileDatas(std::ifstream& File, const ICONDIR& Header, SIZEDATA* SizeData);
    static std::vector<uint32_t> GetPixelArrayOfCurrentMouseImage(SIZEDATA* SizeData);
    static std::pair<float, float> ComputeCursorSizeFromPixelArray(const std::vector<uint32_t>& PixelArray, const SIZEDATA& SizeData);
    static void GetFirstAndLastIndexesFromPixel(const uint8_t& Alpha, FIRSTLASTINDEXES* IndexesStruct, const int& IndexX, const int& IndexY);
    static void ScaleCursorSizeByMouseSystemScale(std::pair<float, float>* CursorSize);
    static void ScaleCursorSizeByDPI(std::pair<float, float>* CursorSize);
    static float GetMouseCursorScale();
    static float GetDPIScaleOfWindowsSystem();
    static float GetDPIScale();
    static void CeilPair(std::pair<float, float>* Pair);
    static float GetRegistryValueFloat(const LPCSTR& RegLocation, const LPCSTR& RegKey, const float& DefaultValue);
    static std::string GetRegistryValueString(const LPCSTR& RegLocation, const LPCSTR& RegKey);
    static std::string GetValueOfEnvVariable(const std::string& EnvName);
    static std::map<std::string, std::string> GetAllEnvNameValueInPath(const std::string& Path);
    static void PurifyPath(std::string* Path);
};

#endif // !MOUSE_CURSOR_SIZE_HELPER_H