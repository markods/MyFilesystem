#include <iostream>
#include "!global.h"
#include "partition.h"
#include "block.h"
#include "fs.h"
#include "file.h"
#include <vector>


int MFS_TEST_03()
{
/*
    std::cout << "==============================< TEST 03 >======" << std::endl;


//  BytesCnt bytescnt = 6;
//  char buffer[] = "proba";

//  BytesCnt bytescnt = 12;
//  char buffer[] = "ovojeproba!";

//  BytesCnt bytescnt = 13;
//  char buffer[] = "ovojeprobaXY";

//  BytesCnt bytescnt = 14;
//  char buffer[] = "ovojeprobaXYZ";

//  BytesCnt bytescnt = 48;
//  char buffer[] = "ovo je probni string koji ce biti upisan u fajl";

//  const BytesCnt bytescnt = FileSizeS - 1;
//  const BytesCnt bytescnt = FileSizeS;
//  const BytesCnt bytescnt = FileSizeS + 1;
//  const BytesCnt bytescnt = FileSizeT +    5*DataBlkSize;
//  const BytesCnt bytescnt = FileSizeT +  100*DataBlkSize;
//  const BytesCnt bytescnt = FileSizeT +  500*DataBlkSize;
    const BytesCnt bytescnt = FileSizeT +  993*DataBlkSize;  // broj blokova jednog fajla koji zauzima skoro celu particiju od 1000 blokova
//  const BytesCnt bytescnt = (2048UL*8UL)*2048UL;
//  char buffer[bytescnt];
    char *buffer = new char[bytescnt];
    for( long i = 0; i < bytescnt-1; i++ )
        buffer[i] = 'a' + i % 26;
    buffer[bytescnt-1] = 0;

    char partprefs[] = "p3.ini";
    Partition partition { partprefs };

 // Block bitv_blk;
 // partition.readCluster(0, bitv_blk);
 // std::cout << bitv_blk.bitv << std::endl;
 // 
 // Block indx_blk;
 // partition.readCluster(1, indx_blk);
 // std::cout << indx_blk.indx << std::endl;

    // -----------------------------------------

    if( FS::mount(&partition) == MFS_FS_OK )
    {
        if( FS::format(true) == MFS_FS_OK )
//      if( FS::format(false) == MFS_FS_OK )
        {
           char filepath[] = "/fajl1.dat";
           File* f = FS::open(filepath, 'w');
           if( f )
           {
               f->write(bytescnt, buffer);
               delete f;
           }
        }

        FS::unmount();
    }

    delete[] buffer;

    std::cout << "===============================================" << std::endl;
    return 0;
*/


/*
    std::cout << "==============================< TEST 03 >======" << std::endl;

//  BytesCnt bytescnt = 1;   long repeat = 1;     
//  BytesCnt bytescnt = 1;   long repeat = 11;    
//  BytesCnt bytescnt = 1;   long repeat = 12;    
//  BytesCnt bytescnt = 1;   long repeat = 13;    
//  BytesCnt bytescnt = 1;   long repeat = 14;    

//  BytesCnt bytescnt = 2;   long repeat = 6;     
//  BytesCnt bytescnt = 2;   long repeat = 7;       
//  BytesCnt bytescnt = 2;   long repeat = 8;     
//  BytesCnt bytescnt = 2;   long repeat = 9;     
//  BytesCnt bytescnt = 2;   long repeat = 1500;

//  BytesCnt bytescnt = 3;   long repeat = 4;     
//  BytesCnt bytescnt = 3;   long repeat = 5;
//  BytesCnt bytescnt = 3;   long repeat = 6;
//  BytesCnt bytescnt = 3;   long repeat = 10000;

//  BytesCnt bytescnt = 10;   long repeat = 5;

//  BytesCnt bytescnt = 12+512*2048+1;   long repeat = 1;
    BytesCnt bytescnt = 12+512*2048+1;   long repeat = 1;

    char *buffer = new char[bytescnt];
    for( long i = 0; i < bytescnt; i++ )
        buffer[i] = 'a' + i % 26;


    char partprefs[] = "p3.ini";
    Partition partition { partprefs };

    MFS status = MFS_OK;

    // -----------------------------------------

    if( FS::mount(&partition) == MFS_FS_OK )
    {
        if( FS::format(true) == MFS_FS_OK )
        {
            char filepath[] = "/fajl1.dat";
            File* f = FS::open(filepath, 'w');
            if( f )
            {
                for( long j = 0; j < repeat; j++ )
                    status = f->write(bytescnt, buffer);
                delete f;
            }
        }

        status = FS::unmount();
    }

    delete[] buffer;

    std::cout << "===============================================" << std::endl;
    return 0;
*/

/*
    std::cout << "==============================< TEST 03 >======" << std::endl;


//  BytesCnt bytescnt = 100;   long repeat = 15;
    BytesCnt bytescnt = 2048+2048+6;  long repeat = 1;

    char *buffer = new char[bytescnt];
    for( long i = 0; i < bytescnt; i++ )
        buffer[i] = 'a' + i % 26;


    char partprefs[] = "p3.ini";
    Partition partition { partprefs };

    MFS status = MFS_OK;
    char filepath[] = "/fajl1.dat";
    File * f;
    // -----------------------------------------

    if( FS::mount(&partition) == MFS_FS_OK )
    {
        if( FS::format(true) == MFS_FS_OK )
        {
            f = FS::open(filepath, 'w');
            if( f )
            {
                for( long j = 0; j < repeat; j++ )
                    status = f->write(bytescnt, buffer);
                delete f;
            }
        }

        status = FS::unmount();
    }

    if( FS::mount(&partition) == MFS_FS_OK )
    {
        f = FS::open(filepath, 'a');
        if( f )
        {
            for( long j = 0; j < repeat; j++ )
                status = f->write(bytescnt, buffer);
            delete f;
        }


        char buf2[] = "APPEND";
        long bcnt = 6;
        f = FS::open(filepath, 'a');
        if( f )
        {
         // f->seek(0);
         // f->seek(10);
         // f->seek(9+2048);
         // f->seek(3000-3);
            f->seek(9+2048*4);
            status = f->write(bcnt, buf2);
            delete f;
        }

        status = FS::unmount();
    }

    if( FS::mount(&partition) == MFS_FS_OK )
    {
        f = FS::open(filepath, 'r');
        const long max_buf3_size = 100000;
        char buf3[max_buf3_size];
        MFS32 bytesread = 0;
        if( f )
        {
            status = f->seek(9+2048*4);
            bytesread = f->read(6, buf3);  // APPEND

            status = f->seek(6);
            bytesread = f->read(9, buf3);

            status = f->seek(0);
            bytesread = f->read(max_buf3_size, buf3);

         // status = f->seek(8000);
         // status = f->truncate();
         //
         // status = f->seek(4000);
         // status = f->truncate();
         //
         // status = f->seek(2000);
         // status = f->truncate();
         //
         // status = f->seek(13);
         // status = f->truncate();

         // status = f->seek(12);
         // status = f->truncate();

         // status = f->seek(0);
         // status = f->truncate();

            delete f;
        }

        status = FS::deleteFile(filepath);


        status = FS::unmount();
    }

    delete[] buffer;

    std::cout << "===============================================" << std::endl;
    return 0;
*/

/*
    std::cout << "==============================< TEST 03 >======" << std::endl;


    BytesCnt bytescnt1 = 200;   long repeat1 = 1;   BytesCnt bytescnt2 = 100;   long repeat2 = 1;

    char *buffer1 = new char[bytescnt1];
    for( long i = 0; i < bytescnt1; i++ )
        buffer1[i] = 'a' + i % 26;

    char *buffer2 = new char[bytescnt2];
    for( long i = 0; i < bytescnt2; i++ )
        buffer2[i] = 'A' + i % 26;


    char partprefs[] = "p3.ini";
    Partition partition { partprefs };

    MFS status = MFS_OK;
    char filepath[] = "/fajl1.dat";
    File * f;
    // -----------------------------------------

    if( FS::mount(&partition) == MFS_FS_OK )
    {
        if( FS::format(true) == MFS_FS_OK )
        {
            f = FS::open(filepath, 'w');
            if( f )
            {
                for( long j = 0; j < repeat1; j++ )
                    status = f->write(bytescnt1, buffer1);
                delete f;
            }
        }

        status = FS::unmount();
    }

    if( FS::mount(&partition) == MFS_FS_OK )
    {
        f = FS::open(filepath, 'w');
        if( f )
        {
            for( long j = 0; j < repeat2; j++ )
                status = f->write(bytescnt2, buffer2);
            delete f;
        }

        status = FS::unmount();
    }

    delete[] buffer1;
    delete[] buffer2;

    std::cout << "===============================================" << std::endl;
    return 0;
*/

/*
    std::cout << "==============================< TEST 03 >======" << std::endl;


    char partprefs[] = "p3.ini";
    Partition partition { partprefs };
    MFS status = MFS_OK;
    char filepath[14];
    File * f;
     
//  const long max_filecnt = 65;         BytesCnt bytescnt = 100;  //   long repeat = 1;
//  const long max_filecnt = 10000;      BytesCnt bytescnt = 100;  //   long repeat = 1;  //maksimalni broj bajtova za format fajlXXXX
//  const long max_filecnt = 512*64+1;   BytesCnt bytescnt = 100;  //   long repeat = 1;  //maksimalni broj bajtova
//  const long max_filecnt = 100;        BytesCnt bytescnt = 100;  //   long repeat = 1;
    const long max_filecnt = 1000;       BytesCnt bytescnt = 100;  //   long repeat = 1;


    char *buffer = new char[bytescnt];
    for( BytesCnt i = 0; i < bytescnt; i++ )
        buffer[i] = 'a' + i % 26;

    // -----------------------------------------

    if( FS::mount(&partition) == MFS_FS_OK )
    {
        status = FS::format(true);
        status = FS::unmount();
    }
   
    if( FS::mount(&partition) == MFS_FS_OK )
    {
        for( long filecnt = 0; filecnt < max_filecnt; filecnt++ )
        {
            sprintf_s( filepath, "/f%li.dat", filecnt );
         // if( filecnt % 1000 == 0 )
                printf_s( "creating file %s - length = %i\n", filepath, filecnt * bytescnt );
   
            f = FS::open(filepath, 'w');
            if( f )
            {
                for( long j = 0; j < filecnt; j++ )
                    status = f->write(bytescnt, buffer);
                delete f;
            }
   
        }
   
        status = FS::unmount();
    }
   
    if( FS::mount(&partition) == MFS_FS_OK )
    {
        FileCnt filecnt = FS::readRootDir();
   
        status = FS::unmount();
   
    }

    std::vector<std::string> fnames;
    for( long filecnt = 0; filecnt < max_filecnt; filecnt++ )
    {
        sprintf_s(filepath, "/f%li.dat", filecnt);
        fnames.push_back( std::string { filepath });
    }

    srand(91872348123);


    if( FS::mount(&partition) == MFS_FS_OK )
    {
        while( fnames.size() > 0 )
        {
            int pos = rand()%fnames.size();

            std::string& str = fnames.at(pos);
            FS::deleteFile(str.c_str());
            
            fnames.erase(fnames.begin() + pos);
        }

        status = FS::unmount();
    }


    delete[] buffer;

    std::cout << "===============================================" << std::endl;
    return 0;
*/
/*
    std::cout << "==============================< TEST 03 >======" << std::endl;


    char partprefs[] = "p3.ini";
    Partition partition { partprefs };
    MFS status = MFS_OK;
    char filepath[14];
    File * f;
     
//  long max_filecnt = 65;         BytesCnt bytescnt = 100;  //   long repeat = 1;
//  long max_filecnt = 10000;      BytesCnt bytescnt = 100;  //   long repeat = 1;  //maksimalni broj bajtova za format fajlXXXX
//  long max_filecnt = 512*64+1;   BytesCnt bytescnt = 100;  //   long repeat = 1;  //maksimalni broj bajtova
//  long max_filecnt = 100;        BytesCnt bytescnt = 100;  //   long repeat = 1;
    long max_filecnt = 1000;       BytesCnt bytescnt = 100;  //   long repeat = 1;


    char *buffer = new char[bytescnt];
    for( BytesCnt i = 0; i < bytescnt; i++ )
        buffer[i] = 'a' + i % 26;

    // -----------------------------------------

    if( FS::mount(&partition) == MFS_FS_OK )
    {
        status = FS::format(true);
        status = FS::unmount();
    }
   
    srand(91872348123);
    FileCnt filecnt;
    long filenum = 0;
    std::vector<std::string> fnames;

    for( long bigloop = 4; bigloop >= 0; bigloop-- )
    {
        max_filecnt = 20 + rand()%981;

        if( FS::mount(&partition) == MFS_FS_OK )
        {
            for( long filecnt = 0; filecnt < max_filecnt; filecnt++ )
            {
                sprintf_s(filepath, "/f%li.dat", filenum++);
             // if( filecnt % 1000 == 0 )
                printf_s("creating file %s - length = %i\n", filepath, filecnt * bytescnt);

                f = FS::open(filepath, 'w');
                if( f )
                {
                    fnames.push_back(std::string { filepath });

                    for( long j = 0; j < filenum; j++ )
                        status = f->write(bytescnt, buffer);
                    delete f;

                }
            }

            long remove = fnames.size();
            if( bigloop > 0 ) remove /= 2;

            for( ; remove > 0; remove-- )
            {
                int pos = rand()%fnames.size();

                std::string& str = fnames.at(pos);
                FS::deleteFile(str.c_str());

                fnames.erase(fnames.begin() + pos);
            }
        }

        status = FS::unmount();
    }

    delete[] buffer;

    std::cout << "===============================================" << std::endl;
    return 0;
*/
/*
    std::cout << "==============================< TEST 03 >======" << std::endl;


    char partprefs[] = "p3.ini";
    Partition partition { partprefs };
    MFS status = MFS_OK;
    char filepath[14];
    File * f;
     
    BytesCnt bytescnt = 100;    long repeat = 0;

    char *buffer = new char[bytescnt];
    for( BytesCnt i = 0; i < bytescnt; i++ )
        buffer[i] = 'a' + i % 26;

    // -----------------------------------------

    if( FS::mount(&partition) == MFS_FS_OK )
    {
        status = FS::format(true);


        f = FS::open("/file1.dat", 'w');
        if( f )
        {
            for( long j = 0; j < repeat; j++ )
                status = f->write(bytescnt, buffer);
            delete f;
        }
        f = FS::open("/file2.dat", 'w');
        if( f )
        {
            for( long j = 0; j < repeat; j++ )
                status = f->write(bytescnt, buffer);
            delete f;
        }
        f = FS::open("/file3.dat", 'w');
        if( f )
        {
            for( long j = 0; j < repeat; j++ )
                status = f->write(bytescnt, buffer);
            delete f;
        }
        FS::deleteFile("/file1.dat");
        f = FS::open("/file4.dat", 'w');
        if( f )
        {
            for( long j = 0; j < repeat; j++ )
                status = f->write(bytescnt, buffer);
            delete f;
        }


        status = FS::unmount();
    }


    std::cout << "===============================================" << std::endl;
    return 0;
*/


    std::cout << "==============================< TEST 03 >======" << std::endl;

    #pragma warning(suppress : 4996)   // suppress the crt secure warning for the next line
    FILE *osf = fopen("ulaz.jpg", "rb");
    if( osf==0 )
        return 0;//exit program

    #pragma warning(suppress : 4996)   // suppress the crt secure warning for the next line
    FILE *isf = fopen("izlaz.jpg", "wb");
    if( isf==0 )
        return 0;//exit program


    char c;
    long osfbytesread = 0;
    long isfbyteswritten = 0;


    char partprefs[] = "p3.ini";
    Partition partition { partprefs };
    MFS status = MFS_OK;
    File * f;
    char c_test;

    if( FS::mount(&partition) == MFS_FS_OK )
    {
        status = FS::format(true);

        f = FS::open("/testfajl.dat", 'w');
        if( f )
        {
            while( fread(&c, 1, 1, osf) > 0 )
            {
                if( osfbytesread < 30 )
                    printf("%02x ", (unsigned char) c);

                status = f->write(1, &c);

             // delete f;
             // FS::unmount();
             // FS::mount(&partition);
             // f = FS::open("/testfajl.dat", 'a');


                f->seek(osfbytesread);
                f->read(1, &c_test);

                if( c != c_test )
                    osfbytesread += 0;


                if( status > 0 )
                    osfbytesread++;
                else
                    osfbytesread += 0;

            }
            printf("\n");

            printf("ucitano %li bajtova iz ulaznog fajla\n", osfbytesread);
            delete f;
        }
        status = FS::unmount();
    }
    fclose(osf);


    if( FS::mount(&partition) == MFS_FS_OK )
    {
        f = FS::open("/testfajl.dat", 'r');
        if( f )
        {
            while( f->read(1, &c) > 0 )
            {
                if( isfbyteswritten < 30 )
                    printf("%02x ", (unsigned char) c);

                fwrite( &c, 1, 1, isf );
                isfbyteswritten++;
            }
            printf("\n");

            printf("upisano %li bajtova u izlazni fajl\n", isfbyteswritten);
            delete f;
        }
        status = FS::unmount();
    }
    fclose(osf);



    std::cout << "===============================================" << std::endl;
    return 0;



/*
    std::cout << "==============================< TEST 03 >======" << std::endl;

    BytesCnt bytescnt = 2048;   long repeat = 980+1;     
//  BytesCnt bytescnt = 2048;   long repeat = 513+1;

    char *buffer = new char[bytescnt+1];

    long room4data = 12;
    char letter = 'a';

    char partprefs[] = "p3.ini";
    Partition partition { partprefs };

    long byteswritten = 0;

    MFS status = MFS_OK;

    // -----------------------------------------

    if( FS::mount(&partition) == MFS_FS_OK )
    {
        if( FS::format(true) == MFS_FS_OK )
        {
            char filepath[] = "/fajl1.dat";
            File* f = FS::open(filepath, 'w');
            if( f )
            {
                for( long j = 0; j < repeat; j++ )
                {
                    for( long i = 0; i < room4data; i++ )
                        buffer[i] = letter;

                    status = f->write(room4data, buffer);
                    if( status == MFS_OK )
                        byteswritten += room4data;
                    else
                        byteswritten += 0;

                    letter = (letter - 'a' + 1) % 26 + 'a';

                    if( room4data == 12 )
                        room4data = 2048;
                }
                delete f;
            }
        }

        printf("total of %li bytes written to fajl1.dat\n", byteswritten);
//      status = FS::unmount();
//  }
//
//
//  if( FS::mount(&partition) == MFS_FS_OK )
//  {
        room4data = 2048;
        buffer[2048] = '\0';
        char filepath[] = "/fajl1.dat";
        File* f = FS::open(filepath, 'r');
        if( f )
        {
            f->seek(12);
            status = f->read(room4data, buffer);

            f->seek(12 + 514*2048);
            status = f->read(room4data, buffer);

            f->seek(12 + 979*2048);
            status = f->read(room4data, buffer);

            f->seek(4);
            status = f->read(16, buffer);
            buffer[16] = '\0';

            f->seek(4 + 514*2048);
            status = f->read(16, buffer);
            buffer[16] = '\0';

            f->seek(4 + 979*2048);
            status = f->read(16, buffer);
            buffer[16] = '\0';

            delete f;
        }

        printf("total of %li bytes written to fajl1.dat\n", byteswritten);
        status = FS::unmount();
    }


    delete[] buffer;

    std::cout << "===============================================" << std::endl;
    return 0;
*/

}
