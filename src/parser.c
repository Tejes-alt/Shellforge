#define _POSIX_C_SOURCE 200809L
#include "shellforge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

static int add_arg(Command *c, const char *s) {
    if (c->argc >= SF_MAX_ARGS-1) { fprintf(stderr, "shellforge: too many arguments (max %d)\n", SF_MAX_ARGS-1); return -1; }
    c->argv[c->argc++] = sf_strdup(s); c->argv[c->argc] = NULL; return 0;
}
static int add_text(char **buf, size_t *len, size_t *cap, char ch) {
    if (*len + 1 >= *cap) {
        size_t nc = *cap ? *cap * 2 : 64;
        char *p = realloc(*buf, nc);
        if (!p) return -1;
        *buf = p;
        *cap = nc;
    }
    (*buf)[(*len)++] = ch;
    (*buf)[*len] = '\0';
    return 0;
}
static int finish_word(Command *c, char **buf, size_t *len, size_t *cap) {
    if (!*buf || *len==0) { free(*buf); *buf=NULL; *len=0; *cap=0; return 0; }
    int r=add_arg(c,*buf); free(*buf); *buf=NULL; *len=0; *cap=0; return r;
}
static int set_redir(Command *c, RedirType t, const char *path) {
    char **dst = (t==REDIR_IN)?&c->infile:&c->outfile;
    if (*dst) { fprintf(stderr,"shellforge: duplicate redirection\n"); return -1; }
    *dst=sf_strdup(path); c->out_type=t; return 0;
}

static int append_variable(char **buf, size_t *len, size_t *cap, const char *line, size_t *idx) {
    size_t i = *idx + 1;
    if (line[i] == '$') {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%ld", (long)getpid());
        for (size_t k = 0; tmp[k]; k++) if (add_text(buf, len, cap, tmp[k]) < 0) return -1;
        *idx = i;
        return 0;
    }
    if (line[i] == '?') {
        const char *value = "0";
        for (size_t k = 0; value[k]; k++) if (add_text(buf, len, cap, value[k]) < 0) return -1;
        *idx = i;
        return 0;
    }
    if (!(isalpha((unsigned char)line[i]) || line[i] == '_')) {
        return add_text(buf, len, cap, '$');
    }
    size_t start = i;
    while (isalnum((unsigned char)line[i]) || line[i] == '_') i++;
    size_t n = i - start;
    char *name = malloc(n + 1);
    if (!name) return -1;
    memcpy(name, line + start, n);
    name[n] = '\0';
    const char *value = getenv(name);
    free(name);
    if (value) for (size_t k = 0; value[k]; k++) if (add_text(buf, len, cap, value[k]) < 0) return -1;
    *idx = i - 1;
    return 0;
}

int parse_line(const char *line, Pipeline *p) {
    memset(p,0,sizeof(*p));
    p->count=1;
    Command *c=&p->commands[0];
    char *buf=NULL; size_t len=0, cap=0;
    bool sq=false,dq=false,esc=false,had=false;
    size_t display_len=strlen(line);
    p->display=sf_strdup(line);
    if(display_len && p->display[display_len-1]=='\n') p->display[display_len-1]='\0';

    for(size_t i=0;;i++) {
        char ch=line[i];
        if(esc) { if(!ch) { fprintf(stderr,"shellforge: trailing escape\n"); goto fail; } if(add_text(&buf,&len,&cap,ch)<0) goto oom; esc=false; had=true; continue; }
        if(ch=='\\' && !sq) { esc=true; had=true; continue; }
        if(sq) { if(ch=='\0'){fprintf(stderr,"shellforge: unterminated single quote\n");goto fail;} if(ch=='\''){sq=false;} else {if(add_text(&buf,&len,&cap,ch)<0)goto oom;} had=true; continue; }
        if(dq) { if(ch=='\0'){fprintf(stderr,"shellforge: unterminated double quote\n");goto fail;} if(ch=='"'){dq=false;} else if(ch=='$'){if(append_variable(&buf,&len,&cap,line,&i)<0)goto oom;} else {if(add_text(&buf,&len,&cap,ch)<0)goto oom;} had=true; continue; }
        if(ch=='\0' || isspace((unsigned char)ch)) { if(had){if(finish_word(c,&buf,&len,&cap)<0)goto fail;had=false;} if(ch=='\0')break; continue; }
        if(ch=='\''){sq=true;had=true;continue;} if(ch=='"'){dq=true;had=true;continue;} if(ch=='$'){if(append_variable(&buf,&len,&cap,line,&i)<0)goto oom;had=true;continue;}
        if(ch=='|') { if(finish_word(c,&buf,&len,&cap)<0)goto fail; had=false; if(c->argc==0){fprintf(stderr,"shellforge: empty pipeline command\n");goto fail;} if(p->count>=SF_MAX_CMDS){fprintf(stderr,"shellforge: too many pipeline commands\n");goto fail;} c=&p->commands[p->count++]; continue; }
        if(ch=='<' || ch=='>') {
            if(finish_word(c,&buf,&len,&cap)<0) goto fail;
            had=false;
            RedirType rt=(ch=='<')?REDIR_IN:REDIR_OUT;
            if(ch=='>' && line[i+1]=='>'){rt=REDIR_APPEND;i++;}
            while(isspace((unsigned char)line[i+1]))i++;
            if(!line[i+1]){fprintf(stderr,"shellforge: missing redirection target\n");goto fail;}
            i++; char *path=NULL; size_t pl=0,pc=0; bool psq=false,pdq=false,pesc=false;
            for(;;i++) { char x=line[i]; if(pesc){if(add_text(&path,&pl,&pc,x)<0){free(path);goto oom;}pesc=false;continue;} if(x=='\\'&&!psq){pesc=true;continue;} if(psq){if(x=='\0'){free(path);fprintf(stderr,"shellforge: unterminated quote in redirection\n");goto fail;}if(x=='\''){psq=false;}else if(add_text(&path,&pl,&pc,x)<0){free(path);goto oom;}continue;} if(pdq){if(x=='\0'){free(path);fprintf(stderr,"shellforge: unterminated quote in redirection\n");goto fail;}if(x=='"'){pdq=false;}else if(add_text(&path,&pl,&pc,x)<0){free(path);goto oom;}continue;} if(x=='\''){psq=true;continue;} if(x=='"'){pdq=true;continue;} if(x=='\0'||isspace((unsigned char)x)||x=='|'||x=='<'||x=='>'){break;} if(add_text(&path,&pl,&pc,x)<0){free(path);goto oom;} }
            if(!path||pl==0){free(path);fprintf(stderr,"shellforge: missing redirection target\n");goto fail;}
            if(set_redir(c,rt,path)<0){free(path);goto fail;} free(path); i--; continue;
        }
        if(ch=='&' && !buf && (line[i+1]=='\0'||isspace((unsigned char)line[i+1]))) { if(c->argc==0 && p->count==1){fprintf(stderr,"shellforge: unexpected '&'\n");goto fail;} p->background=true; continue; }
        if(add_text(&buf,&len,&cap,ch)<0) goto oom;
        had=true;
    }
    if(c->argc==0){ if(p->count>1){fprintf(stderr,"shellforge: pipeline cannot end with '|\n");goto fail;} p->count=0; }
    return 0;
oom: fprintf(stderr,"shellforge: out of memory\n");
fail: pipeline_free(p); return -1;
}
void pipeline_free(Pipeline *p) { if(!p)return; for(int i=0;i<p->count;i++){Command*c=&p->commands[i];for(int j=0;j<c->argc;j++)free(c->argv[j]);free(c->infile);free(c->outfile);} free(p->display); memset(p,0,sizeof(*p)); }
