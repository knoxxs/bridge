#include "access.h"

void signalHandler();

int main(){

	static struct sigaction sigstruct;
	sigfillset(&(sigstruct.sa_mask));//add all the signals
	sigstruct.sa_handler = signalHandler;
	sigstruct.sa_flags |= SA_RESTART;
}

void signalHandler(){

}