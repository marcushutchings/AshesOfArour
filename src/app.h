#ifndef APP_H_
#define APP_H_

#include <stdint.h>

class app {
public:
	void run_frame(void);
private:
	void start_timing_frame(void);
	void stop_timing_frame(void);
};
#endif
