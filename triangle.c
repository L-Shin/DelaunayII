#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int randomized_flag = 0;
	int fast_flag = 0;
	char *input_filename = NULL;
	char *output_filename = NULL;
	int opt;
	opterr = 0;

	while((opt = getopt(argc, argv, "rfi:o:")) != -1)
	{
		switch(opt)
		{
			case 'r':
				randomized_flag = 1;
				break;
			case 'f':
				fast_flag = 1;
				break;
			case 'i':
				input_filename =optarg;
				break;
			case 'o':
				output_filename = optarg;
				break;
			case '?':
				if (optopt == 'i')
					fprintf(stderr, "Please specify input filename\n");
				else if (isprint (optopt))
					fprintf(stderr, "Unknown option '-%c'\n",optopt);
				else 
					fprintf(stderr, "Unknown option character '\\x%x'\n", optopt);
				return 1;
			default:
				abort();
		}
	}
	if (!input_filename) {
		fprintf(stderr, "Please specify input filename\n");
		return 0;
 	}	
	if (!output_filename) {
		output_filename = "output.ele";
	}
	printf("randomized insertion = %d\nfast point location = %d\ninput filename = %s\noutput_filename = %s\n", randomized_flag, fast_flag, input_filename, output_filename);
 
	for(; optind < argc; optind++){
		printf("extra argument: %s\n", argv[optind]);
	}
	return 0;
}
