/***************************************************************************
//
// Copyright (c) 1995, 1996, 1997, 1998, 1999 Gerhard W. Gross.
//
// THIS SOFTWARE IS PROVIDED BY GERHARD W. GROSS ``AS IS'' AND ANY
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
// ALL OR PARTS OF THIS CODE WERE WRITTEN BY GERHARD W. GROSS, 1995-1999.
//
// Functions that are specific to Linux and other Unices are isolated in
// this file.  There is an analogous file for Windows specific code.  The
// reason that forces seperate code paths is subdirectory recursion.  Unices
// perform variable expansion from the command line by the shell.  Win32
// systems do not, but instead perform variable expansion within the
// "_wfindfirst" and "_wfindnext" functions.
// 
// In order to take advantage of wildcard expansion this program requires
// that the file name given on the command line be enclosed in single or
// double quotes in order to suppress shell wildcard expansion.
***************************************************************************/

#include "freqLin.h"
#include "freq.h"

#include <errno.h>

extern bool Verbose;
extern bool Prnt_Lines;
extern bool Prnt_Min;
extern bool File_Find;
extern bool Prnt_Some;
extern bool Case_Insensitive;
extern bool g_reverseSlashDir;
extern long g_numFilesMatchPattern;
extern long g_numDirsMatchPattern;
extern long g_numFilesAndDirsChecked;
extern long g_numDirsToOmit;
extern char g_dirsToOmit[MAX_DIRS_TO_OMIT][_MAX_PATH];
extern long g_numFilePtrnsToOmit;
extern char g_filePtrnsToOmit[MAX_FILE_PATTERNS_TO_OMIT][_MAX_PATH];
extern char F_Arr[ARSZ_MAX];
extern char g_cur_path[_MAX_PATH];
extern char g_prev_path[_MAX_PATH];
extern char g_errBuf[_MAX_PATH];

// Using extern for Raw_In_File compiles but causes segmentation faults.
// Instead declare global variable here.  Can use const even though
// it gets set in a subsequent function (probably should not work but
// it does so I'll leave it).

const char *g_Raw_In_File;
char f_Arr_NoWild[ARSZ_MAX];
struct _stat64 g_statBuf;

const char STAR      = 42;
const char QUES      = 63;

/***************************************************************************
Private function prototypes
***************************************************************************/

int SelectValidDir(const struct dirent *namelist);
int SelectFileMatch(const struct dirent *namelist);
bool ShouldIgnoreThisDir(const char* fname);
bool IsSensibleFileName(const char* dirName);
void CopyStrNoWildCards(char* F_Arr, char* f_Arr_NoWild);

/***************************************************************************
 These MS functions exist for Windows but not Linux so implement here!
 Returns -1 on error, like if there is not enough room to add null char.
***************************************************************************/

int _chdir(const char* dirPath)
{
    return chdir(dirPath);
}

char* _getcwd(char* dirPath, int sz)
{
    return getcwd(dirPath, sz);
}

int strerror_s(char* buff, int buffSz, int errNum)
{
   buff[0] = 0;
   char* msg = strerror(errno);
   int msgSz = strlen(msg);
   if (msgSz < buffSz)
       strcpy(buff, msg);
   return 0;
}

int strcpy_s(char* dest, size_t destSz, const char* src)
{
    int srcLen = strlen(src);
    if ( destSz - 1 < srcLen) return -1;
    strcpy(dest, src);
    dest[srcLen] = 0;
    return 0;
}

int strncpy_s(char* dest, size_t destSz, const char* src, size_t numBytes)
{
    if ( destSz - 1 < numBytes) return -1;
    strncpy(dest, src, numBytes);
    dest[numBytes] = 0;
    return 0;
}

bool strcmpiQues(const char* arg, int argSz, const char* s2, int s2Sz)
{
    if (argSz != s2Sz)
        return false;

    for (int i = 0; i < argSz; ++i)
    {
        if (arg[i] == QUES) // If wild card char replacement allow any char
            continue;

        if ((Case_Insensitive && tolower(arg[i]) != tolower(s2[i]))
            || (!Case_Insensitive && arg[i] != s2[i]))
        {
            return false;
        }
    }

    return true;
}

const char* strstriQues(const char* haystack, const char* needle, int needleSz)
{
    int i, j;
    int haySz = strlen(haystack);

    for (i = 0, j = 0; i < haySz && j < needleSz; ++i)
    {
        // If wild card char allow any char
        if (needle[j] == QUES)
        {
            ++j;
            continue;
        }

        if ((Case_Insensitive && (tolower(haystack[i]) == tolower(needle[j])))
            || (!Case_Insensitive && (haystack[i] == needle[j])))
            j++;
        else
            j = 0;
    }

    return (j == needleSz ? &haystack[i] : NULL);
}


bool ShouldIgnoreThisFile(const char* fname)
{
    char lwrStr[_MAX_PATH];

    CopyStringToLowerCaseRemoveAsterisk(fname, lwrStr);
    for (int i = 0; i < g_numFilePtrnsToOmit; ++i)
    {
        if (strstr(lwrStr, g_filePtrnsToOmit[i]) != 0)
            return true;
    }

    return false;
}

bool ShouldIgnoreThisDir(const char* fname)
{
    for (int i = 0; i < g_numDirsToOmit; ++i)
    {
        if (strcmp(g_dirsToOmit[i], fname) == 0)
            return true;
    }

    return false;
}

/***************************************************************************
A directory is passed in here
***************************************************************************/

bool IsSensibleFileName(const char* dirName)
{
    bool retVal = false;
    int i;
    int len = strlen(dirName);

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
dependent on the filename, with any wildcards, supplied on the command
line.
***************************************************************************/

void SearchAllDirectories(const char *raw_in_file)
{
    int i;
    int dirCnt = 0;
    char *fName;
    struct dirent **namelist;
    //char fullPath[_MAX_PATH];
    char cur_path[_MAX_PATH];
    errno = 0;

    if(getcwd(cur_path, _MAX_PATH) == NULL)
    {
        OnError(7, 0, __LINE__, NULL, false);
        return;
    }

    // Prevent wierd circular dir strutures/symlinks
    if (strcmp(cur_path, g_prev_path) == 0)
        return;

    errno = 0;
    if ((dirCnt = scandir(cur_path, &namelist, SelectValidDir, alphasort)) == -1)
        OnError(9, 2, __LINE__, cur_path, false);
    strcpy(g_cur_path, cur_path);

    if (dirCnt > 0)
    {
        for (i = 0; i < dirCnt; i++)
        {
            fName = namelist[i]->d_name;

            //if (_stat64(fName, &g_statBuf) != 0)
            //{
            //    OnError(13, 0, __LINE__, fName, false);
            //    continue;
            //}

            // Skip file types such as: fifo, pipe, block file, socket, character file.
//            if ((!S_ISREG(g_statBuf.st_mode) && !S_ISDIR(g_statBuf.st_mode)) || S_ISLNK(g_statBuf.st_mode))
//                continue;

            strcpy(g_prev_path, cur_path);
            if(chdir(fName) != 0)
            {
                OnError(8, 0, __LINE__, fName, false);
                continue;
            }

            SearchAllDirectories(raw_in_file);
            if(chdir(cur_path) != 0)
                OnError(8, 1, __LINE__, cur_path);
        }
    }
    SearchCurrentDirectory(raw_in_file, cur_path);
}

/***************************************************************************
This function is called for every directory that has no subdirectories or
if the -R command line option was not supplied.  The current directory is
searched for files that match the file name, with any wildcards, given at
the command line.  For each of those files, ProcessFile() is called which
does the actual search in the file contents of the command line specified
strings.
***************************************************************************/

void SearchCurrentDirectory(const char *raw_in_file, const char *cur_path)
{
    int i;
    int fileCnt;
    char *fName;
    long freq_cntr = 0;
    struct dirent **namelist;
    char cur_path_tmp[_MAX_PATH];
    char buff[_MAX_PATH];

    g_Raw_In_File      = raw_in_file;
    fileCnt            = 0;
    cur_path_tmp[0]    = 0;

    if (cur_path[0] != 0)
        sprintf(cur_path_tmp, "%s/", cur_path);

    errno = 0;
    if ((fileCnt = scandir(".", &namelist, SelectFileMatch, alphasort)) == -1)
        OnError(9, 1, __LINE__, cur_path_tmp, false);
    else if (fileCnt == 0)
    {
        if (Verbose)
            fprintf(stderr, "  No matching file pattern in \"%s%s\"\n", cur_path_tmp, raw_in_file);
        return;
    }

    for (i = 0; i < fileCnt; i++)
    {
        fName = namelist[i]->d_name;
        if (File_Find)
        {
            // No search string just print the files that match the file pattern
            //swprintf_s(buff, _MAX_PATH, "%s\\%s\n", cur_path_tmp, c_file.name);
            //printf(buff);
            printf("\n%s%s", cur_path_tmp, fName);
            g_numFilesMatchPattern++;
            continue;
        }

        freq_cntr = 0;
        // Don't 'ProcessFile' if can't stat or it is a dir
        if (_stat64(fName, &g_statBuf) != 0)
        {
            OnError(13, 0, __LINE__, fName, false);
            continue;
        }

        if (!S_ISDIR(g_statBuf.st_mode))
            freq_cntr = ProcessFile(fName);

        if ((freq_cntr > 0 || Verbose) && (!Prnt_Min || Prnt_Some))
        {
            freq_cntr > 0 ? printf("\n +") : printf("\n  ");
//            printf("%d occurrence(s) in:  %s%s\n", freq_cntr, cur_path_tmp, fName);

            sprintf(buff, "%ld occurrence(s) in:  %s%s", freq_cntr, cur_path_tmp, fName);
            if (g_reverseSlashDir == 1)
                ReverseSlashDirInString(buff);
            printf(buff);

            if(Prnt_Lines || Verbose)
                printf("\n  ---------------------------------------------------------------------------------\n");
        }
    }
}

/***************************************************************************
Get all directories in current directory except "." and ".."..
***************************************************************************/

int SelectValidDir(const struct dirent *namelist)
{
    int retMatch  = 0;
    const char *fName;

    // Include all directories except "." and ".."

    fName = namelist->d_name;
    errno = 0;
    if (_stat64(fName, &g_statBuf) != 0)
    {
        OnError(13, 0, __LINE__, fName, false);
        return 0;
    }

    if (ShouldIgnoreThisDir(fName))
        return 0;
            
    if (S_ISDIR(g_statBuf.st_mode) && (strcmp(".", fName) != 0) && (strcmp("..", fName) != 0))
        retMatch = 1;

    return retMatch;
}

/***************************************************************************
    Count the occurrences of "chr" in "str" and put the array index number
    of the last occurrence of "chr" in "idx".  Return the number of
    occurrences.
***************************************************************************/

inline int strcnt(const char *str, int chr, int *idx)
{
    int i;
    int strLen;
    int cnt = 0;

    *idx = 0;
    strLen = strlen(str);

    for (i = strLen-1; i >= 0; i--)
    {
        if (str[i] == chr)
        {
            if (cnt == 0)
                *idx = i;
            cnt++;
        }
    }
    return cnt;
}
/***************************************************************************
Determine if the command line file name (with or without "*", "?"
wildcards) matches the passed in file name from the scandir() list.
Return 1 if a match is found and 0 otherwise.

This function takes the place of shell variable expansion of the "*" and
"?" wildcard characters.  The file name on the command line is enclosed
in quotes so the shell does not expand the wildcard characters.  This
makes it possible to search subdirectories recursively.  (When the shell
expands wildcards it uses the files in the current directory as the
selection pool.  Recursion requires that the contents of each recursive
subdirectory also be included in the file pool.)

The _stat64() function fails if the file name parameter is a symbolic link
that points to a non-existent file.  If _stat64() fails the external
variable "errno" gets set to some non-zero value and therefore causes
scandir() to fail.  Therefore, errno is explicitly set to zero if _stat64()
fails so scandir() does not fail needlessly.
***************************************************************************/

int SelectFileMatch(const struct dirent *namelist)
{
    int retMatch         = 0;
    int iC               = 0;
    int iA               = 0;
    int lastIdx          = 0;
    int starCntr         = 0;
    t_BOOL skip            = FALSE;
    const char *argFName = g_Raw_In_File;
    const char *curFName = namelist->d_name;
    const char *pA, *pC, *pALast, *pCLast, *ptr;
    char sub[_MAX_PATH];
    int cnt, len, numStars, numChars;
    size_t argLen;
    size_t curLen;
    bool preStar = true;

    if (ShouldIgnoreThisFile(curFName))
        return 0;

    numStars = strcnt(argFName, STAR, &lastIdx);

    errno = 0;
    if (_stat64(curFName, &g_statBuf) != 0)
    {
        OnError(13, 0, __LINE__, curFName, false);
        skip = TRUE;
        return 0;
    } 
    
//    if (S_ISLNK(g_statBuf.st_mode))  // Do not follow links
//        return 0;

    // This block determines if dirs match the specified pattern. Does not
    // proccess wildcard chars but overlooks them instead.
    if (S_ISDIR(g_statBuf.st_mode))
    {
        if ((strcmp(".", curFName) != 0) && (strcmp("..", curFName) != 0))
        {
            g_numFilesAndDirsChecked++;
            CopyStrNoWildCards(F_Arr, f_Arr_NoWild);
            if (strstr(curFName, f_Arr_NoWild))
            {
                g_numDirsMatchPattern++;
                return 1;
            }
        }
    }


    // Only accept "REG"ular files and symlinks (remove if they cause problems!)
    //if (!skip && S_ISREG(g_statBuf.st_mode))
    if (!skip && (S_ISREG(g_statBuf.st_mode) || S_ISLNK(g_statBuf.st_mode)))
    {
        retMatch = 1;
        argLen = strlen(argFName);
        curLen = strlen(curFName);
        g_numFilesAndDirsChecked++;

        // First handle case where there are no stars
        if (numStars == 0)
            return strcmpiQues(argFName, argLen, curFName, curLen) ? 1 : 0;

        pA = argFName;
        pC = curFName;
        pALast = pA + argLen - 1;
        pCLast = pC + curLen - 1;

        while (pA <= pALast)
        {
            cnt = strcspn(pA, "*");
            if (cnt == 0)
            {
                pA++;
                preStar = false;
                continue;
            }

            // Copy next chars from cur arg posn up to char before next star or EOL
            strncpy(sub, pA, cnt);
            sub[cnt] = 0;

            if (preStar)  // Handle chars before any stars
            {
                len = curLen > cnt ? cnt : curLen;
                if (!strcmpiQues(pA, cnt, pC, len))
                    return 0;
                pC += cnt;
            }
            else if (pA[cnt] != STAR)  // Handle no star at end of search substring
            {
                // Match exactly last cnt chars of strings
                len = curLen > cnt ? cnt : curLen;
                if (!strcmpiQues(pALast - cnt + 1, cnt, pCLast - cnt + 1, len))
                    return 0;
            }
            else // Handle all other cases, star at begin and end of search substring
            {
                if ((pC = strstriQues(pC, sub, cnt)) == NULL)
                    return 0;
            }

            pA += cnt + 1;
            preStar = false;
        }

    }

    return retMatch;
}

void CopyStrNoWildCards(char* F_Arr, char* f_Arr_NoWild)
{
    int i, k;
    for (i = 0, k = 0; F_Arr[i] != 0; ++i)
    {
        if (F_Arr[i] != STAR && F_Arr[i] != QUES)
            f_Arr_NoWild[k++] = F_Arr[i];
    }
    f_Arr_NoWild[k] = 0;
}
