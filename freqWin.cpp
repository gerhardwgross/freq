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
#include "freqwin.h"
#include "freq.h"

extern bool Verbose;
extern bool Prnt_Lines;
extern bool Prnt_Min;
extern bool File_Find;
extern bool Prnt_Some;
extern bool g_reverseSlashDir;
extern long g_numFilesMatchPattern;
extern long g_numDirsMatchPattern;
extern long g_numFilesAndDirsChecked;
extern long g_numDirsToOmit;
extern char g_dirsToOmit[MAX_DIRS_TO_OMIT][_MAX_PATH];
extern long g_numFilePtrnsToOmit;
extern char g_filePtrnsToOmit[MAX_FILE_PATTERNS_TO_OMIT][_MAX_PATH];

void PrintLastError(char* lpszFunction);

char* strerror(int errNum)
{  
    const int errMsgSz = 2048;
    static char errMsg[errMsgSz];
    errMsg[0] = 0;
    strerror_s(errMsg, errMsgSz, errNum);
    return errMsg;
}

bool ShouldIgnoreThisFile(WIN32_FIND_DATA ffd)
{
    char lwrStr[_MAX_PATH];

    CopyStringToLowerCaseRemoveAsterisk((const char*)ffd.cFileName, lwrStr);
    for (int i = 0; i < g_numFilePtrnsToOmit; ++i)
    {
        unsigned int tmp = ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        if ((tmp == 0) && strstr(lwrStr, g_filePtrnsToOmit[i]) != 0)
            return true;
    }

    return false;
}

bool ShouldIgnoreThisDir(WIN32_FIND_DATA ffd)
{
    size_t charsConverted;
    char fname[_MAX_PATH];

    wcstombs_s(&charsConverted, fname, _MAX_PATH, ffd.cFileName, _MAX_PATH - 1);
    for (int i = 0; i < g_numDirsToOmit; ++i)
    {
        if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strcmp(g_dirsToOmit[i], fname) == 0)
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

void SearchAllDirectories(const char *raw_in_file)
{
    const int curPathSz = _MAX_PATH;
    char current_path[curPathSz];
    TCHAR all_wildcard[] = L"*";    // search through for all directories
    long fl_cnt = 0;
    const int errStrSz = _MAX_PATH;
    char errStr[errStrSz];
	bool skipThisDir = false;
    static int funcRecursionCnt = 0;
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    try
    {
        funcRecursionCnt++;
        if(_getcwd(current_path, curPathSz) == NULL)
        {
            errStr[0] = 0;
            if (Verbose == 1)
                sprintf_s(errStr, errStrSz, "  Attempting to _getcwd to \"%s\", funcRecursionCnt = %d - ",
                    current_path, funcRecursionCnt);
            throw errStr;
        }

        if ((hFind = FindFirstFile(all_wildcard, &ffd)) == INVALID_HANDLE_VALUE)
            printf("\n  No \"%s\" files in %s", all_wildcard, current_path);
        else
        {
		    do
		    {
                errStr[0] = 0;
                if (skipThisDir = ShouldIgnoreThisDir(ffd))
					continue;
				if(wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0 && wcscmp(ffd.cFileName, L"") != 0)
				{
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
                                    sprintf_s(errStr, errStrSz, "  Attempting to _chdir to \"%s\\%s\", file attrib: %d, funcRecursionCnt: %d - ",
									    current_path, ffd.cFileName, ffd.dwFileAttributes, funcRecursionCnt);
								throw errStr;
							}
						}
					}
					catch (char * excp)
					{
						PrintLastError(excp);
						continue;
					}

					SearchAllDirectories(raw_in_file);

					try
					{
						if(_chdir(current_path) != 0)
						{
							sprintf_s(errStr, errStrSz, "  Attempting to _chdir back to \"%s\" - ", current_path);
							throw errStr;
						}
					}
					catch (char* excp)
					{
						PrintLastError(excp);
						continue;
					}
				}
            } while ((hFind != INVALID_HANDLE_VALUE) && (FindNextFile(hFind, &ffd) != 0));
        }

        if (hFind != INVALID_HANDLE_VALUE)
            FindClose(hFind);

	    if (!skipThisDir)
		    SearchCurrentDirectory(raw_in_file, current_path);
    }
    catch (char* excp)
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

void SearchCurrentDirectory(
    const char *raw_in_file,
    const char *current_path)
{
    long fl_cnt = 0, freq_cntr = 0;
    char cur_path_tmp[_MAX_PATH];
    char buff[_MAX_PATH];
    char errStr[_MAX_PATH];
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    TCHAR raw_in_fileW[_MAX_PATH];
    size_t charsConverted;
    char fname[_MAX_PATH];

    try
    {
        if (current_path[0] != 0)
            strcpy_s(cur_path_tmp, _MAX_PATH, current_path);
        else
        {
            cur_path_tmp[0] = '.';
            cur_path_tmp[1] = 0;
        }

        mbstowcs_s(&charsConverted, raw_in_fileW, _MAX_PATH, raw_in_file, _MAX_PATH - 1);
        errStr[0] = 0;
        //if ((hFind = FindFirstFile(raw_in_file, &ffd)) == INVALID_HANDLE_VALUE)
        if ((hFind = FindFirstFileEx(raw_in_fileW, FindExInfoStandard, &ffd, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH)) == INVALID_HANDLE_VALUE)
        {
            if (Verbose == 1)
                sprintf_s(errStr, _MAX_PATH, "  Error attempting to FindFirstFile in %s\\%s", current_path, raw_in_file);
            throw errStr;
        }

        do
        {
            if (wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0 && !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                if (File_Find)
                {
                    // No search string just print the files that match the file pattern
                    //sprintf_s(buff, _MAX_PATH, "%s\\%s\n", cur_path_tmp, ffd.cFileName);
                    //printf(buff);
                    printf_s("\n%s\\%s", cur_path_tmp, ffd.cFileName);
                    g_numFilesMatchPattern++;
                }
                else if (ShouldIgnoreThisFile(ffd))
				    continue;

                wcstombs_s(&charsConverted, fname, _MAX_PATH, ffd.cFileName, _MAX_PATH - 1);
                freq_cntr = ProcessFile(fname);

				if ((freq_cntr > 0 || Verbose == 1) && (!Prnt_Min || Prnt_Some))
				{
					freq_cntr > 0 ? printf(" +") : printf("  ");
					sprintf_s(buff, _MAX_PATH, "\n%d occurrence(s) in:  %s\\%s", freq_cntr, cur_path_tmp, ffd.cFileName);
					if (g_reverseSlashDir == 1)
						ReverseSlashDirInString(buff);
					printf(buff);
					if (Prnt_Lines || Verbose)
						printf("\n  ---------------------------------------------------------------------------------");
                }
            }
			else if (File_Find && ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0)
			{
				if (!ShouldIgnoreThisDir(ffd))
				{
					printf("\n%s\\%s", cur_path_tmp, ffd.cFileName);
					g_numDirsMatchPattern++;
				}
			}
        } while ((hFind != INVALID_HANDLE_VALUE) && (FindNextFile(hFind, &ffd) != 0));
    }
    catch (char* excp)
    {
        if (excp[0] != 0)
            PrintLastError(excp);
    }

    if (hFind != INVALID_HANDLE_VALUE)
        FindClose(hFind);
}

void PrintLastError(char* msg)
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();
    TCHAR msgW[_MAX_PATH];
    size_t charsConverted;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process
    mbstowcs_s(&charsConverted, msgW, _MAX_PATH, msg, _MAX_PATH - 1);

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen(msgW)+40)*sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        msgW, dw, lpMsgBuf);
    //MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
	fprintf(stderr, "\n  GetLastError: %s", (LPCTSTR)lpDisplayBuf);

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
