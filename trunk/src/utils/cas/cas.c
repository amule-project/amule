#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	FILE *amulesig;
	char *login, *path;
	char stats[20][20];
	int ler,i;

	login=getenv("USER");
	path = malloc(strlen(login) + 64);
	sprintf(path,"/home/%s/.aMule/amulesig.dat",login);

	if ((amulesig = fopen(path, "r")) == NULL)
	{
		puts("Unable to open the file\nCheck if you have amule sig enabled");
		return 0;
	}

	for (i=0;i<= 19;i++)
		stats[i][0]=0;

	i=0;
	while (!feof(amulesig))
	{
		ler = fgetc(amulesig);

		if (!feof(amulesig))
		{

			if (ler != 10)	sprintf(stats[i],"%s%c",stats[i],ler);
			else i++;
		}

	}
	fclose(amulesig);

	// LETS PRINT IT OUT!!!
	if(stats[0][0]=='0') 
	{
		printf("aMule is not running\n");
		return(0);
	}
	else
	{
		printf("aMule %s has been running for %s\n",stats[12],stats[15]);
		printf("%s is on %s [%s:%s] with ",stats[9],stats[1],stats[2],stats[3]);
		if (stats[4][0]=='H') printf("HighID\n"); 
		else printf("LowID\n");
		printf("Total Download: %s GB, Upload %s GB\n",stats[10],stats[11]);
		// experimental
		printf("Session Download: %s MB, Upload %s MB\n",stats[13],stats[14]);
		printf("Download : %s kB/s, Upload : %s kB/s\n",stats[5],stats[6]);
		printf("Sharing : %s file(s), Clients on queue: %s\n",stats[8],stats[7]);
	}
	return 0;
}
