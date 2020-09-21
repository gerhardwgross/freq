# freq
Windows/Linux search command line utility. Works with text and binary files, can be used to search for any bit pattern.

    Usage:  freq [-iplfvwcWPRUT] str filename

    Searches 'filename' for occurrences of 'str'.

    Can use wildcards in file name search. In Linux, add quotes around file name search pattern to
    prevent the shell from expanding (e.g. "*.cpp"). Expansion and recursion are done within this program.
    
    -i integer. 'str' must be one or more ASCII integers (0 - 255)
       separated by commas, no spaces.  The integers in 'str'
       must by enclosed in quotes if there are more than one.
    -p print text containing string 'str' and other info.
    -v verbose. Print text containing string 'str' and even more info.
    -P Print only line of text that contains string 'str'.
    -l Same as -P but also prints filename (no path) on next line.
    -n Search string must not span multiple lines.
    -mXX Print XX-1 lines after the line containing the search string (text files only).
    -dXX Set width of output (number of chars across screen = XX)
    -f find file/dir command. Does not accept search string. Prints all files that match
       the file pattern. Case insensitive option not possible on Linux.
    -w wildcard char '?' in \'str\' represents any single character.
    -o omit file pattern from being searched (-o *.dll). Can supply this option multiple times.
    -O omit directory from being searched (-O DirName). Can supply this option multiple times.
    -c case insensitive. Insensitive to case of search string.
    -W whole word. Searches for whole word occurrences only.
    -R recursive. Recursively search files in all subdirectories.
    -U Unicode. Converts the search string to Unicode before search.
    -s slash. Prints file path using opposite direction of slash.
    -T type of file. Prints whether file is detected as binary or text.

    Wilcards in filenames are supported in this version.
    Linux/Unice implementations require quotes around command line
      strings if using wildcard characters (\'*\', \'?\') in order to prevent shell expansion.
    If a hyphen is used as the first char of 'str', succeed it with
      a slash character ('/') to differentiate it from an option
    The slash is removed from the string internally.
