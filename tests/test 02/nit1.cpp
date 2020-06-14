#include"testprimer.h"

static char threadName[] = "Nit1";

DWORD WINAPI nit1run(){
	wait(_mutex); partition=new Partition((char *)"p2.ini"); signal(_mutex);
	wait(_mutex); cout<<threadName<<": Kreirana particija"<<endl; signal(_mutex);
	FS::mount(partition);
	wait(_mutex); cout<<threadName<<": Montirana particija"<<endl; signal(_mutex);
	FS::format();
	wait(_mutex); cout<< threadName << ": Formatirana particija"<<endl; signal(_mutex);
	signal(sem12); //signalizira niti 2
	wait(_mutex); cout << threadName << ": wait 1" << endl; signal(_mutex);
	wait(sem21); //ceka nit1
	{
		char filepath[]="/fajl1.dat";
		File *f=FS::open(filepath,'w');
		wait(_mutex); cout<< threadName << ": Kreiran fajl '"<< filepath <<"'"<<endl; signal(_mutex);
		f->write(ulazSize,ulazBuffer);
		wait(_mutex); cout<< threadName << ": Prepisan sadrzaj 'p2.dat' u '" << filepath << "'"<<endl; signal(_mutex);
		delete f;
		wait(_mutex); cout<< threadName << ": zatvoren fajl '" << filepath << "'"<<endl; signal(_mutex);
	}	

	
	{
		File *src,*dst;
		char filepath[]="/fajl1.dat";
		src=FS::open(filepath,'r');
		src->seek(src->getFileSize()/2);//pozicionira se na pola fajla
		wait(_mutex); cout<< threadName << ": Otvoren fajl '" << filepath << "' i pozicionirani smo na polovini"<<endl; signal(_mutex);
		char filepath1[]="/fajll5.dat";
		dst=FS::open(filepath1,'w');
		wait(_mutex); cout<< threadName << ": Otvoren fajl '" << filepath1 << "'"<<endl; signal(_mutex);
		char c;
		while(!src->eof()){
			src->read(1,&c);
			dst->write(1,&c);
		}
		wait(_mutex); cout<< threadName << ": Prepisana druga polovina '" << filepath << "' u '" << filepath1 << "'"<<endl; signal(_mutex);
		delete dst;
		wait(_mutex); cout<< threadName << ": Zatvoren fajl '" << filepath1 << "'"<<endl; signal(_mutex);
		delete src;
		wait(_mutex); cout<< threadName << ": Zatvoren fajl '" << filepath << "'"<<endl; signal(_mutex);
	}
	signal(sem12); // signalizira niti 2
	wait(_mutex); cout<< threadName << ": wait 2"<<endl; signal(_mutex);
	wait(sem21);//ceka nit1


	{
		File *src, *dst;
		char filepath[] = "/fajl25.dat";
		dst = FS::open(filepath, 'a');
		wait(_mutex); cout << threadName << ": Otvoren fajl '" << filepath << "'" << endl; signal(_mutex);
		char filepath1[] = "/fajll5.dat";
		src = FS::open(filepath1, 'r');
		wait(_mutex); cout << threadName << ": Otvoren fajl '" << filepath1 << "'" << endl; signal(_mutex);
		char c;
		while (!src->eof()) {
			src->read(1, &c);
			dst->write(1, &c);
		}
		wait(_mutex); cout << threadName << ": Prepisana druga polovina '" << filepath << "' u '" << filepath1 << "'" << endl; signal(_mutex);
		delete dst;
		wait(_mutex); cout << threadName << ": Zatvoren fajl '" << filepath1 << "'" << endl; signal(_mutex);
		delete src;
		wait(_mutex); cout << threadName << ": Zatvoren fajl '" << filepath << "'" << endl; signal(_mutex);
	}
	signal(sem12); // signalizira niti 2

	wait(_mutex); cout<< threadName << ": Zavrsena!"<<endl; signal(_mutex);
	signal(semMain);
	return 0;
}