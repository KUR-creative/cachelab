#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define	ASS	"ass"

typedef struct {
	union {
		int		i;
		double	d;
	};
	int			num;
} uni;

int main(int argc, char *argv[])
{
	uni		some = { {10}, 1 };
	uni*	ptr	= &some;
	printf("%d \n", some.i);
	printf("%d \n\n", some.d);

	printf("%d \n", ptr->i);
	printf("%d \n", ptr->d);

    int flags, opt;
    int nsecs, tfnd;

	char str [80];
  float f;
  FILE * pFile;

  pFile = fopen ("myfile.txt","w+");
  fprintf (pFile, "%f %s", 3.1416, "PI");
  rewind (pFile);
  fscanf (pFile, "%f", &f);
  fscanf (pFile, "%s", str);
  fclose (pFile);
  printf ("I have read: %f and %s \n",f,str);

	for(int i = 0; i < argc; i++){
		printf("%s \n", argv[i]);
	}

    nsecs = 0;
    tfnd = 0;
    flags = 0;
    while ((opt = getopt(argc, argv, "nt:")) != -1) {
		switch (opt) {
	    case 'n':
	        flags = 1;
	        break;
	    case 't':
			printf(">>> %s << \n", optarg);
	        nsecs = atoi(optarg);
	        tfnd = 1;
	        break;
	    default: /* '?' */
	        fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
	                argv[0]);
	        exit(EXIT_FAILURE);
	    }
    }

    printf("flags=%d; tfnd=%d; optind=%d\n", flags, tfnd, optind);
	printf("!!>>%d \n", nsecs);

    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
													     }

		printf("name argument = %s\n", argv[optind]);

	/* Other code omitted */

	exit(EXIT_SUCCESS);
}
