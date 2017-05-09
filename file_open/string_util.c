#include <string.h>
#include <ctype.h>

char * ltrim(char  *s)
{
	char * begin;
	begin = s;

	while (*begin != '\0')
	{
		if (isspace(*begin)) {
			begin++;
		} else {
			s = begin;
			break;
		} // end of if
	} // end of while

	return s;
}

char * rtrim(char * s)
{
	char  *end;

	end = s + strlen(s) - 1;
	while (end != s && isspace(*end))
		end--;
	*(end + 1) = '\0';

	return s;
}

char * trim(char  *s)
{
	return rtrim(ltrim(s));
}
