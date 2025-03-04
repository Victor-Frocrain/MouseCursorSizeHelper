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

#include "FirstLastIndexesStruct.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MouseCursorSizeHelper.generated.h"

constexpr int DEFAULT_IMAGE_CURSOR_SIZE = 32;
constexpr float DEFAULT_ORIGIN_MOUSE_WIDTH = 12;
constexpr float DEFAULT_ORIGIN_MOUSE_HEIGHT = 18;
constexpr float DEFAULT_MOUSE_SCALE = 1;
constexpr float DEFAULT_APPLIED_DPI = 96;

constexpr auto REG_ACCESSIBILITY_GROUP = L"Software\\Microsoft\\Accessibility";
constexpr auto REG_KEY_CURSOR_SIZE = L"CursorSize";
constexpr auto REG_CURRENT_DPI_SCALE = L"Control Panel\\Desktop\\WindowMetrics";
constexpr auto REG_KEY_APPLIED_DPI = L"AppliedDPI";
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
    static FirstLastIndexesStruct InitFirstLastIndexesStruct();
    static TArray<uint8> GetBitsArrayOfCurrentMouseImage(int* Width, int* Height);
    static FVector2f ComputeCursorSizeFromBitsArray(const TArray<uint8>& BitsArray, int ImageWidth, int ImageHeight);
    static void GetFirstAndLastIndexesFromPixel(uint8 Color, FirstLastIndexesStruct* IndexesStruct, int IndexX, int IndexY);
    static void ScaleCursorSizeByMouseSystemScale(FVector2f* CursorSize);
    static void ScaleCursorSizeByDPI(FVector2f* CursorSize);
    static float GetMouseCursorScale();
    static float GetDPIScale();
    static void CeilPair(FVector2f* Vector2f);
    static float GetRegistryValue(LPCWSTR RegLocation, LPCWSTR RegKey, float DefaultValue);
};