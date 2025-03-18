/*
 * This file is part of the MouseCursorSizeHelper project.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

#include <iostream>
#include <vector>
#include <fstream>
#include <map>

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MouseCursorSizeHelper.generated.h"

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
UCLASS(Blueprintable)
class PAZAAK_API UMouseCursorSizeHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    /**
    * Get the real current mouse cursor size with scales.
    *
    * @return The Vector2f of the real mouse cursor width and height.
    */
    UFUNCTION(BlueprintCallable, BlueprintPure)
    static FVector2f GetCurrentMouseCursorSize();

private:
    struct FBitmapinfoheader {
        uint32 biSize;                // Header size
        int32 biWidth;                // Picture width
        int32 biHeight;               // Picture height (negative for a top-down)
        uint16 biPlanes;              // Number of planes
        uint16 biBitCount;            // Bits per pixel
        uint32 biCompression;         // Compression type (BI_RGB for no one)
        uint32 biSizeImage;           // Image size in bytes
        int32 biXPelsPerMeter;        // Horizontal resolution
        int32 biYPelsPerMeter;        // Vertical resolution
        uint32 biClrUsed;             // Number of used colors
        uint32 biClrImportant;        // Number of important colors
    };

    struct FIcondirentry {
        uint8 bWidth;                 // Picture width
        uint8 bHeight;		        // Picture Height
        uint8 bColorCount;	        // Number of colors (0 if greater than 256)
        uint8 bReserved;		        // Always 0
        uint16 wPlanes;		        // Number of color planes
        uint16 wBitCount;		        // Bits per pixel
        uint32 dwBytesInRes;	        // Size of data bytes picture
        uint32 dwImageOffset;	        // Picture datas offset in file
    };

    struct FIcondir {
        uint16 idReserved;	        // Always 0
        uint16 idType;		        // 1 for icon, 2 for cursor
        uint16 idCount;		        // Number of pictures in file
    };

    struct FFirstlastindexes {
        int lastYValue;                 // Last Y value saved
        int firstIndexHeight;           // First index of height
        int lastIndexHeight;            // Last index of height
        int firstIndexWidth;            // First index of width
        int lastIndexWidth;             // Last index of width
        bool isFirstIndexHeightFound;   // The first index of height was found
        bool isFirstIndexWidthFound;    // The first index of width was found
    };

    struct FSizedata {
        int width;                      // Picture width
        int height;                     // Picture height
        bool isRealSize;                // The corresponding size was found
    };

    static FFirstlastindexes InitFirstLastIndexesStruct();
    static int GetIndexOfSmallestPicture(const TArray<FIcondirentry>& Pictures);
    static int GetIndexOfDesiredFrame(const TArray<FIcondirentry>& Pictures, FSizedata* SizeData);
    static void InvertArrayHeight(TArray<uint32>* Array, const FSizedata& SizeData);
    static TArray<uint32> ExtractPixels(std::ifstream& File, const FBitmapinfoheader& BmpHeader, FSizedata* SizeData);
    static TArray<uint32> GetCursorFileDatas(std::ifstream& File, const FIcondir& Header, FSizedata* SizeData);
    static TArray<uint32> GetPixelArrayOfCurrentMouseImage(FSizedata* SizeData);
    static FVector2f ComputeCursorSizeFromPixelArray(const TArray<uint32>& PixelArray, const FSizedata& SizeData);
    static void GetFirstAndLastIndexesFromPixel(const uint8& Alpha, FFirstlastindexes* IndexesStruct, const int& IndexX, const int& IndexY);
    static void ScaleCursorSizeByMouseSystemScale(FVector2f* CursorSize);
    static void ScaleCursorSizeByDPI(FVector2f* CursorSize);
    static float GetMouseCursorScale();
    static float GetDPIScaleOfWindowsSystem();
    static float GetDPIScale();
    static void CeilVector2f(FVector2f* Vector2f);
    static float GetRegistryValueFloat(const LPCSTR& RegLocation, const LPCSTR& RegKey, const float& DefaultValue);
    static std::string GetRegistryValueString(const LPCSTR& RegLocation, const LPCSTR& RegKey);
    static std::string GetValueOfEnvVariable(const std::string& EnvName);
    static std::map<std::string, std::string> GetAllEnvNameValueInPath(const std::string& Path);
    static void PurifyPath(std::string* Path);
};