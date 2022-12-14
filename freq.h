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
//    Freq.h - header file for system nonspecific functions for freq.c
//
***************************************************************************/

#ifndef _FREQ_H_
#define _FREQ_H_

// Common headers
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<stddef.h>
#include<fcntl.h>

const int TB = 9;
const int NL = 10;
const int CR = 13;
const int NP = 12;
#define ARSZ_MAX                32768
#define WC1                     63   /* '?' - chosen as char wildcard. */
#define MAX_LINE_SZ             4096
#define FILES_MATCH_MAX         3000 /* max num files matching wildcards
                                           per subdir */
#define FREQ_EIGHT_BIT_ASCII    0    // normal ASCII
#define FREQ_UNICODE            1    // Unicode search (two bytes per char)
#define FREQ_TEXT_FILE          0    // text file
#define FREQ_BINARY_FILE        1    // binary file

//#ifndef _MAX_PATH
#define _MAX_PATH               4096
//#endif

const int MAX_DIRS_TO_OMIT = 50;
const int MAX_FILE_PATTERNS_TO_OMIT = 50;

#define DO_CLEANUP(x) goto x
#define CLEANUP_LABEL(x) x:
#define UTIL_SUCCESS            0
#define UTIL_ERROR              1

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

typedef char t_BOOL;

/***************************************************************************
    Public Function Prototypes
***************************************************************************/
long ProcessFile(TCHAR *fname);
void ReverseSlashDirInString(TCHAR* buff);
void CopyStringToLowerCaseRemoveAsterisk(const TCHAR* origStr, TCHAR * lwrCaseStr);
void OnError(int i, int x, int line, const TCHAR *str, bool exitApp = true);
TCHAR* ErrNoMsg();

#endif
