#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
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
 printf("%s;%o;%d;%d;%s\n",x.cale,x.permisiuni,x.ino,x.size,x.mod_recenta);
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

//trebuie sa compar 2 vectori de structuri de tip metadata -> diferentele le pun la stdout
int verificare_harm(const char * file,const char  * iso_dir)
{
	int pfd[2];
	pipe(pfd);
	int pid=fork();
	if(pid==-1)
		error("eroare procese");
	if(pid==0)// proces copil
	{
		close(pfd[0]); // inchide capat citire -> va scrie
		//execut script in proces copil
		//printf("potential harm ");
		dup2(pfd[1],1); // redirectam iesirea standard la care scrie scriptul spre capat scriere pipe
		execlp("/home/rose/so/verify_corupt.sh","./verify_corupt.sh",file,NULL);
		//execlp("ls","ls","-l","dir",NULL);
		printf("nu face exec");
		close(pfd[1]);
		
		
	}
	if(pid>0)
	{
		close(pfd[1]); // inchide capat scriere sa nu blocheze read
		char buffer[256];
		read(pfd[0],buffer,256);
		//printf("+%s+\n",buffer);
		int n=strlen(buffer);
		buffer[n-1]=0; //elimin '\n'
		if(strcmp("SAFE",buffer)!=0)
		{
			int p_mutare=fork();
			if(p_mutare==0) // proces fiu
			{
			 execlp("/home/rose/so/muta.sh","./muta",file,iso_dir,NULL);
			 error("eroare exec");
			}
			if(p_mutare < 0)
				error("creare proces");
			if(p_mutare>0)
			{
				int status;
				waitpid(p_mutare, & status, WCONTINUED); // sa nu ramana procese zombie de la mutare
			}
			close(pfd[0]);
			int status;
			waitpid(pid,&status, WCONTINUED); // sa nu ramana procese zombie de le verif
			return 1;
		}
		close(pfd[0]);
		int status;
		waitpid(pid,&status, WCONTINUED); // sa nu ramana procese zombie de le verif
	}
	return 0;
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


int parse(const char * dir_path, int fd,const char * iso_dir)  // parcurgere director
{
  DIR * d=NULL;
  int pericol=0;
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
	    pericol+=parse(cale_relativa,fd,iso_dir); // repetam pt subdirector
	}
      else
	{ //prelucrare fisier
	 if(S_ISLNK(status.st_mode))
	 {
	 	struct stat status1;
      		lstat(cale_relativa, &status1);
      		printf("Este legatura simbolica catre fisierul cu ino %ld",status1.st_ino);
	 }
	else{
	// declansare verficare potential harm
	if( (status.st_mode & 0777) == 0){
		//printf("potential harm");
		
		pericol+=verificare_harm(cale_relativa,iso_dir);
		//if(verificare_harm(cale_relativa))
			//mutare(cale_relativa,dir_izolare);
		}
	else{	
	  char description[512];
	sprintf(description,"%s;%o;%ld;%ld;%s", cale_relativa,status.st_mode, status.st_ino,status.st_size, ctime(&status.st_mtime));
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

      
	}


      
    }

  
  if(closedir(d)!=0)
    error("eroare la inchidere");
  return pericol;


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



void compara(metadata * old, int nv, metadata  * new , int nn)
{
printf("metadata \n");
 afisare_tablou(old,nv);
  printf("\n \n \n");
 afisare_tablou(new,nn);
printf("metadata fin \n"); 
int j=0;

if(nv!=0 && nn!=0){
int i=0;
	for(i=0;i<nv && j< nn;i++){
		if(old[i].ino==new[j].ino) //acelasi file in aceeasi pozitie == acelasi director
		{
			if(strcmp(old[i].cale,new[j].cale)!=0)
				printf("Fisierul si-a schimbat denumirea din %s in %s \n",old[i].cale,new[j].cale);
			if(strcmp(old[i].mod_recenta,new[j].mod_recenta)!=0)
				printf("Fisierul %s s-a modificat \n",new[j].cale);
			j++;
				// creste si i si j
		}
		else
		{
			int k;
			for( k=j;k<nn;k++) //caut sa vad daca mai exista
				if(old[i].ino==new[k].ino){ // `1il regasesc
					if(strcmp(old[i].cale,new[k].cale)!=0){ // nu cresc j !!! sterg din new
						printf("Fisierul %s fost mutat \n", old[i].cale); // are cale diferita deci s-a mutat
						for(int q=k+1;q<nn;q++){
							new[q-1]=new[q];
							nn--;
								}
							
						}
					else
					{ // aceeasi cale , deci ce e intre s a adaugat
					//printf("nepotivire %s", old[i].cale);
						for(int q=j;q<k;q++) 
							printf("Fisierul %s a fost adaugat \n",new[q].cale);
						// cresc k pana dupa q gasit
						j=k;
						i--;
					}
					break;
				}
			if(k==nn) // nu l-am mai regasit in new pe old[i] -> a fost sters
				{ // nu cresc j
				
					printf("Fisierul %s a fost sters  \n",old[i].cale);
				}
		}
		}
		
		
		if(j!=nn)
	{
		for(int ai=j;ai<nn;ai++)
	
			printf("Fisierul %s a fost adaugat \n",new[ai].cale);
			
		
	}
	if(i!=nv)
	{
		for(int ia=i;ia<nv;ia++)
			printf("Fisierul %s a fost sters \n" , old[ia].cale);
	}
		
		
	}
	// ce nu intra prin for
	
	if(nv == 0  && nn != 0)
	{
		for(int i=0;i<nn;i++)
			printf("Fisierul %s a fost adaugat \n",new[i].cale);
	}
	if(nn == 0 && nv != 0)
	{
		for(int i=0;i<nv;i++)
			printf("Fisierul %s a fost sters \n" , old[i].cale);
	}

}

//123 34 45 78
//123 78
//printf nr proces :txt
//123 34 45 78
//123 67 89 35 45 78


void analiza_director(const char * director, int order, const char * dir_output,const char * iso_dir)
{
	int pericol;
  char file_out[256];
  sprintf(file_out,"%s/fileout%d.txt",dir_output,order);
  int fd = open(file_out,O_RDONLY);
	if(fd!=-1)
	{	
	
  metadata *old =NULL;	
  int nv=salvare_info_snaptxt(fd,&old);

if(close(fd)!=0)
	error("inchidere");
	
	
//golire fisier snapshot anterior
fd = open(file_out,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IXUSR);
	if(fd==-1)
		error("eroare fisier out");
  if(ftruncate(fd,0) ==-1)
  	error("golire");
  	
  	
  	//actualizare snapshot 
  pericol=parse(director,fd,iso_dir);
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
/*printf("metadata \n");
 afisare_tablou(old,nv);
  printf("\n \n \n");
 afisare_tablou(nou,nn);
printf("metadata fin \n"); */
  compara(old,nv,nou,nn);
  
}
else // nu exista snapshot anterior
{
	fd=creare_fisier(file_out);
	  pericol=parse(director,fd,iso_dir);
}
	printf("Fisierul %s contine snapshot-ul directotului %s \n",file_out,director);
	exit(pericol);

}
void verifica_creeaza_dir(const char * dir)
{
	struct stat st;
	if(stat(dir,&st)!=0)// nu exista directorul
	{
		if(mkdir(dir,S_IRWXU | S_IRWXG)!=0)// il creeaza
			error("eroare creeare dir");
	
	}
}

int main(int argc , char ** argv)
{
  if(argc<3)
    error("argumente insuficiente");
  //int fd=creare_fisier("fileout.txt");
  int pid ;
  int v_pid[15];
  int nr_procese=0;
  // primul director valid in linie de comanda e de output
  // al doilea director valid in linie de comanda e de izolare
  if(strcmp(argv[1],"-o")==0){ 
  	verifica_creeaza_dir(argv[2]);
  if(strcmp(argv[3],"-s")==0){
  	verifica_creeaza_dir(argv[4]);
  	for(int i=5;i<argc;i++){
  		pid=fork();
  		if(pid==0){ // cod proces copil
 		analiza_director(argv[i],i-4,argv[2],argv[4]);
 		break; // sa iese din for == sa nu mai fie create procese din copil
 		}
 		else // cod proces parinte ; pid !=0 retine pid copil
 		{
 			if(pid!=-1)
 				v_pid[nr_procese++]=pid;
 			else
 				error("eroare creeare proces");
 		}
 		}
 	}
 	else{
 		verifica_creeaza_dir(argv[3]);
 		for(int i=4;i<argc;i++){
  		pid=fork();
  		if(pid==0){ // cod proces copil
 		analiza_director(argv[i],i-3,argv[2],argv[3]);
 		break; // sa iese din for == sa nu mai fie create procese din copil
 		}
 		else // cod proces parinte ; pid !=0 retine pid copil
 		{
 			if(pid!=-1)
 				v_pid[nr_procese++]=pid;
 			else
 				error("eroare creeare proces");
 		}
 		}
 	
 	}
 }		
 else{
    verifica_creeaza_dir(argv[1]);
    if(strcmp("-s",argv[2])==0){
    	verifica_creeaza_dir(argv[3]);
 	for(int i=4;i<argc;i++)
 	{
 		pid=fork();
  		if(pid==0){ // cod proces copil
 		analiza_director(argv[i],i-3,argv[1],argv[3]);
 		break; // sa iese din for == sa nu mai fie create procese din copil
 		}
 		else // cod proces parinte ; pid !=0 retine pid copil
 		{
 			if(pid!=-1)
 				v_pid[nr_procese++]=pid;
 			else
 				error("eroare creeare proces");
 		}
  	}
  	}
  else{
  	verifica_creeaza_dir(argv[2]);
  	for(int i=3;i<argc;i++){
  		pid=fork();
  		if(pid==0){ // cod proces copil
 		analiza_director(argv[i],i-2,argv[1],argv[2]);
 		break; // sa iese din for == sa nu mai fie create procese din copil
 		}
 		else // cod proces parinte ; pid !=0 retine pid copil
 		{
 			if(pid!=-1)
 				v_pid[nr_procese++]=pid;
 			else
 				error("eroare creeare proces");
 		}
 		}
  	}
  }	
  if(pid!=0){	// doar parintele asteapta incheierea copiilor
  
    for(int i=0;i<nr_procese;i++)
  {
  	int status;
  	int pid_incheiat=waitpid(v_pid[i],&status,WCONTINUED);
  	printf("Procesul %d s-a incheiat, au fost detectate %d fisiere periculoase\n", pid_incheiat, WEXITSTATUS(status));
  }
  
  
  }
  
  
  
  return 0;
}
