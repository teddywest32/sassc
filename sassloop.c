#include <stdlib.h>
#include <stdio.h>
#include "libsass/sass_interface.h"

int main(int argc, char** argv)
{	
	if (argc < 3) {
		printf("Usage: sassloop [INPUT FILE] [ITERATIONS]\n");
		return 0;
	}
	int i;
	for (i = 0; i < atoi(argv[2]); ++i) {
		printf("\n***\n*** PASS %d ***\n***\n\n", i);
		struct sass_file_context* ctx = sass_new_file_context();
		ctx->options.include_paths = "";
		ctx->options.image_path = "images";
		ctx->options.output_style = SASS_STYLE_NESTED;
		ctx->input_path = argv[1];
			
		sass_compile_file(ctx);
		
		if (ctx->error_status) {
	    if (ctx->error_message) printf("%s", ctx->error_message);
	    else printf("An error occured; no error message available.\n");
	  }
		else if (ctx->output_string) {
		  printf("%s", ctx->output_string);
	  }
	  else {
	    printf("Unknown internal error.\n");
	  }
		
	  sass_free_file_context(ctx);
	}
	return 0;
}