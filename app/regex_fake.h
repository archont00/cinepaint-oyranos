/* regex_fake.h
// In-memory buffer used instead of pipe to plug-ins
// Copyright Mar 6, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#ifndef REGEX_FAKE_H
#define REGEX_FAKE_H

typedef char regex_t;
typedef char regmatch_t;

#define regcomp(a,b,c) 

#define regexec(a,b,c,d,e) 0

/*
int regcomp(regex_t *preg, const char *regex, int cflags);
int regexec(const  regex_t  *preg,  const  char *string, size_t nmatch,
         regmatch_t pmatch[], int eflags);

int regcomp(regex_t *preg, const char *regex, int cflags)
{	return 0;
}

int regexec(const  regex_t  *preg,  const  char *string, size_t nmatch,
         regmatch_t pmatch[], int eflags)
{	return 0;
}
*/

#endif

