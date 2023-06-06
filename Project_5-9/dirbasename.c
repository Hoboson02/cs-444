#include <stdio.h>
#include <string.h>
// Inspired by the `dirname` and `basename` Unix utilities
//
// But doesn't handle a trailing '/' on the path correctly.
//
// So don't do that.
char *get_dirname(const char *path, char *dirname)
{
  strcpy(dirname, path);
  char *p = strrchr(dirname, '/');
  if (p == NULL) {
    strcpy(dirname, ".");
    return dirname;
  }
  if (p == dirname) // Last slash is the root /
    *(p+1) = '\0';
    else
    *p = '\0'; // Last slash is not the root /
    return dirname;
}
char *get_basename(const char *path, char *basename)
{
  if (strcmp(path, "/") == 0) {
    strcpy(basename, path);
    return basename;
  }
  const char *p = strrchr(path, '/');
  if (p == NULL)
    p = path; // No slash in name, start at beginning
  else
    p++; // Start just after slash
    strcpy(basename, p);
    return basename;
}
int main(void)
{
  char result[1024];
  puts(get_dirname("/foo/bar/baz", result)); // /foo/bar
  puts(get_dirname("/foo/bar", result)); // /foo
  puts(get_dirname("/foo", result)); // /
  puts(get_dirname("/", result)); // /
  puts(get_dirname("foo", result)); // .
  puts(get_dirname("", result)); // .
  puts(get_basename("/foo/bar/baz", result)); // baz
  puts(get_basename("/foo/bar", result)); // bar
  puts(get_basename("/foo", result)); // foo
  puts(get_basename("/", result)); // /
  puts(get_basename("foo", result)); // foo
  puts(get_basename("", result)); //
}
