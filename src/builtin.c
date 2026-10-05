#define _POSIX_C_SOURCE 200809L
#include "shellforge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/history.h>

static int builtin_jobs(Command*c){(void)c;jobs_print();return 0;}
static int parse_job_arg(const char*s){if(!s||!*s)return -1;if(*s=='%')s++;char*e=NULL;long n=strtol(s,&e,10);if(*e||n<=0||n>9999)return -1;return(int)n;}
static int builtin_fg(Command*c){Job*j=NULL;if(c->argc>1){int id=parse_job_arg(c->argv[1]);j=id>0?jobs_get(id):NULL;}else j=jobs_get_current();if(!j){fprintf(stderr,"shellforge: fg: no such job\n");return 1;}return put_job_foreground(j,j->state==JOB_STOPPED);}
static int builtin_bg(Command*c){Job*j=NULL;if(c->argc>1){int id=parse_job_arg(c->argv[1]);j=id>0?jobs_get(id):NULL;}else j=jobs_get_current();if(!j){fprintf(stderr,"shellforge: bg: no such job\n");return 1;}return put_job_background(j,true);}
static int builtin_cd(Command*c){const char*path=c->argc>1?c->argv[1]:getenv("HOME");if(c->argc>2){fprintf(stderr,"shellforge: cd: too many arguments\n");return 1;}if(chdir(path)<0){fprintf(stderr,"shellforge: cd: %s: %s\n",path,strerror(errno));return 1;}return 0;}
static int builtin_export(Command*c){if(c->argc==1){extern char**environ;for(char**e=environ;*e;e++)puts(*e);return 0;}for(int i=1;i<c->argc;i++){char*eq=strchr(c->argv[i],'=');if(!eq||eq==c->argv[i]){fprintf(stderr,"shellforge: export: invalid assignment: %s\n",c->argv[i]);return 1;}*eq='\0';if(setenv(c->argv[i],eq+1,1)<0){perror("export");*eq='=';return 1;}*eq='=';}return 0;}
static int builtin_unset(Command*c){for(int i=1;i<c->argc;i++)unsetenv(c->argv[i]);return 0;}
static int builtin_echo(Command*c){bool n=false;int i=1;if(i<c->argc&&strcmp(c->argv[i],"-n")==0){n=true;i++;}for(;i<c->argc;i++){if(i>1+(n?1:0))putchar(' ');fputs(c->argv[i],stdout);}if(!n)putchar('\n');fflush(stdout);return 0;}
int is_builtin(const Command*c){if(!c||!c->argv[0])return 0;const char*s=c->argv[0];return !strcmp(s,"cd")||!strcmp(s,"pwd")||!strcmp(s,"echo")||!strcmp(s,"export")||!strcmp(s,"unset")||!strcmp(s,"history")||!strcmp(s,"jobs")||!strcmp(s,"fg")||!strcmp(s,"bg")||!strcmp(s,"exit")||!strcmp(s,"help");}
int execute_builtin(Command*c,bool parent){(void)parent;const char*s=c->argv[0];if(!strcmp(s,"cd"))return builtin_cd(c);if(!strcmp(s,"pwd")){char b[4096];if(getcwd(b,sizeof b))printf("%s\n",b);else perror("pwd");return 0;}if(!strcmp(s,"echo"))return builtin_echo(c);if(!strcmp(s,"export"))return builtin_export(c);if(!strcmp(s,"unset"))return builtin_unset(c);if(!strcmp(s,"history")){HIST_ENTRY**h=history_list();if(h)for(int i=0;h[i];i++)printf("%5d  %s\n",i+history_base,h[i]->line);return 0;}if(!strcmp(s,"jobs"))return builtin_jobs(c);if(!strcmp(s,"fg"))return builtin_fg(c);if(!strcmp(s,"bg"))return builtin_bg(c);if(!strcmp(s,"help")){puts("ShellForge built-ins: cd pwd echo export unset history jobs fg bg exit help");puts("Supports pipes, <, >, >>, background jobs (&), and job control.");return 0;}if(!strcmp(s,"exit")){int st=c->argc>1?atoi(c->argv[1]):0;exit(st);}return 1;}
