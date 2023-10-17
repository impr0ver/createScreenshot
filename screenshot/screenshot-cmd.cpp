#include <stdio.h>
#include <windows.h>
#include <winuser.h>
#include <shlwapi.h>
#include <time.h>
#include <sys/types.h>
#include <Gdiplus.h>
#include <GdiPlusEnums.h>


#define MAX_BUF_SIZE 1024


#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")


using namespace Gdiplus;




DWORD sendScreenCaptureInfos();
int CreateBMPFile(HWND hwnd, LPTSTR pOutputFileName, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC);
int convertBMP2JPG(char *pBMPFile, char *pJPGFile);
int GetCodecClsid(const WCHAR* pFormat, CLSID* pClsid);
PBITMAPINFO CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp);





int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  int lRetVal = 0;
  char *lBMPFileFullPath = "PrintScreen.bmp";
  char *lJPGFileFullPath = "PrintScreen.jpg";
  PBITMAPINFO lPBI;
  SIZE lSize;
  HWND hWND;
  HDC hDC;
  HDC hMemDC;
  HBITMAP hBitmap;
  HBITMAP hOld;



  //Get full screen.
  hDC = GetDC(NULL); 
  // Make a compatible device context in memory for screen device context.
  hMemDC = CreateCompatibleDC(hDC);
  // Get the handle to the desktop device context.
  hWND = GetDesktopWindow();

  lSize.cx = GetSystemMetrics(SM_CXSCREEN);
  lSize.cy = GetSystemMetrics(SM_CYSCREEN);


  /*
   * Create a compatible bitmap of the screen size and using 
   * the screen device context.
   *
   */

  if (hBitmap = CreateCompatibleBitmap(hDC, lSize.cx, lSize.cy))
  {
    /*
     * Select the compatible bitmap in the memeory device context 
     * and keep the refrence to the old bitmap.
     *
     */
    hOld = (HBITMAP) SelectObject(hMemDC, hBitmap);

    // Copy the Bitmap to the memory device context.
    BitBlt(hMemDC, 0, 0, lSize.cx, lSize.cy, hDC, 0, 0, SRCCOPY);


    // Create BMP-file, convert to JPG, encrypt it, write it to a file
    // and open it.
    if ((lPBI = CreateBitmapInfoStruct(hWND, hBitmap)) != NULL)
    {
      if (CreateBMPFile(hWND, (LPTSTR) lBMPFileFullPath, lPBI, hBitmap, hDC) == 0)
      {
        convertBMP2JPG(lBMPFileFullPath, lJPGFileFullPath);
       // ShellExecute(NULL, "open", lJPGFileFullPath, NULL, NULL, 0);
        DeleteFile(lBMPFileFullPath);
      } // if (CreateBMPFile(hWND, lBMPFileFullPath, lPBI, hBM
    } // if ((lPBI = CreateBitmapInfo

    SelectObject(hMemDC, hOld);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hDC);
    DeleteObject(hBitmap);
    LocalFree(lPBI);

  } // if (hBitmap = CreateCompatibleBitmap(hDC, siz...


  return(lRetVal);
}





/*
 * 
 *
 */

int CreateBMPFile(HWND hwnd, LPTSTR pOutputFileName, PBITMAPINFO pPBI, HBITMAP hBMP, HDC hDC) 
{
  BITMAPFILEHEADER lBFH;
  PBITMAPINFOHEADER lBIH = (PBITMAPINFOHEADER) pPBI;
  LPBYTE lBits = NULL;
  int lRetVal = 0;
  HANDLE lFileHandle = INVALID_HANDLE_VALUE;
  DWORD lBytesWritten = 0;


  if (!(lBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, lBIH->biSizeImage)))
  {
    lBits = NULL;
    lRetVal = 1;
    goto END;
  }

  /*
   * Retrieve the color table (RGBQUAD array) and the bits (array of palette indices) from the DIB. 
   *
   */
  if (!GetDIBits(hDC, hBMP, 0, (WORD) lBIH->biHeight, lBits, pPBI, DIB_RGB_COLORS)) 
  {
    lRetVal = 2;
    goto END;
  }

  if ((lFileHandle = CreateFile(pOutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
  {
    lRetVal = 3;
    goto END;
  }


  lBFH.bfType = 0x4d42; // 0x42 = "B" 0x4d = "M" 
  // Calculate size of the entire file. 
  lBFH.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + lBIH->biSize + lBIH->biClrUsed * sizeof(RGBQUAD) + lBIH->biSizeImage); 
  lBFH.bfReserved1 = 0; 
  lBFH.bfReserved2 = 0; 

  // Calculate offset to the array of color indices. 
  lBFH.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + lBIH->biSize + lBIH->biClrUsed  * sizeof (RGBQUAD); 

  // Copy BITMAPFILEHEADER into the .BMP file. 
  WriteFile(lFileHandle, &lBFH, sizeof(BITMAPFILEHEADER), &lBytesWritten, NULL);
  // Copy BITMAPINFOHEADER and RGBQUAD array into the file. 
  WriteFile(lFileHandle, lBIH, sizeof(BITMAPINFOHEADER) + lBIH->biClrUsed * sizeof (RGBQUAD), &lBytesWritten, NULL);
  // Copy array of color indices into the .BMP file. 
  WriteFile(lFileHandle, lBits, lBIH->biSizeImage, &lBytesWritten, NULL);

END:

  /*
   * Free allocated resources.
   *
   */

  if (lBits != NULL)
    GlobalFree((HGLOBAL) lBits);

  if (lFileHandle != INVALID_HANDLE_VALUE)
    CloseHandle(lFileHandle);

  return(lRetVal);
}








/*
 * Create bitmap structure (what else :))
 *
 */


PBITMAPINFO CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp)
{ 
  BITMAP lBitMap; 
  PBITMAPINFO lBMI = NULL; 
  WORD lCtrlBits;


  // Retrieve the bitmap color format, width, and height. 
  if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&lBitMap))
    goto END;


  // Convert the color format to a count of bits. 
  lCtrlBits = (WORD)(lBitMap.bmPlanes * lBitMap.bmBitsPixel); 
  if (lCtrlBits == 1) 
    lCtrlBits = 1; 
  else if (lCtrlBits <= 4) 
    lCtrlBits = 4; 
  else if (lCtrlBits <= 8) 
    lCtrlBits = 8; 
  else if (lCtrlBits <= 16) 
    lCtrlBits = 16; 
  else if (lCtrlBits <= 24) 
    lCtrlBits = 24; 
  else lCtrlBits = 32; 

  /*
   * Allocate memory for the BITMAPINFO structure. (This structure 
   * contains a BITMAPINFOHEADER structure and an array of RGBQUAD 
   * data structures.) 
   *
   */

  if (lCtrlBits != 24)
    lBMI = (PBITMAPINFO) LocalAlloc(LPTR,sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1<< lCtrlBits));
  // There is no RGBQUAD array for the 24-bit-per-pixel format. 
  else 
    lBMI = (PBITMAPINFO) LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER));

  /*
   * Initialize the fields in the BITMAPINFO structure. 
   *
   */

  lBMI->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
  lBMI->bmiHeader.biWidth = lBitMap.bmWidth; 
  lBMI->bmiHeader.biHeight = lBitMap.bmHeight; 
  lBMI->bmiHeader.biPlanes = lBitMap.bmPlanes; 
  lBMI->bmiHeader.biBitCount = lBitMap.bmBitsPixel; 

  if (lCtrlBits < 24) 
    lBMI->bmiHeader.biClrUsed = (1<<lCtrlBits); 

  // If the bitmap is not compressed, set the BI_RGB flag. 
  lBMI->bmiHeader.biCompression = BI_RGB; 

  /*
   * Compute the number of bytes in the array of color 
   * indices and store the result in biSizeImage. 
   * For Windows NT, the width must be DWORD aligned unless 
   * the bitmap is RLE compressed. This example shows this. 
   * For Windows 95/98/Me, the width must be WORD aligned unless the 
   * bitmap is RLE compressed.
   *
   */

  lBMI->bmiHeader.biSizeImage = ((lBMI->bmiHeader.biWidth * lCtrlBits +31) & ~31) /8 * lBMI->bmiHeader.biHeight; 
  // Set biClrImportant to 0, indicating that all of the 
  // device colors are important. 
  lBMI->bmiHeader.biClrImportant = 0; 

END:

  return(lBMI); 
}






/*
 * Convert from Bitmap format to JPG.
 *
 */

int convertBMP2JPG(char *pBMPFile, char *pJPGFile)
{
  CLSID lCodecClsid;
  EncoderParameters lEncoderParameters;
  long lQuality = 80;
  GdiplusStartupInput lGdiplusStartupInput;
  ULONG lGdiplusToken;
  WCHAR lWCBMPFile[MAX_BUF_SIZE + 1];
  WCHAR lWCJPGFile[MAX_BUF_SIZE + 1];
  int lRetVal = 0;


  ZeroMemory(lWCBMPFile, sizeof(lWCBMPFile));
  ZeroMemory(lWCBMPFile, sizeof(lWCJPGFile));

  mbstowcs(lWCBMPFile, pBMPFile, sizeof(lWCBMPFile) / sizeof(WCHAR));
  mbstowcs(lWCJPGFile, pJPGFile, sizeof(lWCJPGFile) / sizeof(WCHAR));

  GdiplusStartup(&lGdiplusToken, &lGdiplusStartupInput, NULL);
  {
    Image image(lWCBMPFile);

    // Get the CLSID of the JPEG codec.
    if (GetCodecClsid(L"image/jpeg", &lCodecClsid) >= 0)
    {
      lEncoderParameters.Count = 1;
      lEncoderParameters.Parameter[0].Guid = EncoderQuality;
      lEncoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
      lEncoderParameters.Parameter[0].NumberOfValues = 1;
      lEncoderParameters.Parameter[0].Value = &lQuality;

      if (image.Save(lWCJPGFile, &lCodecClsid, &lEncoderParameters) != Ok)
        lRetVal = 1;
    } // if (GetCodecClsid(L"image/jpeg", &lCodecClsid) >= 0)
  }

  GdiplusShutdown(lGdiplusToken);

  return(lRetVal);
}








/*
 * Get an array of ImageCodecInfo objects that contain 
 * information about the available image encoders.
 *
 */

int GetCodecClsid(const WCHAR* pFormat, CLSID* pClsid)
{
  UINT lNumberOfEncoders = 0;
  UINT lArraySize = 0;
  ImageCodecInfo* lImageCodecInfo = NULL;
  int lRetVal = 0;
  int lCounter = 0;

  GetImageEncodersSize(&lNumberOfEncoders, &lArraySize);
  if(lArraySize == 0)
  {
    lRetVal = -1;
    goto END;
  }


  if ((lImageCodecInfo = (ImageCodecInfo*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lArraySize)) == NULL)
  {
    lRetVal = -2;
    goto END;
  }

  GetImageEncoders(lNumberOfEncoders, lArraySize, lImageCodecInfo);

  for(lCounter = 0; lCounter < lNumberOfEncoders; ++lCounter)
  {
    if(wcscmp(lImageCodecInfo[lCounter].MimeType, pFormat) == 0 )
    {
      *pClsid = lImageCodecInfo[lCounter].Clsid;
      lRetVal = lCounter; 
      goto END;
    } // if(wcscmp(lImageC...
  } // for(lCounter = 0; lCounter < lNu


END:
  if (lImageCodecInfo != NULL)
    HeapFree(GetProcessHeap(), 0, lImageCodecInfo);


  return(lRetVal);
}
