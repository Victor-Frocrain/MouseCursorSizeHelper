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

#include "FirstLastIndexesStruct.h"

#include <iostream>
#include <vector>

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
    static FirstLastIndexesStruct InitFirstLastIndexesStruct();
    static std::vector<uint8_t> GetBitsArrayOfCurrentMouseImage(int* Width, int* Height);
    static std::pair<float, float> ComputeCursorSizeFromBitsArray(const std::vector<uint8_t>& BitsArray, int ImageWidth, int ImageHeight);
    static void GetFirstAndLastIndexesFromPixel(uint8_t Color, FirstLastIndexesStruct* IndexesStruct, int IndexX, int IndexY);
    static void ScaleCursorSizeByMouseSystemScale(std::pair<float, float>* CursorSize);
    static void ScaleCursorSizeByDPI(std::pair<float, float>* CursorSize);
    static float GetMouseCursorScale();
    static float GetDPIScale();
    static void CeilPair(std::pair<float, float>* pair);
    static float GetRegistryValue(LPCWSTR RegLocation, LPCWSTR RegKey, float DefaultValue);
};

#endif // !MOUSE_CURSOR_SIZE_HELPER_H