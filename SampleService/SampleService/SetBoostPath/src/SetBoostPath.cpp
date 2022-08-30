#define _CRT_SECURE_NO_WARNINGS
#include "Func.h"
using namespace std;
#define BUF 1024

int main()
{ 
    //////////////////////////////////
    // Read Boost_Path.txt
    //////////////////////////////////

    auto src = fopen(BoostPathTxtPath, "r");
    if (src == NULL) {
        std::printf("%s not found!\n", BoostPathTxtPath);
        std::cin.get();
        return -1;
    }

    int i = 0;
    char path[BUF];
    char includePath[BUF];
    includePath[0] = '\0';
    char libraryPath[BUF];
    libraryPath[0] = '\0';

    while (fgets(path, BUF, src) != NULL) {
        if (strlen(path) >= 2 
            && path[strlen(path) - 1] == '\n'
            && path[strlen(path) - 2] == '\r') 
        {
            path[strlen(path) - 2] = '\0';
        }

        if (strlen(path) >= 1
            && path[strlen(path) - 1] == '\n')
        {
            path[strlen(path) - 1] = '\0';
        }

        if(i== 0) {
            strcpy(includePath, path);
        }else if (i == 1) {
            strcpy(libraryPath, path);
            break;
        }

        i++;

        if (i == 2) break;
    }

    if (includePath[0] == '\0' || libraryPath[0] == '\0') return -1;

    //std::printf("%s\n", includePath);
    //std::printf("%s", libraryPath);
    //std::cin.get();

    //////////////////////////////////
    // Change SampleService.vcxproj
    //////////////////////////////////
    string newFileContent = "";
    int result = changeFile(
        (char*)vcxprojPath,
        includePath,
        libraryPath,
        newFileContent);
    if (result == EXIT_FAILURE) 
    {
        std::printf("Failed to open %s!\n", vcxprojPath);
        std::cin.get();
        return -1;
    }

    //////////////////////////////////
    // Update SampleService.vcxproj
    //////////////////////////////////
    result = saveFile((char*)vcxprojPath, newFileContent.c_str());

    if (result != 0)
    {
        std::printf("Failed to update %s!\n", vcxprojPath);
        std::cin.get();
        return -1;
    }

    std::printf("Success boost path setup!!\n");
    std::cin.get();
    exit(EXIT_SUCCESS);
}