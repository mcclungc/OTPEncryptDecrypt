#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define POOLSIZE 27
 
char getRandom()
{
	char randomPool[POOLSIZE] ={'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
	'Q','R','S','T','U','V','W','X','Y','Z',' '};
	int min = 1;
	int randCharIndex = (rand() % (POOLSIZE - min + 1) + 1);
	randCharIndex--;
	//printf("\n randCharIndex is %d\n", randCharIndex);
	return randomPool[randCharIndex];	
}

void printRandChars(int keylength)
{

	int i;
	char randChar;

	for (i = 0; i < keylength; i++)
	{
		randChar = getRandom();
		printf("%c", randChar);
	}
	return;
}

int main(int argc, char *argv[])
{
	srand((int)time(NULL)); //seed rand with current time

	// check for valid # of arguments	
	if(argc < 2)
	{
		fprintf(stderr, "USAGE: %s keylength\n", argv[0]);
		exit(0);
	}
	
	int keylength;
	keylength = atoi(argv[1]);//convert arg 1 to int
	
	printRandChars(keylength); //print ran
	printf("\n");

	return 0;
}
