/*
		c amule statistics
	
	written by:
	Pedro de Olveira <falso@rdk.homeip.net>

	this is very buggy software by i hope you like it.
	i mainly did this cause aStats was very slow, so i
	tried to do a remake of it in c to learn something
	and do something useful.

	if anyone wants to help me or give any ideas please
	email me.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	/* Declaration of variables */
	FILE *amulesig;
	char *login, *path;
	char stats[20][80];
	int ler,i;

	/* get user name of the shell and assing the amulesign path */
	login=getenv("USER");
	path = malloc(strlen(login) + 64);
	sprintf(path,"/home/%s/.aMule/amulesig.dat",login);

	/* open the file and if not exists exit with an error */
	if ((amulesig = fopen(path, "r")) == NULL)
	{
		puts("Unable to open the file\nCheck if you have amule sig enabled");
		return 0;
	}

	/* initialize all the strings with nothing */
	for (i=0;i<= 19;i++)
		stats[i][0]=0;

	/* start reading the stuff from amulesign to the stats array */
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

	/* if amule isnt running say that and exit else print out the stuff */
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
		printf("Session Download: %s MB, Upload %s MB\n",stats[13],stats[14]);
		printf("Download : %s kB/s, Upload : %s kB/s\n",stats[5],stats[6]);
		printf("Sharing : %s file(s), Clients on queue: %s\n",stats[8],stats[7]);
	}
	return 0;
}
