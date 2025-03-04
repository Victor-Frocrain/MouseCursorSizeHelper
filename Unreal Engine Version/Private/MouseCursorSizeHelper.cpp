/*
 * This file is part of the MouseCursorSizeHelper project.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "MouseCursorSizeHelper.h"

/**
 * Get the real current mouse cursor size with scales.
 *
 * @return The pair of the real mouse cursor width and height.
 */
FVector2f UMouseCursorSizeHelper::GetCurrentMouseCursorSize()
{
	int Width = DEFAULT_IMAGE_CURSOR_SIZE;
	int Height = DEFAULT_IMAGE_CURSOR_SIZE;
	TArray<uint8> BitsArray = GetBitsArrayOfCurrentMouseImage(&Width, &Height);

	// Compute the origin real size of mouse cursor
	FVector2f CursorSize = ComputeCursorSizeFromBitsArray(BitsArray, Width, Height);

	// Scale mouse cursor size by DPI
	ScaleCursorSizeByDPI(&CursorSize);

	// Scale mouse cursor size by defined system mouse size
	ScaleCursorSizeByMouseSystemScale(&CursorSize);

	// Ceil mouse cursor size
	CeilPair(&CursorSize);

	return CursorSize;
}

/**
 * Initialize FirstLastIndexes structure.
 *
 * @return The initialized FirstLastIndexes structure.
 */
FirstLastIndexesStruct UMouseCursorSizeHelper::InitFirstLastIndexesStruct()
{
	FirstLastIndexesStruct IndexesStruct;

	IndexesStruct.LastYValue = -1; // Negative value to take first line in account for height calculation
	IndexesStruct.FirstIndexHeight = 0;
	IndexesStruct.LastIndexHeight = 0;
	IndexesStruct.FirstIndexWidth = 0;
	IndexesStruct.LastIndexWidth = 0;
	IndexesStruct.IsFirstIndexHeightFound = false;
	IndexesStruct.IsFirstIndexWidthFound = false;

	return IndexesStruct;
}

/**
 * Get bits array of mouse cursor image from its BITMAP with Windows library.
 *
 * @param Width the width of cursor mouse image.
 * @param Height the height of cursor mouse image.
 * @return The bits array of the mouse cursor image.
 */
TArray<uint8> UMouseCursorSizeHelper::GetBitsArrayOfCurrentMouseImage(int* Width, int* Height)
{
	TArray<uint8> BitsArray = {};

#ifdef _WIN32
	ICONINFO IconInfo;
	BITMAP Bitmap;

	// Get mouse icon informations
	GetIconInfo(LoadCursor(NULL, IDC_ARROW), &IconInfo);

	// Get the device context (HDC)
	HDC Hdc = GetDC(NULL);

	if (Hdc != NULL)
	{
		if (GetObject(IconInfo.hbmColor, sizeof(BITMAP), &Bitmap) != 0)
		{
			// Size of Bitmap
			*Width = Bitmap.bmWidth;
			*Height = Bitmap.bmHeight;

			// Resize the array of data bits
			BitsArray.Reserve((*Width) * (*Height) * BYTES_PER_PIXEL);

			// Get the BITMAPINFO to read bits
			BITMAPINFO BmpInfo = {};
			BmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			BmpInfo.bmiHeader.biWidth = *Width;
			BmpInfo.bmiHeader.biHeight = -*Height; // Negative height for top-down order
			BmpInfo.bmiHeader.biPlanes = 1;
			BmpInfo.bmiHeader.biBitCount = BYTES_PER_PIXEL * 8;
			BmpInfo.bmiHeader.biCompression = BI_RGB;

			// Get and read bits in the array to verify validity
			if (GetDIBits(Hdc, IconInfo.hbmColor, 0, *Height, BitsArray.GetData(), &BmpInfo, DIB_RGB_COLORS) == 0)
			{
				BitsArray.Empty();
			}
		}

		// Free the device context
		ReleaseDC(NULL, Hdc);
	}
#endif // _WIN32

	return BitsArray;
}

/**
 * Compute the mouse cursor size from its image bits array.
 *
 * @param BitsArray the bits array of the mouse cursor image.
 * @param ImageWidth the width of the mouse cursor image.
 * @param ImageHeight the height of the mouse cursor image.
 * @return The computed original real size of mouse cursor (without scales).
 */
FVector2f UMouseCursorSizeHelper::ComputeCursorSizeFromBitsArray(const TArray<uint8>& BitsArray, int ImageWidth, int ImageHeight)
{
	FVector2f CursorSize = FVector2f(DEFAULT_ORIGIN_MOUSE_WIDTH, DEFAULT_ORIGIN_MOUSE_HEIGHT);
	FirstLastIndexesStruct IndexesStruct = InitFirstLastIndexesStruct();
	float Width = 0;
	float Height = 0;

	if (&BitsArray != NULL && !BitsArray.IsEmpty())
	{
		for (int y = 0; y < ImageWidth; y++)
		{
			IndexesStruct.FirstIndexWidth = 0;
			IndexesStruct.LastIndexWidth = 0;
			IndexesStruct.IsFirstIndexWidthFound = false;

			for (int x = 0; x < ImageHeight; x++) {
				int Index = (y * ImageWidth + x) * BYTES_PER_PIXEL;

				// Get the pixel color
				uint8 Blue = BitsArray[Index];
				uint8 Green = BitsArray[Index + 1];
				uint8 Red = BitsArray[Index + 2];
				uint8 Alpha = BitsArray[Index + 3];

				GetFirstAndLastIndexesFromPixel(Blue + Green + Red + Alpha, &IndexesStruct, x, y);
			}

			// Compute valid width of the line
			float LineWidth = float(IndexesStruct.LastIndexWidth - IndexesStruct.FirstIndexWidth + 1);
			if (Width < LineWidth)
			{
				Width = LineWidth;
			}
		}

		// Compute valid Height
		Height = float(IndexesStruct.LastIndexHeight - IndexesStruct.FirstIndexHeight + 1);
		CursorSize = FVector2f(Width, Height);
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
void UMouseCursorSizeHelper::GetFirstAndLastIndexesFromPixel(uint8 Color, FirstLastIndexesStruct* IndexesStruct, int IndexX, int IndexY)
{
	// If pixel is not 100% transparent and is colored
	if (Color > 0)
	{
		// Compute the first and last index where there is a valid pixel in line
		if (!IndexesStruct->IsFirstIndexWidthFound)
		{
			IndexesStruct->FirstIndexWidth = IndexX;
			IndexesStruct->IsFirstIndexWidthFound = true;
		}
		else
		{
			IndexesStruct->LastIndexWidth = IndexX;
		}

		// Compute the first and last index where there is a valid pixel for height
		if (IndexY != IndexesStruct->LastYValue)
		{
			if (!IndexesStruct->IsFirstIndexHeightFound)
			{
				IndexesStruct->FirstIndexHeight = IndexY;
				IndexesStruct->IsFirstIndexHeightFound = true;
			}
			else
			{
				IndexesStruct->LastIndexHeight = IndexY;
			}
			IndexesStruct->LastYValue = IndexY;
		}
	}
}

/**
 * Scale the real mouse cursor size depending on the mouse cursor size multiplier defined on the system.
 *
 * @param CursorSize the real mouse cursor size to scale.
 */
void UMouseCursorSizeHelper::ScaleCursorSizeByMouseSystemScale(FVector2f* CursorSize)
{
	float MouseScale = GetMouseCursorScale();

	CursorSize->X = CursorSize->X + (MouseScale - 1) * (CursorSize->X / 2);
	CursorSize->Y = CursorSize->Y + (MouseScale - 1) * (CursorSize->Y / 2);
}

/**
 * Scale the real mouse cursor size depending on the DPI defined on the system.
 *
 * @param CursorSize the real mouse cursor size to scale.
 */
void UMouseCursorSizeHelper::ScaleCursorSizeByDPI(FVector2f* CursorSize)
{
	float DPIScale = GetDPIScale() / 100.0F;

	CursorSize->X *= DPIScale;
	CursorSize->Y *= DPIScale;
}

/**
 * Get the mouse cursor size multiplier defined on the system, using registry.
 *
 * @return The mouse cursor size multiplier defined on the system.
 */
float UMouseCursorSizeHelper::GetMouseCursorScale()
{
	return GetRegistryValue(REG_ACCESSIBILITY_GROUP, REG_KEY_CURSOR_SIZE, DEFAULT_MOUSE_SCALE);
}

/**
 * Get the DPI defined on the system, using registry.
 *
 * @return The DPI defined on the system.
 */
float UMouseCursorSizeHelper::GetDPIScale()
{
	float AppliedDPI = GetRegistryValue(REG_CURRENT_DPI_SCALE, REG_KEY_APPLIED_DPI, DEFAULT_APPLIED_DPI);

	return float(DPI_FACTOR) * AppliedDPI;
}

/**
 * Ceil the Vector2f of float passed as parameter.
 * For example: {3.44F, 10.12F} become {3.0F, 10.0F}.
 *
 * @param Vector2f the Vector2f to ceil.
 */
void UMouseCursorSizeHelper::CeilPair(FVector2f* Vector2f)
{
	Vector2f->X = ceil(Vector2f->X);
	Vector2f->Y = ceil(Vector2f->Y);
}

/**
 * Get the value of regiistry key passed as parameter.
 *
 * @param RegLocation the location of the registry key.
 * @param RegKey the registry key to read.
 * @param DefaultValue the default value to make equal the variable if the registry key is not found.
 * @return The value of the registry key if found. The default value otherwise.
 */
float UMouseCursorSizeHelper::GetRegistryValue(LPCWSTR RegLocation, LPCWSTR RegKey, float DefaultValue)
{
	float resultValue = DefaultValue;

#ifdef _WIN32
	DWORD Value = 0;
	DWORD DataSize = sizeof(Value);
	long ResultRegCode = RegGetValueW(HKEY_CURRENT_USER, RegLocation, RegKey, RRF_RT_REG_DWORD, NULL, &Value, &DataSize);

	// If the registry exists and value gotten
	if (ResultRegCode == ERROR_SUCCESS)
	{
		resultValue = float(Value);
	}
#endif // _WIN32

	return resultValue;
}
