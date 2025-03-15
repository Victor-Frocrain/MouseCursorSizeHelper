# Mouse Cursor Size Helper

The goal of this script is to allow to recover the real size of the mouse cursor, taking into account the DPI and the configured scale of the cursor. For example, this can be helpful to scale a custom cursor to the exact size of the system mouse cursor in a video game. The script uses the Windows API, but for other devices, it returns the default size values. This project contains two different versions of MouseCursorSizeHelper : a generic version and an Unreal Engine one.



## Usage

### Generic Version

This script is suitable for any common C++ project without any specific library.

1. Copy the files contained in the *Generic Version* directory to your C++ project (*MouseCursorSizeHelper.h* and *MouseCursorSizeHelper.cpp*).
2. To get the real cursor size, use this line : `std::pair<float, float> CursorSize = MouseCursorSizeHelper::GetCurrentMouseCursorSize();`. The width of the mouse cursor is stored in `CursorSize.first` and the height is in `CursorSize.second`.



### Unreal Engine Version

This script is suitable for any Unreal Engine project and can be used from Blueprints. Your Unreal Engine project must be configured to be built with a C++ project.

1. Copy the files contained in *Unreal Engine Version* directory to your C++ project for Unreal Engine. The file into the *Private* folder must be pasted to *Source/[project_name]/Private*. The file into the *Public* folder must be transferred to *Source/[project_name]/Public*.

2. There are two ways to use this script :

   - **From the C++ project** with this line : `FVector2f CursorSize = MouseCursorSizeHelper::GetCurrentMouseCursorSize();`.
   - **From the Blueprints** calling the pure function `GetCurrentMouseCursorSize()`.

   The function `GetCurrentMouseCursorSize()` returns an `FVector2f CursorSize`. The width of the cursor is stored in `CursorSize.X` and the height is in `CursorSize.Y`.