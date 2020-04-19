#pragma once
#include "!global.h"
class Partition;
class KernelFS;
class File;


class FS
{
public:
    ~FS();

    static char mount(Partition* part);   // montira particiju,   vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
    static char unmount();                // demontira particiju, vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
    static char format();                 // formatira particiju, vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha

    static FileCnt readRootDir();         // vraca -1 u slucaju neuspeha ili broj fajlova u slucaju uspeha
    static char doesExist(char* fname);   // argument je naziv fajla sa apsolutnom putanjom
    static File* open(char* fname, char mode);
    static char deleteFile(char* fname);

protected:
    FS();
    static KernelFS *impl;
};

