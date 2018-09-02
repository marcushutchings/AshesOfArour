#include <SPI.h>
#include <Gamebuino.h>
#include "app.h"

Gamebuino gb;

extern "C" {
	void setup(void);
	void loop(void);
}

void setup() {
	gb.begin();
	Serial.begin(115200);
}
int lock = 0;
app ashes;
int8_t c = 0;

void loop() {
	while (gb.update()){
		if (c==0)	{
			gb.titleScreen(F("Ashes of Arour"));
			c = 40;
			lock = 0;
		}
		ashes.run_frame();
		//c--;
	}
}

