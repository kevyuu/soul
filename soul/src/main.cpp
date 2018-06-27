#include "../soul/src/soul_core.h"
#include <stdio.h>

int main() {
	char kevin[6] = "kevin";
	kevin[5] = '\0';
	printf("Test = %s", kevin);
	Soul::Init();
	Soul::MainLoop();
	Soul::Terminate();


}