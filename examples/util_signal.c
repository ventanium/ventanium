/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <stdio.h>
#include <vtm/core/error.h>
#include <vtm/util/thread.h>
#include <vtm/util/signal.h>

volatile bool running;

void handler(int psig)
{
	enum vtm_signal_type sig;

	if (vtm_signal_convert_from_os(psig, &sig) != VTM_OK)
		return;

	switch (sig) {
		case VTM_SIG_INT:
			vtm_signal_safe_puts("Received SIGINT\n");
			break;

		case VTM_SIG_HUP:
			vtm_signal_safe_puts("Received SIGHUP\n");
			break;

		case VTM_SIG_TERM:
			vtm_signal_safe_puts("Received SIGTERM\n");
			break;

		default:
			vtm_signal_safe_puts("Received unknown signal\n");
			break;
	}

	running = false;
}

int main(void)
{
	/* register signal handlers */
	vtm_signal_set_handler(VTM_SIG_INT, handler);
	vtm_signal_set_handler(VTM_SIG_HUP, handler);
	vtm_signal_set_handler(VTM_SIG_TERM, handler);

	/* display message to user */
	printf("Running loop, please interrupt me with SIGINT, SIGHUP or SIGTERM\n");
	fflush(stdout);

	/* run loop */
	running = true;
	while (running)
		vtm_thread_sleep(1000);

	printf("Finished!\n");

	return 0;
}
