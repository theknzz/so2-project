#pragma once

# define DLL_EXPORT __declspec(dllexport)
# define DLL_IMPORT __declspec(dllimport)

DLL_EXPORT void register(TCHAR* name, int type);
DLL_EXPORT void log(TCHAR* text);