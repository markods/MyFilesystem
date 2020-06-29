#include"testprimer.h"

static char threadName[] = "Nit2";

DWORD WINAPI nit2run(){
	wait(sem12);	//ceka nit1
	signal(sem21); // signalizira nit1
	{
		File *src, *dst;
		char filepath[] = "/fajl1.dat";
		while ((src = FS::open(filepath, 'r')) == 0) {
			wait(_mutex); cout << threadName << ":Neuspesno otvoren fajl '" << filepath << "'" << endl; signal(_mutex);
			Sleep(1); // Ceka 1 milisekundu
		}
		wait(_mutex); cout << threadName << ": Otvoren fajl '" << filepath << "'" << endl; signal(_mutex);
		char filepath1[] = "/fajl2.dat";
		dst = FS::open(filepath1, 'w');
		wait(_mutex); cout << threadName << ": Otvoren fajl '" << filepath1 << "'" << endl; signal(_mutex);
		char c;
		while (!src->eof()) {
			src->read(1, &c);
			dst->write(1, &c);
		}
		wait(_mutex); cout << threadName << ": Prepisan fajl '" << filepath << "' u '" << filepath1 << "'" << endl; signal(_mutex);
		delete dst;
		wait(_mutex); cout << threadName << ": Zatvoren fajl '" << filepath1 << "'" << endl; signal(_mutex);
		delete src;
		wait(_mutex); cout << threadName << ": Zatvoren fajl '" << filepath << "'" << endl; signal(_mutex);

	}
	wait(_mutex); cout<< threadName << ": wait 1"<<endl; signal(_mutex);
	wait(sem12); // ceka nit 1	 
	{
		wait(_mutex); cout << threadName << ": Broj fajlova na disku je " << FS::readRootDir() << endl; signal(_mutex);
	}

	{
		char filepath[]="/fajl2.dat";
		File *f=FS::open(filepath,'r');
		wait(_mutex); cout<< threadName << ": Otvoren fajl " << filepath << ""<<endl; signal(_mutex);
		delete f;
		wait(_mutex); cout<< threadName << ": Zatvoren fajl " << filepath << ""<<endl; signal(_mutex);
	}

	{
		char filepath[]="/fajl2.dat";
		File *f=FS::open(filepath,'r');
		wait(_mutex); cout<< threadName << ": Otvoren fajl " << filepath << ""<<endl; signal(_mutex);
		ofstream fout("izlaz1.jpg", ios::out|ios::binary);
		char *buff=new char[f->getFileSize()];
		f->read(f->getFileSize(),buff);
		fout.write(buff,f->getFileSize());
		wait(_mutex); cout<< threadName << ": Upisan '" << filepath << "' u fajl os domacina 'izlaz1.dat'"<<endl; signal(_mutex);
		delete [] buff;
		fout.close();
		delete f;
		wait(_mutex); cout<< threadName << ": Zatvoren fajl " << filepath << ""<<endl; signal(_mutex);
	}

	{
		char copied_filepath[] = "/fajll5.dat";
		File *copy = FS::open(copied_filepath, 'r');
		BytesCnt size = copy->getFileSize();
		wait(_mutex); cout << threadName << ": Otvoren fajl '" << copied_filepath << "' i dohvacena velicina" << endl; signal(_mutex);
		delete copy;
		wait(_mutex); cout << threadName << ": Zatvoren fajl '" << copied_filepath << "'" << endl; signal(_mutex);
		File *src, *dst;
		char filepath[] = "/fajl1.dat";
		src = FS::open(filepath, 'r');
		src->seek(0);//pozicionira se na pola fajla
		wait(_mutex); cout << threadName << ": Otvoren fajl '" << filepath << "' i pozicionirani smo na polovini" << endl; signal(_mutex);
		char filepath1[] = "/fajl25.dat";
		dst = FS::open(filepath1, 'w');
		wait(_mutex); cout << threadName << ": Otvoren fajl '" << filepath1 << "'" << endl; signal(_mutex);
		char c; BytesCnt cnt = src->getFileSize() - size;
		while (!src->eof() && cnt-- > 0) {
			src->read(1, &c);
			dst->write(1, &c);
		}
		wait(_mutex); cout << threadName << ": Prepisana druga polovina '" << filepath << "' u '" << filepath1 << "'" << endl; signal(_mutex);
		delete dst;
		wait(_mutex); cout << threadName << ": Zatvoren fajl '" << filepath1 << "'" << endl; signal(_mutex);
		delete src;
		wait(_mutex); cout << threadName << ": Zatvoren fajl '" << filepath << "'" << endl; signal(_mutex);
	}

	signal(sem21); // signalizira niti 1
	wait(_mutex); cout << threadName << ": wait 1" << endl; signal(_mutex);
	wait(sem12); // ceka nit 2	

	{
		char filepath[] = "/fajl25.dat";
		File *f = FS::open(filepath, 'r');
		wait(_mutex); cout << threadName << ": Otvoren fajl " << filepath << "" << endl; signal(_mutex);
		ofstream fout("izlaz2.jpg", ios::out | ios::binary);
		char *buff = new char[f->getFileSize()];
		f->read(f->getFileSize(), buff);
		fout.write(buff, f->getFileSize());
		wait(_mutex); cout << threadName << ": Upisan '" << filepath << "' u fajl os domacina 'izlaz1.dat'" << endl; signal(_mutex);
		delete[] buff;
		fout.close();
		delete f;
		wait(_mutex); cout << threadName << ": Zatvoren fajl " << filepath << "" << endl; signal(_mutex);
	}

	{
		FS::unmount();
		wait(_mutex); cout << threadName << ": Demontirana particija p1" << endl; signal(_mutex);
	}


	wait(_mutex); cout<< threadName << ": Zavrsena!"<<endl; signal(_mutex);
	signal(semMain);
	return 0;
}