#include "Func.h"

int Change_File(char* fileName, 
    char* includePath, 
    char* libraryPath,
    std::string& newFileContent) {
    FILE* fp;

    char source_ch[BUFSIZE] = {};

    if ((fp = fopen(fileName, "r")) == NULL) {
        std::printf("Failed to open rb mode vcproj!\n");
        std::cin.get();
        return EXIT_FAILURE;
    }

    //pick up every line
    while (fgets(source_ch, BUFSIZE, fp) != NULL)
    {
        //update include path
        char* result = strstr(source_ch, "<AdditionalIncludeDirectories>");
        if (result != NULL)
        {
            newFileContent += "<AdditionalIncludeDirectories>";
            newFileContent += includePath;
            //newFileContent += "</AdditionalIncludeDirectories>\n";
            continue;
        }

        //update library path
        result = strstr(source_ch, "<AdditionalLibraryDirectories>");
        if (result != NULL)
        {
            newFileContent += "<AdditionalLibraryDirectories>";
            newFileContent += libraryPath;
            newFileContent += "</AdditionalLibraryDirectories>\n";
            continue;
        }

        //replace preprocessor definition expression
        result = strstr(source_ch, "(PreprocessorDefinitions)");
        if (result != NULL)
        {
            replace(source_ch, "(PreprocessorDefinitions)", "%(PreprocessorDefinitions)");
        }
        
        newFileContent += std::string(source_ch);
    }

    //std::cout << *newFileContent << std::endl;
    fclose(fp);

    return EXIT_SUCCESS;
}

char* replace(char* s, const char* before, const char* after)
{
    assert(s != NULL);
    assert(before != NULL);
    assert(after != NULL);

    const size_t before_len = strlen(before);
    if (before_len == 0) {
        return s;
    }

    const size_t after_len = strlen(after);
    char* p = s;

    for (;;) {

        // 置換する文字列を探す
        p = strstr(p, before);
        if (p == NULL) {
            // 見つからなければ、これ以上置換するものはないので終了する
            break;
        }

        // 置換対象にならない位置を計算
        const char* p2 = p + before_len;

        // 置換対象にならない位置(p2) 以降の文字列を、
        // 置換の影響を受けない位置に移動
        memmove(p + after_len, p2, strlen(p2) + 1);

        // 置換する
        memcpy(p, after, after_len);

        // 探索開始位置をずらす
        p += after_len;
    }

    return s;
}

int Save_File(char* file_In, const char* content) {
    FILE* fp;

    //check exist
    fp = fopen(file_In, "r");
    if (fp == NULL) 
    {
        return -1;
    }
    fclose(fp);

    fp = fopen(file_In, "w");

    if (fp == NULL) 
    {
        return -1;
    }

    fprintf(fp, content);

    fclose(fp);
    return 0;
}