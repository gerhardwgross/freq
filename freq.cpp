/* **************************************************************************
// freq -- utility program written by Gerhard Gross (PSU '96 - '99)
//
// Copyright (c) 1995 - 2006 Gerhard W. Gross.
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
//
// Function: Counts the frequency of a com line arg in ASCII ints or chars.
//
// Usage:     freq [-iplfvwcWPRUT] str filename
//
// Program: Opens source_file for I/O.
//   Reads file size and creates 2 new char arrays dynamically.
//   Reads entire file contents into src_buf[].
//   Searches for and counts the number of occurrences of the supplied
//   sequence.    The sequence can be in regular text, or in s sequence
//   of one or more ASCII integers.    If more than one integer is
//   supplied, they need to be separated by commas with no spaces
//   and the whole sequence needs to be put in quotes.
//   A search string
//   can begin with a hyphen '-' without fear of it being treated as
//   an option declaration if the hyphen is directly followed by a
//   slach character ('/' ASCII 45).    The slach character is then
//   removed from the string and the other characters are
//   appropriately shifted over.    The '-w' option is the Wildcard
//   option as described in 'rplc.c'.    It causes the program to
//   interpret the '?' character as a wildcard that represents
//   any character.    The -c option sets the case insensitive flag
//   and -R recursively searchs all subdirectories.
//
// Portability:
//   The common functions are housed in freq.c while system specific
//   functions are in freqWin.c and freqLin.c.
//
// *************************************************************************/


#undef _MAX_PATH
#define _MAX_PATH (260 * 2)
#ifdef _WIN32
#include "freqWin.h"
#else
#include "freqLin.h"
#endif

#include<ctype.h>
#include <errno.h>
#include <fstream>
#include "freq.h"

using namespace std;

/***************************************************************************
Global variables and macros
***************************************************************************/

#define SPACE_ASCII_INT             32
#define TAB_ASCII_INT               9
#define COMMA_ASCII_INT             44
#define ASTERISK_ASCII_INT          42
#define QUESTION_MARK_ASCII_INT     63
#define FORWARD_SLASH               47
#define BACKWARD_SLASH              92

#define BYTES_DEF_NUM_XTRA_LNS      3
#define BYTES_DEF_NUM_COLS          100

TCHAR Raw_In_File[_MAX_PATH];
TCHAR F_Arr[ARSZ_MAX];
TCHAR F_Arr_Case[ARSZ_MAX];
TCHAR Raw_Search_String[_MAX_PATH];
long Num_Search_Chars               = 0;
bool Integer                        = false;
bool WildCard                       = false;
bool Prnt_Lines                     = false;
bool Prnt_Min                       = false;
bool Prnt_Some                      = false;
bool File_Find                      = false;
bool Case_Insensitive               = false;
bool Verbose                        = false;
bool WholeWord                      = false;
bool SuppressErrorsPrintout         = true;
bool Search_Subdirectories          = false;
bool Search_Single_File             = false;
int g_maxLineWidth                  = BYTES_DEF_NUM_COLS;   /* max chars to print per line */
int g_textEncoding                  = FREQ_EIGHT_BIT_ASCII;
bool g_printFileType                = false;
bool g_reverseSlashDir              = false;
bool g_noWrapNewLine                = false;
int g_numXtraLnsToPrnt              = 0;
long g_freqCntTotal                 = 0;
long g_filesWithHitsCntr            = 0;
long g_filesSearchedCntr            = 0;
long g_numFilesMatchPattern         = 0;
long g_numDirsMatchPattern			= 0;
long g_numFilesAndDirsChecked       = 0;
long g_numFailedChdir               = 0;
long g_numDeadSymLinks              = 0;

long g_numDirsToOmit = 0;
long g_numFilePtrnsToOmit = 0;
TCHAR g_dirsToOmit[MAX_DIRS_TO_OMIT][_MAX_PATH];
TCHAR g_filePtrnsToOmit[MAX_FILE_PATTERNS_TO_OMIT][_MAX_PATH];
TCHAR g_cur_path[_MAX_PATH];
TCHAR g_prev_path[_MAX_PATH];
TCHAR g_errBuf[_MAX_PATH];

/***************************************************************************
Private function prototypes
***************************************************************************/

void SearchSingleFile(TCHAR* raw_in_file);
int GetConsoleWidth();
long Search_File(size_t szFile, TCHAR *fname);
void deal_with_args(int argc, TCHAR*argv[], TCHAR *working_dir);
void seperate_filename_and_path(
    TCHAR *path_and_file_name,
    TCHAR *working_dir,
    TCHAR *raw_in_file);
void fill_reverse_case_array(TCHAR *f_arr_case, TCHAR*f_arr);
int deal_with_options(int arrgc, TCHAR*arrgv[]);
int measure_file(TCHAR* fname, size_t& fileSize);
int read_file(TCHAR *s_buf, size_t sz, TCHAR *fname);
void shift(TCHAR *str);
int GetTypeOfFile(const TCHAR* src, long szFile);
int count(TCHAR *str, TCHAR *arr);
void print_usage();

/***************************************************************************
MAIN
***************************************************************************/

int wmain(int argc, TCHAR* argv[])
{
    TCHAR original_calling_dir[_MAX_PATH];
    TCHAR working_dir[_MAX_PATH];
    TCHAR Reset_Dir;

    working_dir[0]  = 0;
    g_prev_path[0]  = 0;
    g_cur_path[0]   = 0;
    Reset_Dir       = 0;


    g_maxLineWidth = GetConsoleWidth();

    argc = deal_with_options(argc, argv);
    deal_with_args(argc, argv, working_dir);

    if(_wgetcwd(original_calling_dir, _MAX_PATH) == NULL)
        OnError(7, 1, __LINE__, NULL, true);

    if (working_dir[0] != 0)
    {
        if(_wchdir(working_dir) != 0)
            OnError(8, 2, __LINE__, working_dir, false);
        Reset_Dir = 1;
    }

    if (Search_Single_File)
        SearchSingleFile(Raw_In_File);
    else if(Search_Subdirectories == 1)
        SearchAllDirectories(Raw_In_File);
    else
        SearchCurrentDirectory(Raw_In_File, working_dir);

    if (File_Find )
        if (Prnt_Min)
            printf("\n");
        else
            printf("\n -- Matching: files - %ld, dirs - %ld, files/dirs checked: %ld, num failed chdir: %ld, dead symlinks: %ld\n",
            g_numFilesMatchPattern, g_numDirsMatchPattern, g_numFilesAndDirsChecked, g_numFailedChdir, g_numDeadSymLinks);
    else //if (Verbose || Prnt_Lines || Prnt_Min)
    {
        // Print the total of all occurrences for all files searched
        printf("\n -- Found %ld matches in %ld files, %ld files searched, %ld files/dirs checked, %ld dead symlinks.\n",
            g_freqCntTotal, g_filesWithHitsCntr, g_filesSearchedCntr, g_numFilesAndDirsChecked, g_numDeadSymLinks);
    }

    if (Reset_Dir == 1)
    {
        if(_wchdir(original_calling_dir) != 0)
            OnError(8, 3, __LINE__, original_calling_dir, false);
    }

    // Debugging line
    //_wsystem(L"pause");
    return 0;
}

/***************************************************************************
Get the width, in characters, of the console window in which this program
is running.
***************************************************************************/

int GetConsoleWidth()
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns, rows;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    // Must subtract 16 for some reason to get this to format the best
    return ((columns - 16) < 10 ? 150 : columns - 16);
#endif
#ifndef _WIN32
    return BYTES_DEF_NUM_COLS;
#endif
}

void SearchSingleFile(TCHAR* raw_in_file)
{
    TCHAR buff[_MAX_PATH];

    long freq_cntr = ProcessFile(raw_in_file);

    if ((freq_cntr > 0 || Verbose == 1) && (!Prnt_Min || Prnt_Some))
    {
        //freq_cntr > 0 ? printf(" +") : printf("  ");
        swprintf_s(buff, _MAX_PATH, L"\n%d occurrence(s) in:  %s", freq_cntr, raw_in_file);
        if (g_reverseSlashDir == 1)
            ReverseSlashDirInString(buff);
        wprintf(buff);
        if (Prnt_Lines || Verbose)
            printf("\n  ---------------------------------------------------------------------------------\n");
    }
}

/***************************************************************************
The file access and size is checked before searching the file. If the
file is busy (used by another process) it can't be opened.
***************************************************************************/

long ProcessFile(TCHAR *fname)
{
    size_t szFile, freq_cntr;

    int retVal = measure_file(fname, szFile);

    if(szFile == 0)
    {
        if (Verbose)
            fwprintf(stderr, L"\n    0 bytes in file \"%s\"", fname);
        freq_cntr = -1;
    }
    else if(retVal < 0)
        freq_cntr = -1;
    else
        freq_cntr = Search_File(szFile, fname);

    return freq_cntr;
}

/***************************************************************************
    This function is the workhorse of the program. The contents of the
    file are checked for occurences of the search string.
***************************************************************************/

long Search_File(size_t szFile, TCHAR *fname)
{
    TCHAR *src_buf, lin_buf[MAX_LINE_SZ+1];
    long i = 0, k = 0, freq_cntr = 0, line_cntr = 1, col_num = 1,
        char_cntr;
    size_t lnsz;
    int fileType;
    const TCHAR endLineMrk[]   = {CR, NL, 0};
    const TCHAR newLineAsStr[2] = {NL, 0};
    const int errStrSz = _MAX_PATH;
    TCHAR errStr[errStrSz];

    src_buf = new TCHAR[szFile + 1];
    if (src_buf == nullptr)
    //if((src_buf = (TCHAR*)malloc(szFile + 1)) == (void*)NULL)
    {
        _snwprintf_s(errStr, errStrSz, L"%ls, size: %lu", fname, (int)(szFile + 1));
        OnError(4, 1, __LINE__, fname, false);
        return 0;
    }

    if (read_file(src_buf, szFile, fname) != UTIL_SUCCESS)
        DO_CLEANUP(Zero);

    // See if this is a binary or text file
    fileType = GetTypeOfFile(src_buf, szFile);

    if (g_printFileType)
        wprintf(L"\n   -- File type is %ls (%ls) --", (fileType == FREQ_TEXT_FILE ? L"text" : L"binary"), fname);

    for(i = 0; i < szFile; i++, col_num++)
    {
        k = 0;

        if(src_buf[i] == NL)            /* count the number of lines. */
        {
            line_cntr++;
            col_num = 0;                /* reset the column counter. */
        }

        if (g_noWrapNewLine)
        {
            while ((long)(lnsz = wcscspn(&src_buf[i], endLineMrk)) < Num_Search_Chars)
            {
                // Search string can not span over multiple lines.
                // Jump char counter to char past the next end of line markers
                i += lnsz;
                while (src_buf[i] == CR || src_buf[i] == NL)
                    i++;

                // End loop if past end of file
                if (i >= szFile)
                    break;
            }
        }

        // ****************************************************************
        // This is the core of the search functionality!
        // The search string (F_Arr) is compared in this "if" code block to
        // the source string (src_buf) char by char for a match.
        // ****************************************************************
        if((src_buf[i] == F_Arr[0]) || (WildCard && (F_Arr[0] == WC1)) ||
            (Case_Insensitive && (src_buf[i] == F_Arr_Case[0])))
        {
            for(k = 1;
                k < Num_Search_Chars &&
                ( (src_buf[i+k] == F_Arr[k]) ||
                (WildCard && (F_Arr[k] == WC1)) ||
                (Case_Insensitive && (src_buf[i+k] == F_Arr_Case[k])) );
            k++)
            {;}
        }

        if(k == Num_Search_Chars && WholeWord)
        {
            if( ( (i != 0) && ( isalpha(src_buf[i - 1]) || src_buf[i-1] == '_' )) ||
                ( (i + k) < (szFile) && ( isalpha(src_buf[i + k]) || src_buf[i + k] == '_' ))
                )
                k = Num_Search_Chars + 2;
        }
        if(k == Num_Search_Chars)  // Found a match
        {
            freq_cntr++;
            g_freqCntTotal++;
            if(Verbose)
                printf("\n            Char #: %ld,    Line #: %ld,    column#: %ld", i + 1, line_cntr, col_num);

            k = i;
            if(Prnt_Lines || Prnt_Min || Prnt_Some)
            {
                if (fileType == FREQ_TEXT_FILE)
                {
                    if (src_buf[k] == NL || src_buf[k] == CR)
                        ++k;
                    else
                        while(k > 0 && src_buf[k - 1] != NL)
                            --k;

                    for (int n = 0; n <= g_numXtraLnsToPrnt && k < szFile; ++n)
                    {
                        char_cntr = 0;
                        lnsz = wcscspn(&src_buf[k], endLineMrk);
                        lnsz = lnsz > MAX_LINE_SZ ? MAX_LINE_SZ : lnsz;

                        wcsncpy_s(lin_buf, MAX_LINE_SZ+1, &(src_buf[k]), lnsz);
                        lin_buf[lnsz] = 0;
                        if (!Prnt_Min && !Prnt_Some)
                            printf("   line %5ld: ", line_cntr + n);

                        while(lin_buf[char_cntr] != 0)
                        {
                            printf("%c", lin_buf[char_cntr++]);

                            if(Prnt_Lines == 1 && ((char_cntr % g_maxLineWidth) == 0) && (lin_buf[char_cntr] != 0))
                                printf("\n               ");
                        }

                        // Add a new line if printing multiple lines after search string line
                        if (n < g_numXtraLnsToPrnt)
                            printf("\n");

                        // In case more lines are to be printed after the line containing the
                        // search string, move just beyond the next line delimiter.
                        k += lnsz;
                        k += wcscspn(&src_buf[k], newLineAsStr) + 1;

                        // Get past end of line characters
                        if (src_buf[k] == NL || src_buf[k] == CR)
                            k++;

                    }

                }
                else if (fileType == FREQ_BINARY_FILE)
                {
                    char_cntr = 0;

                    // Binary file so can't rely on newline chars and
                    // don't want to print non-printable chars.
                    printf("   byte %8ld: ", k + 1);
                    for (; k < szFile && char_cntr++ < g_maxLineWidth; k++)
                        wprintf(L"%c", (src_buf[k] > -1 && isprint(src_buf[k])) ? src_buf[k] : 32);

                    // Draw second line
                    printf("\n                  ");
                    char_cntr = 0;
                    for (; k < szFile && char_cntr++ < g_maxLineWidth; k++)
                        wprintf(L"%c", (src_buf[k] > -1 && isprint(src_buf[k])) ? src_buf[k] : 32);
                }
                printf("\n");
                i += Num_Search_Chars;
            }
        }
    }

    // Increment counters
    g_filesSearchedCntr++;

    if (freq_cntr > 0)
        g_filesWithHitsCntr++;

    CLEANUP_LABEL(Zero)

    //free(src_buf);
    delete[] src_buf;
    return freq_cntr;
}

/***************************************************************************
The command line arguments (except options) are parsed and _read into
local or global variables.    The number of characters in the search
string (or integers if the '-i' option is used) is determined and
returned.
***************************************************************************/

void deal_with_args(int argc, TCHAR *argv[], TCHAR working_dir[])
{
    int i, j, len;

    // If just listing the files that match the file pattern or computing the file type, can skip most of this stuff

    if(argc == 1)
        OnError(99, 1, __LINE__, NULL, true);
    else if(argc != 2 && (File_Find || g_printFileType))
        OnError(3, 1, __LINE__, NULL, true);
    else if(argc != 3 && !File_Find && !g_printFileType)
        OnError(3, 1, __LINE__, NULL, true);

    wcscpy_s(Raw_Search_String, _MAX_PATH, argv[1]);

    if(Integer)
        Num_Search_Chars = count(argv[1], F_Arr);
    else if (g_textEncoding == FREQ_EIGHT_BIT_ASCII)
    {
        // Make sure string not too large
        if (wcslen(argv[1]) >= ARSZ_MAX)
            OnError(12, 1, __LINE__, NULL);

        // Fill find array and get number of chars
        wcscpy_s(F_Arr, ARSZ_MAX, argv[1]);
        Num_Search_Chars = wcslen(F_Arr);
    }
    else if (g_textEncoding == FREQ_UNICODE)
    {
        // Make sure string not too large
        if ( (len = wcslen(argv[1])) >= ARSZ_MAX / 2)
            OnError(12, 1, __LINE__, NULL);

        // Put a zero behind every character in search
        // string (Unicode).
        for (i = 0, j = 0; i < len; i++, j++)
        {
            F_Arr[j++]  = argv[1][i];
            F_Arr[j]    = 0;
        }

        Num_Search_Chars = len * 2;
    }

    seperate_filename_and_path(argv[argc - 1], working_dir, Raw_In_File);

    //    strcpy_s(Raw_In_File, argv[argc - 1]);

    if(Case_Insensitive)
        fill_reverse_case_array(F_Arr_Case, F_Arr);
}

/***************************************************************************
For cases when the Case_Insensitive option is used this function
fills a char array with the reverse case of any alphabetic characters
that are in the search array (f_arr).
***************************************************************************/

void fill_reverse_case_array(TCHAR *f_arr_case, TCHAR *f_arr)
{
    long i;
    for(i = 0; i < Num_Search_Chars; i++)
    {
        if(f_arr[i] >= 65 && f_arr[i] <= 90)        // uppercase
            f_arr_case[i] = f_arr[i] + 32;
        else if(f_arr[i] >= 97 && f_arr[i] <= 122)    // lowercase
            f_arr_case[i] = f_arr[i] - 32;
        else
            f_arr_case[i] = f_arr[i];                // non-alphabetic
    }
}

/***************************************************************************
This function checks all args to see if they begin with a hyphen.
If so the necessary flags are set.    argc and argv[] are adjusted
accordingly, set back to the condition of the option not having been
supplied at the com line (i.e. all except the first argv[] ptr are
bumped back in the array).
***************************************************************************/

int deal_with_options(int arrgc, TCHAR *arrgv[])
{
    long i, j, num_opts;
    TCHAR lwrStr[_MAX_PATH], *ptr;
    for(j = 1; j < arrgc; j++)
    {
        if(*arrgv[j] == '-')
        {
            if(*(arrgv[j] + 1) == '/')
                shift(arrgv[j]);
            else
            {
                num_opts = wcslen(arrgv[j]) - 1;
                for(i = 1; i <= num_opts; i++)
                {
                    switch(*(arrgv[j] + i))
                    {
                    case 'i':
                        Integer = true;
                        break;
                    case 'w':
                        WildCard = true;
                        break;
                    case 'p':
                        Prnt_Lines = true;
                        break;
                    case 'P':
                        Prnt_Min = true;
                        g_maxLineWidth = 100000;
                        break;
                    case 'l':
                        Prnt_Some = true;
                        g_maxLineWidth = 100000;
                        break;
                    case 'f':
                        File_Find = true;
                        break;
                    case 'c':
                        Case_Insensitive = true;
                        break;
                    case 'v':
                        Verbose = true;
                        break;
                    case 'W':
                        WholeWord = true;
                        break;
                    case 'e':
                        SuppressErrorsPrintout = false;
                        break;
                    case 'R':
                        Search_Subdirectories = true;
                        break;
                    case 'U':
                        g_textEncoding = FREQ_UNICODE;
                        break;
                    case 'T':
                        g_printFileType = true;
                        break;
                    case 's':
                        g_reverseSlashDir = true;
                        break;
                    case 'n':
                        g_noWrapNewLine = true;
                        break;
                    case 'm':
                        // This option must be succeeded with an integer and
                        // then a space. Convert string integer to numeric value.
                        // Implies print lines.
                        g_numXtraLnsToPrnt = wcstol(arrgv[j] + i + 1, NULL, 0) - 1;
                        Prnt_Lines = true;
                        g_maxLineWidth = 100000;

                        // Error check
                        if (g_numXtraLnsToPrnt < 1)
                            g_numXtraLnsToPrnt = BYTES_DEF_NUM_XTRA_LNS;

                        // Now move loop counter to end of this option
                        i = num_opts;
                        break;
                    case 'o':
                        // This option must be succeeded with a space, then a string and then a space
                        if (g_numFilePtrnsToOmit + 1 >= MAX_FILE_PATTERNS_TO_OMIT)
                            fwprintf(stderr, L"\nOmitting too many file patterns - max %d", MAX_FILE_PATTERNS_TO_OMIT);
                        CopyStringToLowerCaseRemoveAsterisk((arrgv[j] + i + 2), lwrStr);
                        wcscpy_s(g_filePtrnsToOmit[g_numFilePtrnsToOmit++], _MAX_PATH, lwrStr);

                        // Consume the directory name, decrement number of args, and move the loop counter
                        for(i = j + 1; i < arrgc - 1; i++)
                            arrgv[i] = arrgv[i + 1];
                        arrgc--;
                        i = num_opts;
                        break;
                    case 'O':
                        // This option must be succeeded with a space, then a string and then a space
                        if (g_numDirsToOmit + 1 >= MAX_DIRS_TO_OMIT)
                            fwprintf(stderr, L"\nOmitting too many directories - max %d", MAX_DIRS_TO_OMIT);
                        CopyStringToLowerCaseRemoveAsterisk((arrgv[j] + i + 2), lwrStr);
                        wcscpy_s(g_dirsToOmit[g_numDirsToOmit++], _MAX_PATH, lwrStr);

                        // Consume the directory name, decrement number of args, and move the loop counter
                        for (i = j + 1; i < arrgc - 1; i++)
                            arrgv[i] = arrgv[i + 1];
                        arrgc--;
                        i = num_opts;
                        break;
                    case 'd':
                        // This option must be succeeded with an integer and
                        // then a space. Convert string integer to numeric value.
                        g_maxLineWidth = wcstol(arrgv[j] + i + 1, &ptr, 10);

                        // Error check
                        if (g_maxLineWidth < 0)
                            g_maxLineWidth = BYTES_DEF_NUM_COLS;

                        // Now move loop counter to end of this option
                        i = num_opts;
                        break;
                    default:
                        OnError(5, 1, __LINE__, arrgv[j]);
                        break;
                    }
                }
                for(i = j; i < arrgc - 1; i++)
                    arrgv[i] = arrgv[i + 1];
                arrgc--;
                j--;
            }
        }
    }
    return arrgc;
}

/********************************************************************************
Seperate the file name to search through from any path given with the file name.
*********************************************************************************/

void seperate_filename_and_path(
    TCHAR *path_and_file_name,
    TCHAR *working_dir,
    TCHAR *raw_in_file)
{
    const TCHAR *cPtr;

    // Replace any backslash with forward slash to make dir and file name separation easier.
    // Also find if there are any wildcard chars present (*, ?)
    size_t sz = wcslen(path_and_file_name);
    bool has_wildcard_char = false;
    for (size_t i = 0; i < sz; ++i) {
        if (path_and_file_name[i] == BACKWARD_SLASH)
            path_and_file_name[i] = FORWARD_SLASH;
        else if (path_and_file_name[i] == ASTERISK_ASCII_INT || path_and_file_name[i] == QUESTION_MARK_ASCII_INT)
            has_wildcard_char = true;
    }

    // Find last path delimiter
    cPtr = wcschr(path_and_file_name, FORWARD_SLASH);

    if (cPtr != nullptr)
    {
        // If there is a dir and file name, with no wildcards, search just that file
        if (!has_wildcard_char) {
            Search_Single_File = true;
            wcscpy_s(raw_in_file, _MAX_PATH, path_and_file_name);
            working_dir[0] = 0;
        }
        else {
            wcscpy_s(raw_in_file, _MAX_PATH, cPtr + 1);
            wcsncpy_s(working_dir, _MAX_PATH, path_and_file_name, cPtr - path_and_file_name);
            working_dir[cPtr - path_and_file_name] = 0;
        }
    }
    else
        wcscpy_s(raw_in_file, _MAX_PATH, path_and_file_name);
}

/***************************************************************************
This function measures and returns the number of bytes in the
file whose name is currently stored in the global string 'fname'.
***************************************************************************/

int measure_file(TCHAR *fname, size_t& fileSize)
{
/*
    long sze;
    errno = 0;
    ifstream inFile;

    inFile.open(fname, ios::in | ios::binary);
    if(errno != 0)
    {
        fwprintf(stderr, "Can't _sopen_s \"%s\".  File may be busy ", fname);
        fwprintf(stderr, "or have restricted access; errno = %d\n", errno);
        sze = -1;
    }
    else
    {
        inFile.seekg(0, ios::end);
        sze = inFile.tellg();
    }
    inFile.close();
    return sze;
*/

    struct _stat64 statBuf;
    errno = 0;

    if (_wstat64(fname, &statBuf) != 0)
    {
        OnError(13, 0, __LINE__, fname, false);
        return -1;
    }

    fileSize = static_cast<size_t>(statBuf.st_size);
    return 0;
}

/***************************************************************************
This function opens the file whose name is stored in fname, reads 'sz'
number of bytes from this file and stores this data in s_buf.
***************************************************************************/

int read_file(TCHAR *s_buf, size_t sz, TCHAR *fname)
{
    int ret = UTIL_SUCCESS;
    wifstream inFile;
    errno = 0;

    inFile.open(fname, ios::in | ios::binary);
    if(errno != 0)
    {
        OnError(13, 0, __LINE__, fname, false);
        ret = UTIL_ERROR;
    }
    else
    {
        inFile.read(s_buf, sz);
        if (inFile.gcount() != sz)
        {
            OnError(11, 1, __LINE__, fname, false);
            ret = UTIL_ERROR;
        }
        s_buf[sz] = 0;
    }


    inFile.close();
    return ret;
}

/***************************************************************************
This function removes the second character from the passed string. This
is called from deal_with_options() to remove the special purpose slash
character.
***************************************************************************/

void shift(TCHAR *str)
{
    int i, len;
    len = wcslen(str);
    for(i = 1; i < len; i++)
        str[i] = str[i + 1];
}

/***************************************************************************
Determine if file is text or binary by looking through first block
of chars for non-printing chars or not CR, NL, or NP.
***************************************************************************/

int GetTypeOfFile(const TCHAR* src, long szFile)
{
    int i;
    const int charsCheck    = 1000;
    int result              = FREQ_TEXT_FILE;
    int startByte           = szFile > 10 ? 5 : 0;

    // When checking, skip first several bytes because .NET puts binary info
    // in them for some reason.
    for (i = startByte; i < charsCheck && i < szFile; i++)
    {
        if (src[i] >= 0 && src[i] < 128 && !isprint(src[i]) && src[i] != NL && src[i] != CR &&
            src[i] != NP && src[i] != TB)
        {
            result = FREQ_BINARY_FILE;
            i = charsCheck;
        }
    }

    return result;
}

/***************************************************************************
This function counts the number of integers in the command line
arguments and puts them in an array (arr) declared in main.  The
arguments should be separated by commas, spaces, or tabs.

The char array with the com line string is passed in as str[].  Each
of the delimited integer entries are converted to an ASCII int and
stored in a single byte.  These bytes are stored together in the char
array arg arr[].  The number of integers in the input string (and
consequently the number of bytes in the output string arr[]) is
returned by the func.
***************************************************************************/

int count(TCHAR *str, TCHAR *arr)
{
    TCHAR *ptrs[ARSZ_MAX];
    int i = 0, j = 0, num, cnt = 0;

    // Remove leading whitespace (and commas)
    while ( (str[i] == SPACE_ASCII_INT ||
        str[i] == TAB_ASCII_INT   ||
        str[i] == COMMA_ASCII_INT  ) && str[i] != 0)
    { i++; }
    ptrs[j++] = &str[i];
    if(wcslen(ptrs[j-1]) > 0)
        cnt = 1;
    while(str[i] != 0)
    {
        i++;
        // Get next integer when encounter delimiter (comma, space, tab)
        if(str[i] == SPACE_ASCII_INT ||
            str[i] == TAB_ASCII_INT   ||
            str[i] == COMMA_ASCII_INT   )
        {
            i++;
            // Remove delimiters (spaces, commas, tabs)
            while ( (str[i] == SPACE_ASCII_INT ||
                str[i] == TAB_ASCII_INT   ||
                str[i] == COMMA_ASCII_INT  ) && str[i] != 0)
            { i++; }
            if (str[i] != 0)
            {
                ptrs[j++] = &str[i];
                cnt++;
            }
        }
    }
    for(i = 0; i < cnt; i++)
    {
        num = wcstol(ptrs[i], NULL, 0);
        if(num >= 0 && num < 256)
            arr[i] = (char)num;
        else
            OnError(1, 1, __LINE__, str);
    }
    return cnt;
}

/*
ORIGINAL FREQ FUNCTION REPLACED BY MORE GENERAL VERSION FROM RPLC ABOVE
int count(char *str, char *arr)
{
char *ptrs[ARSZ_MAX];
int i = 0, j = 0, cnt = 0, num;
ptrs[j++] = str;
if(strlen(str))
cnt = 1;
while(str[i] != 0)
{
if(str[i++] == ',')
{
ptrs[j++] = &str[i];
cnt++;
}
}
for(i = 0; i < cnt; i++)
{
num = strtol(ptrs[i], NULL, 0);
if(num >= 0 && num < 256)
arr[i] = (char)num;
else
OnError(1, 1, __LINE__, str);
}
return cnt;
}
*/

/***************************************************************************
The direction of the separator slash in the passed in string is
reversed. If a forward slash (/) is encounterd it is replaced with a
backward slash (\) and vice versa.
***************************************************************************/
void ReverseSlashDirInString(TCHAR* buff)
{
    int i = 0;

    while (buff[i] != 0)
    {
        if (buff[i] == '\\')
            buff[i] = '/';
        else if (buff[i] == '/')
            buff[i] = '\\';
        i++;
    }
}

void CopyStringToLowerCaseRemoveAsterisk(const TCHAR* origStr, TCHAR* lwrCaseStr)
{
    int i = 0, j = 0;
    while (origStr[i] != 0)
    {
        if (origStr[i] != 42)
            lwrCaseStr[j++] = tolower(origStr[i]);
        i++;
    }
    lwrCaseStr[j] = 0;
}

/***************************************************************************
This function reports errors and usage text to the output screen.
If the interger arg passed is 99, just print the usage message and
no error.    exit() is called from here.
***************************************************************************/

void OnError(int i, int x, int line, const TCHAR *str, bool exitApp)
{
    if (SuppressErrorsPrintout) {
        if (exitApp) {
            fwprintf(stderr, L"\n");
            print_usage();
            exit(1);
        }
        return;
    }

    fflush(stdout);
    if(i != 99)
        fwprintf(stderr, L"\nError %d-%d, line:%d - ", i, x, line);
    switch(i)
    {
    case 1:
        fwprintf(stderr, L"The first two arguments must contain one or more integers ranging from 1 - 255");
        fwprintf(stderr, L"\n  inclusive, separated by commas (no spaces)!");
        fwprintf(stderr, L"\n  If an argument has two or more integers, the whole argument must be put between quotes.");
        break;
    case 2:
        fwprintf(stderr, L"Opening file \"%s/%s\", errno: %s", g_cur_path, str, ErrNoMsg());
        break;
    case 3:
        fwprintf(stderr, L"Wrong number of arguments.");
        break;
    case 4:
        fwprintf(stderr, L"There is not enough memory for files of this size - %s", str);
        fwprintf(stderr, L"\n  Split the file into one or more separate files and try again on each file.\n");
        break;
    case 5:
        fwprintf(stderr, L"Undefined option.");
        break;
    case 7:
        fwprintf(stderr, L"System call getcwd(), errno: %s.", ErrNoMsg());
        break;
    case 8:
        fwprintf(stderr, L"System call chdir(%s/%s),  errno: %s", g_cur_path, str, ErrNoMsg());
        g_numFailedChdir++;
        break;
    case 9:
        fwprintf(stderr, L"System call scandir(%s/%s), errno: %s", g_cur_path, str, ErrNoMsg());
        break;
    case 10:
        fwprintf(stderr, L"System call chmod(%s/%s), errno: %s", g_cur_path, str, ErrNoMsg());
        break;
    case 11:
        fwprintf(stderr, L"Error reading %s/%s, errno: %s", g_cur_path, str, ErrNoMsg());
        break;
    case 12:
        fwprintf(stderr, L"Search string too large.");
        break;
    case 13:
        fwprintf(stderr, L"System call stat(%s/%s)  errno: %s", g_cur_path, str, ErrNoMsg());
        g_numDeadSymLinks++;
        break;
    }
    //fwprintf(stderr, "\n");

    if (exitApp)
    {
        fwprintf(stderr, L"\n");
        print_usage();
        exit(1);
    }
    errno = 0;
}

/***************************************************************************
This function prints notes to screen.
***************************************************************************/

void print_usage()
{
    fwprintf(stderr,
        L"\n\
    Usage:  freq [-ipvPlnmdfwoOcWeRUsT] str filename\n\
    \n\
    Searches 'filename' for occurrences of 'str'.\n\
    \n\
    -i integer. 'str' must be one or more ASCII integers (0 - 255)\n\
       separated by commas, no spaces.  The integers in 'str'\n\
       must by enclosed in quotes if there are more than one.\n\
    -p print text containing string 'str' and other info.\n\
    -v verbose. Print text containing string 'str' and even more info.\n\
    -P Print only line of text that contains string 'str'.\n\
    -l Same as -P but also prints filename (no path) on next line.\n\
    -n Search string must not span multiple lines.\n\
    -mXX Print XX-1 lines after the line containing the search string (text files only).\n\
    -dXX Set width of output (number of chars across screen = XX)\n\
    -f find file/dir command. Does not accept search string. Prints all files that match\n\
       the file pattern. Case insensitive option not possible on Linux.\n\
    -w wildcard char '?' in \'str\' represents any single character.\n\
    -o omit file pattern from being searched (-o *.dll). Can supply this option multiple times.\n\
    -O omit directory from being searched (-O DirName). Can supply this option multiple times.\n\
    -c case insensitive. Insensitive to case of search string.\n\
    -W whole word. Searches for whole word occurrences only.\n\
    -e Print any error statements to console, suppress by default.\n\
    -R recursive. Recursively search files in all subdirectories.\n\
    -U Unicode. Converts the search string to Unicode before search.\n\
    -s slash. Prints file path using opposite direction of slash.\n\
    -T type of file. Prints whether file is detected as binary or text.\n\
    \n\
    Wilcards in filenames are supported in this version.\n\
    Linux/Unice implementations require quotes around command line\n\
      strings if using wildcard characters (\'*\', \'?\') in order to prevent shell expansion.\n\
    If a hyphen is used as the first char of 'str', succeed it with\n\
      a slash character ('/') to differentiate it from an option\n\
    The slash is removed from the string internally.\n\
    \n\
    ****** Gerhard W. Gross ******\n\n");
}

TCHAR* ErrNoMsg()
{
    _wcserror_s(g_errBuf, _MAX_PATH, errno);
    return g_errBuf;

}

