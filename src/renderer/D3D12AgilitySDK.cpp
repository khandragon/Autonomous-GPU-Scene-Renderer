#include <Windows.h>

// Agility SDK package:
// Microsoft.Direct3D.D3D12 1.619.3
//
// The SDK version must match the D3D12Core.dll version you ship
// in the app-local D3D12 folder.
extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 619;
    __declspec(dllexport) extern const char *D3D12SDKPath = ".\\D3D12\\";
}