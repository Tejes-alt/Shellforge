#define _POSIX_C_SOURCE 200809L
#include "shellforge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

int main(void){jobs_init();job_control_init();using_history();stifle_history(1000);
 printf("=====================================\n      Shellforge\n A Unix Style Shell written in C\n=====================================\n");fflush(stdout);
 while(1){jobs_notify_done();char cwd[256];const char*home=getenv("HOME");const char*display=getcwd(cwd,sizeof cwd)?cwd:"?";char prompt[512];if(home&&strncmp(display,home,strlen(home))==0)snprintf(prompt,sizeof prompt,"shellforge:%s$ ",display+strlen(home));else snprintf(prompt,sizeof prompt,"shellforge:%s$ ",display);char*line=readline(prompt);if(!line){putchar('\n');break;}if(!*line){free(line);continue;}add_history(line);Pipeline p;if(parse_line(line,&p)==0)execute_pipeline(&p);pipeline_free(&p);free(line);}return 0;}
