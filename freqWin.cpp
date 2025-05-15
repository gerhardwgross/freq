/***************************************************************************
//
// Copyright (c) 1995, 1996, 1997, 1998, 1999 Gerhard W. Gross.
//
// THIS SOFTWARE IS PROVIDED BY GERHARD W. GROSS ''AS IS'' AND ANY
// EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GERHARD W. GROSS 
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
// PERMISSION TO USE, COPY, MODIFY, AND DISTRIBUTE THIS SOFTWARE AND ITS
// DOCUMENTATION FOR ANY PURPOSE AND WITHOUT FEE IS HEREBY GRANTED,
// PROVIDED THAT THE ABOVE COPYRIGHT NOTICE, THE ABOVE DISCLAIMER
// NOTICE, THIS PERMISSION NOTICE, AND THE FOLLOWING ATTRIBUTION
// NOTICE APPEAR IN ALL SOURCE CODE FILES AND IN SUPPORTING
// DOCUMENTATION AND THAT GERHARD W. GROSS BE GIVEN ATTRIBUTION
// AS THE MAIN AUTHOR OF THIS PROGRAM IN THE FORM OF A TEXTUAL
// MESSAGE AT PROGRAM STARTUP OR IN THE DISPLAY OF A USAGE MESSAGE,
// OR IN DOCUMENTATION (ONLINE OR TEXTUAL) PROVIDED WITH THIS PROGRAM.
//
// ALL OR PARTS OF THIS CODE WERE WRITTEN BY GERHARD W. GROSS, 1995-2003.
//
// Functions that are specific to Win32 are isolated in this file.
// There is an analogous file for Linux/Unice specific code.  The
// reason that forces seperate code paths is subdirectory recursion.  Unices
// perform variable expansion from the command line by the shell.  Win32
// systems do not, but instead perform variable expansion within the
// "_findfirsti64" and "_findnexti64" functions.
***************************************************************************/

//#include <exception>
#include <windows.h>
#include <minwinbase.h>
#include <strsafe.h>
#include <stdio.h>
#include <wchar.h>
#include <cstdio>
#include <cwchar>
#include "freqwin.h"
#include "freq.h"

extern bool Verbose;
extern bool Prnt_Lines;
extern bool Prnt_Min;
extern bool File_Find;
extern bool Prnt_Some;
extern bool SuppressErrorsPrintout;
extern bool g_reverseSlashDir;
extern long g_numFilesMatchPattern;
extern long g_numDirsMatchPattern;
extern long g_numFilesAndDirsChecked;
extern long g_numDirsToOmit;
extern TCHAR g_dirsToOmit[MAX_DIRS_TO_OMIT][_MAX_PATH];
extern long g_numFilePtrnsToOmit;
extern TCHAR g_filePtrnsToOmit[MAX_FILE_PATTERNS_TO_OMIT][_MAX_PATH];

void PrintLastError(TCHAR* lpszFunction);

//TCHAR* strerror(int errNum)
//{  
//    const int errMsgSz = 2048;
//    static TCHAR errMsg[errMsgSz];
//    errMsg[0] = 0;
//    _wcserror_s(errMsg, errMsgSz, errNum);
//    return errMsg;
//}

bool ShouldIgnoreThisFile(WIN32_FIND_DATA ffd)
{
    TCHAR lwrStr[_MAX_PATH];

    CopyStringToLowerCaseRemoveAsterisk(ffd.cFileName, lwrStr);
    for (int i = 0; i < g_numFilePtrnsToOmit; ++i)
    {
        unsigned int tmp = ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        if ((tmp == 0) && wcsstr(lwrStr, g_filePtrnsToOmit[i]) != 0)
            return true;
    }

    return false;
}

bool ShouldIgnoreThisDir(WIN32_FIND_DATA ffd)
{
    for (int i = 0; i < g_numDirsToOmit; ++i)
    {
        if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && _wcsicmp(g_dirsToOmit[i], ffd.cFileName) == 0)
            return true;
    }

    return false;
}

/***************************************************************************
A directory is passed in here
***************************************************************************/

bool IsSensibleFileName(const TCHAR* dirName)
{
	bool retVal = false;
	int i;
	size_t len = wcslen(dirName);

	if (len > 0)
	{
		for (i = 0; i < len; ++i)
		{
            // Allow extended ASCII chars (>= 127) since have seen the copyright symbol used
			if (dirName[i] > -1 && !isprint(dirName[i]))
				break;
		}

		// This directory name is OK if all characaters were printable
		// (loop did not end early).
		if (i >= len)
			retVal = true;
	}

	return retVal;
}

/***************************************************************************
This recursive function is called for every subdirectory in the search
path.  When it finally encounters a directory with no subdirectories,
SearchCurrentDirectory() is called to do the actual file searching
dependent on the filename, with any file cards, supplied on the command
line.
***************************************************************************/

void SearchAllDirectories(const TCHAR *raw_in_file)
{
    const int curPathSz = _MAX_PATH;
    TCHAR current_path[curPathSz];
    const TCHAR all_wildcard[] = L"*";    // search through for all directories
    long fl_cnt = 0;
    const int errStrSz = _MAX_PATH;
    TCHAR errStr[errStrSz];
	bool skipThisDir = false;
    static int funcRecursionCnt = 0;
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    try
    {
        funcRecursionCnt++;
        if(_wgetcwd(current_path, curPathSz) == NULL)
        {
            errStr[0] = 0;
            if (Verbose == 1)
                swprintf_s(errStr, errStrSz, L"  Attempting to _getcwd to \"%s\", funcRecursionCnt = %d - ",
                    current_path, funcRecursionCnt);
            throw errStr;
        }

        if ((hFind = FindFirstFileEx(all_wildcard, FindExInfoBasic, &ffd, FindExSearchNameMatch, nullptr, 0)) == INVALID_HANDLE_VALUE)
        {
            if (!SuppressErrorsPrintout)
                wprintf(L"\n  No \"%s\" files in %s", all_wildcard, current_path);
        }
        else
        {
		    do
		    {
                errStr[0] = 0;
                if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0 || wcscmp(ffd.cFileName, L"") == 0 || ShouldIgnoreThisDir(ffd))
                    continue;

				g_numFilesAndDirsChecked++;
				if((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					continue;
				try
				{
					if (IsSensibleFileName(ffd.cFileName))
					{
						if(_wchdir(ffd.cFileName) != 0)
						{
                            if (Verbose == 1)
                                swprintf_s(errStr, errStrSz, L"  Attempting to _chdir to \"%s\\%s\", file attrib: %d, funcRecursionCnt: %d - ",
									current_path, ffd.cFileName, ffd.dwFileAttributes, funcRecursionCnt);
							throw errStr;
						}
					}
				}
				catch (TCHAR * excp)
				{
					PrintLastError(excp);
					continue;
				}

				SearchAllDirectories(raw_in_file);

				try
				{
					if(_wchdir(current_path) != 0)
					{
						swprintf_s(errStr, errStrSz, L"  Attempting to _chdir back to \"%s\" - ", current_path);
						throw errStr;
					}
				}
				catch (TCHAR* excp)
				{
					PrintLastError(excp);
					continue;
				}
            } while ((hFind != INVALID_HANDLE_VALUE) && (FindNextFileW(hFind, &ffd) != 0));
        }

        if (hFind != INVALID_HANDLE_VALUE && hFind != 0)
            FindClose(hFind);

		SearchCurrentDirectory(raw_in_file, current_path);
    }
    catch (TCHAR* excp)
    {
        if (excp[0] != 0)
            PrintLastError(excp);
    }

    funcRecursionCnt--;
}

/***************************************************************************
This function is called for every directory that has no subdirectories or
if the -R command line option was not supplied.  The current directory is
searched for files that match the file name, with any wildcards, given at
the command line.  For each of those files, ProcessFile() is called which
does the actual search and replace in the file contents of the command
line specified strings.
***************************************************************************/

void SearchCurrentDirectory(const TCHAR *raw_in_file, const TCHAR *current_path)
{
    long fl_cnt = 0, freq_cntr = 0;
    TCHAR cur_path_tmp[_MAX_PATH];
    TCHAR buff[_MAX_PATH];
    TCHAR errStr[_MAX_PATH];
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    TCHAR fname[_MAX_PATH];

    try
    {
        if (current_path[0] != 0)
            wcscpy_s(cur_path_tmp, _MAX_PATH, current_path);
        else
        {
            cur_path_tmp[0] = '.';
            cur_path_tmp[1] = 0;
        }

        errStr[0] = 0;
        if ((hFind = FindFirstFileEx(raw_in_file, FindExInfoBasic, &ffd, FindExSearchNameMatch, nullptr, 0)) == INVALID_HANDLE_VALUE)
        {
            if (!SuppressErrorsPrintout)
                swprintf_s(errStr, _MAX_PATH, L"  Error attempting to FindFirstFile in %s\\%s", current_path, raw_in_file);
            throw errStr;
        }

        do
        {
            if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
                continue;

            wcscpy_s(fname, _MAX_PATH, ffd.cFileName);
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                if (File_Find)
                {
                    // No search string just print the files that match the file pattern
                    //sprintf_s(buff, _MAX_PATH, "%s\\%s\n", cur_path_tmp, ffd.cFileName);
                    //printf(buff);
                    wprintf_s(L"\n%s\\%s", cur_path_tmp, fname);
                    g_numFilesMatchPattern++;
                }
                else if (ShouldIgnoreThisFile(ffd))
				    continue;

                freq_cntr = ProcessFile(fname);

				if ((freq_cntr > 0 || Verbose == 1) && (!Prnt_Min || Prnt_Some))
				{
					//freq_cntr > 0 ? printf(" +") : printf("  ");
                    swprintf_s(buff, _MAX_PATH, L"\n%d occurrence(s) in:  %s\\%s", freq_cntr, cur_path_tmp, fname);
					if (g_reverseSlashDir == 1)
						ReverseSlashDirInString(buff);
					wprintf(buff);
					if (Prnt_Lines || Verbose)
						printf("\n  ---------------------------------------------------------------------------------\n");
                }
            }
			else if (File_Find && ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!ShouldIgnoreThisDir(ffd))
				{
					wprintf(L"\n%s\\%s", cur_path_tmp, fname);
					g_numDirsMatchPattern++;
				}
			}
        } while ((hFind != INVALID_HANDLE_VALUE) && (FindNextFileW(hFind, &ffd) != 0));
    }
    catch (TCHAR* excp)
    {
        if (excp[0] != 0)
            PrintLastError(excp);
    }

    if (hFind != INVALID_HANDLE_VALUE && hFind != 0)
        FindClose(hFind);
}

void PrintLastError(TCHAR* msg)
{
    if (SuppressErrorsPrintout)
        return;

    // Retrieve the system error message for the last-error code
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen(msg)+40)*sizeof(TCHAR));
    StringCchPrintfW((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("%s failed with error %d: %s"),  msg, dw, lpMsgBuf);
    //MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
	fwprintf(stderr, L"\n  GetLastError: %s", (LPCTSTR)lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    //ExitProcess(dw); 
}

/***************************************************************************
     This function prints an error message and the system error message
     using perror().  exit() is not called from here.
***************************************************************************/
/*
void SysError( char *fmt, ... )
{
    char buff[256];
    va_list    va;

    va_start(va, fmt);
    wvsprintf( buff, fmt, va );
    perror(buff);
}
*/
