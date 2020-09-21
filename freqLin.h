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

//    FreqLin.h - header file for Linux specific functions for freq.c

//

***************************************************************************/


#ifndef _FREQLIN_H_
#define _FREQLIN_H_


// Linux specific headers

#include<dirent.h>
#include<sys/stat.h>
#include<unistd.h>
#include<ctype.h>

// For raw (unbuffered) file I/O

#define FREQ_BINARY     0x00000000
#define FREQ_READ       O_RDONLY
#define _stat64         stat64
#define DELIM           47    // dirname delimiter - forward slash character

/***************************************************************************
    Public Function Prototypes
***************************************************************************/

void SearchAllDirectories(const char *raw_in_file);
void SearchCurrentDirectory(const char *raw_in_file, const char *current_path);

int _chdir(const char* dirPath);
char* _getcwd(char* dirPath, int sz);
int strerror_s(char* buff, int buffSz, int errNum);
int strcpy_s(char* dest, size_t destSz, const char* src);
int strncpy_s(char* dest, size_t destSz, const char* src, size_t numBytes);

#endif

