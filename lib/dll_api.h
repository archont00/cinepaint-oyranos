/* libcinepaint/dll_api.h
// The usual Windows DLL export/import trick
// Copyright Mar 8, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#ifdef WIN32

#ifdef DLL_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

#else

#define DLL_API 

#endif
