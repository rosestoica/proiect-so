#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
void error(char * msg)
{
  perror(msg);
  exit(-1);
}


int creare_fisier(const char * file)
{
	int fd = open(file,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IXUSR);
	if(fd==-1)
		error("eroare fisier out");
	else
		return fd;
	return -1;
}


void parse(const char * dir_path, int fd)
{
  DIR * d=NULL;
  if((d=opendir(dir_path))== NULL)
    error("eroare deschidere director");

  
  struct dirent * curent=NULL;
  while((curent=readdir(d))!=NULL)
    {

      //creeare cale realativa
      // printf("%s \n",curent->d_name);
      char cale_relativa[256];
      strcpy(cale_relativa,dir_path);
      int n=strlen(cale_relativa);
      cale_relativa[n]='/';
      cale_relativa[n+1]=0;
      strcat(cale_relativa, curent->d_name);
      // printf("%s \n",cale_relativa);


      struct stat status;
      lstat(cale_relativa, &status);

      if(S_ISDIR(status.st_mode))
	{
	  n=strlen(cale_relativa);
	  if(cale_relativa[n-1]!='.')
	    parse(cale_relativa,fd);
	}
      else
	{ //prelucrare fisier
	  char description[512];
	sprintf(description,"%s %ld %ld %ld \n", cale_relativa, status.st_ino,status.st_size,status.st_mtim.tv_sec);

	  int k=strlen(description);
	  if(write(fd,description,k+1) == -1)
	  	error("eroare scriere in fisier");

	}

      



      
    }

  
  if(closedir(d)!=0)
    error("eroare la inchidere");


}


int main(int argc , char ** argv)
{
  if(argc<2)
    error("argumente insuficiente");
  int fd=creare_fisier("fileout.txt");
  
  parse(argv[1],fd);

  return 0;
}
