#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#define CHUNK 20
//metadatele unui fisier : cale relativa + nume ;permisiuni - ultimele 3 cifre; inode number ;  dimensiune in bytes ; data celei mai //recente modificari
typedef struct {
	char cale[256];
	int permisiuni;
	int ino;
	int size;
	char mod_recenta[256];
}metadata;
void afisare_metadata(metadata x)
{
 printf("%s;%o;%d%;%d;%s\n",x.cale,x.permisiuni,x.ino,x.size,x.mod_recenta);
}
void afisare_tablou(metadata *v , int n)
{
	for(int i=0;i<n;i++)
		afisare_metadata(v[i]);
}
void error(char * msg)
{
  perror(msg);
  exit(-1);
}
// creeare vectori
//trebuie sa compar 2 vectori de structuri de tip metadata -> diferentele le pun la stdout
//+ actualizez snapshot

/*int creare_fisier(const char * file)
{
	int fd = open(file,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IXUSR);
	if(fd==-1)
		error("eroare fisier out");
	else
		return fd;
	return -1;
}*/


void parse(const char * dir_path, int fd)  // parcurgere director
{
  DIR * d=NULL;
  if((d=opendir(dir_path))== NULL) // obtin DIR * ~ referinta catre director
    error("eroare deschidere director");

  
  struct dirent * curent=NULL;
  while((curent=readdir(d))!=NULL) // obtin dirent * ~ referinta catre intrarea curenta
    {

      //creeare cale realativa pentru stat/lstat
      
     
      // printf("%s \n",curent->d_name);
      char cale_relativa[256];
      strcpy(cale_relativa,dir_path);
      int n=strlen(cale_relativa);
      cale_relativa[n]='/';
      cale_relativa[n+1]=0;
      strcat(cale_relativa, curent->d_name);
      // printf("%s \n",cale_relativa);


      struct stat status;
      lstat(cale_relativa, &status); // obtin structura stat pentru informatii

      if(S_ISDIR(status.st_mode))
	{
	  n=strlen(cale_relativa);
	  if(cale_relativa[n-1]!='.') // sa nu mai procesam . (dir curent) sau .. (dir parinte)  -> altfel ciclu infinit
	    parse(cale_relativa,fd); // repetam pt subdirector
	}
      else
	{ //prelucrare fisier
	  char description[512];
	sprintf(description,"%s;%o;%ld;%ld;%s ", cale_relativa,status.st_mode, status.st_ino,status.st_size, ctime(&status.st_mtime));
	// metadatele unui fisier : cale relativa + nume ;permisiuni - ultimele 3 cifre; inode number ;  dimensiune in bytes ; data celei mai recente modificari
	
	metadata aux;
	strcpy(aux.cale,cale_relativa);
	aux.permisiuni=status.st_mode;
	aux.ino=status.st_ino;
	aux.size=status.st_size;
	strcpy(aux.mod_recenta,ctime(&status.st_mtime)); 
	//printf("%s %s \n",aux.cale,aux.mod_recenta); verificare salvare corecta in structura
	
	//adauga(aux,tab_new,n1);
	  int k=strlen(description);
	  if(write(fd,description,k) == -1)
	  	error("eroare scriere in fisier");
	}
	

      



      
    }

  
  if(closedir(d)!=0)
    error("eroare la inchidere");


}
metadata procesare( char * line)
{
/*
	char cale[256];
	int permisiuni;
	int ino;
	int size;
	char mod_recenta[256];*/
	metadata info;
	char *p= strtok(line,";");
	strcpy(info.cale,p);
	p=strtok(NULL,";");
	info.permisiuni=strtol(p,NULL,8);
	p=strtok(NULL,";");
	info.ino=atoi(p);
	p=strtok(NULL,";");
	info.size=atoi(p);
	p=strtok(NULL,";");
	strcpy(info.mod_recenta, p);
	return info;
	
	 
}
int salvare_info_snaptxt(int fd, metadata ** v)
{

	int curent=0;
	int max=0;
	char ch;
	char line[256];
	int i=0;
	while((read(fd,&ch,sizeof(ch)))>0)
	{
	
		if(ch=='\n')
		{
		line[i]='\0';
		char auxi[256];
		//printf("%s ***\n" ,line); verificare citire corecta de linii doar cu fctii sistem
		strcpy(auxi,line);
		metadata aux = procesare(auxi);
		//printf("%s %s \n", aux.cale, aux.mod_recenta); verificare salvare corecta in structura
		
		//adaugare in structura in tablou alocaat dinamic


		if(curent== max){
			*v= (metadata*) realloc(*v, (max+CHUNK) *sizeof(metadata));
			max=max+CHUNK;
			}
		(*v)[curent++]=aux;
		
		i=0;
		}
		else
			line[i++]=ch;			
	}
	return curent;
}

int main(int argc , char ** argv)
{
  if(argc<2)
    error("argumente insuficiente");
  //int fd=creare_fisier("fileout.txt");
  int fd = open("fileout.txt",O_RDONLY);
	if(fd==-1)
		error("eroare fisier out");
	
  metadata *old =NULL;	
  int nv=salvare_info_snaptxt(fd,&old);

if(close(fd)!=0)
	error("inchidere");
	
	
//golire fisier snapshot anterior
fd = open("fileout.txt",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IXUSR);
	if(fd==-1)
		error("eroare fisier out");
  if(ftruncate(fd,0) ==-1)
  	error("golire");
  	
  	
  	//actualizare snapshot 
  parse(argv[1],fd);
  lseek(fd,0,SEEK_SET);
  metadata * nou=NULL;
  int nn = salvare_info_snaptxt(fd,&nou);
  /*char buff[256];
  FILE * f= NULL;
  f=fdopen(fd,"r");
  while(fgets(buff,256,f)!=NULL)
  {	
  	printf("%s",buff);
  }*/
  if(close(fd)!=0)
	error("inchidere");
  afisare_tablou(old,nv);
  printf("\n \n \n");
  afisare_tablou(nou,nn);

  return 0;
}
