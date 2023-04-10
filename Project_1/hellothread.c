#include <stddef.h>
#include <stdio.h>
#include <pthread.h>

void *run(void *arg)
{
	char *string_to_print = arg;
	int i;
	for (i=0; i<5; i++){
		printf("%s: %d\n", string_to_print, i);
	}
	return NULL;
}

int main(void)
{
	pthread_t t1, t2;
	// int x = 12;
	printf("%s\n", "Launching threads");
	pthread_create(&t1, NULL, run, "thread 1");
	pthread_create(&t2, NULL, run, "thread 2");
	pthread_join(t1, NULL);
  pthread_join(t2, NULL);
	printf("%s\n", "Threads complete!");

}
