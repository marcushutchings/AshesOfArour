#include "app.h"
#include "fixed_math.h"
#include <SPI.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <Gamebuino.h>

#ifdef __cplusplus
extern "C" {
#endif
extern uint8_t *get_display_buffer(void);
extern void set_display_buffer(uint8_t *p);
extern uint16_t draw_vertical(uint16_t, uint16_t, uint8_t);
#ifdef __cplusplus
}
#endif

#define MAP_WIDTH 14
#define MAP_HEIGHT 14

uint8_t active_map[MAP_HEIGHT][MAP_WIDTH] =
{
	{0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11},
	{0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01},
	{0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01},
	{0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01},
	{0x10,0x00,0x11,0x00,0x11,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01},
	{0x10,0x00,0x11,0x00,0x00,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01},
	{0x10,0x00,0x11,0x00,0x00,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01},
	{0x10,0x00,0x11,0x00,0x00,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01},
	{0x10,0x00,0x11,0x00,0x00,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01},
	{0x10,0x00,0x11,0x00,0x11,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01},
	{0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01},
	{0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01},
	{0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01},
	{0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11},
};

typedef struct {
	uint8_t w, h;
	uint8_t data[];
} texture;

typedef struct {
	uint8_t w, h;
	uint8_t data[8];
} texture8x8;

PROGMEM const texture8x8 wall_1_texture_1 = {
		8, 8,
		{ 0x00, 0x7e, 0x85, 0x04, 0x04, 0x85, 0x7e, 0x00 }
};

uint16_t frame_counter;

struct entity {
	fixed_vec2d origin;
};
/*
-I /opt/arduino/hardware/arduino/avr/cores/arduino
-I/opt/arduino/hardware/arduino/avr/variants/standard
-I/opt/arduino/hardware/arduino/avr/libraries/SPI/src
-I./libraries/Gamebuino-Classic
-I./libraries/Gamebuino-Classic/utility
-mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=185 -DARDUINO_ARCH_AVR -D__PROG_TYPES_COMPAT__
*/

struct player : public entity {
	fixed_vec2d orientation;
};

extern Gamebuino gb;

// ticks once every ms
void app::start_timing_frame(void) {
	frame_counter = 0;
    // Set the Timer Mode to CTC
    TCCR2A |= (1 << WGM01);
    // Set the value that you want to count to
    OCR0A = 0xF9;
    TIMSK2 |= (1 << OCIE2A);    //Set the ISR COMPA vect
    sei();         //enable interrupts
	TCNT2 = 0;
	// 000 = off, 001 x1. 010 x8, 011 x64, 100 x256, 101 x1024
	TCCR2B |= (1 << CS21);
    //TCCR2B |= (1 << CS21) | (1 << CS20);
    // set prescaler to 64 and start the timer
}

void app::stop_timing_frame(void) {
	TCCR2B = 0;
}

ISR (TIMER2_COMPA_vect)
{
    frame_counter++;
}

#define F2D(x) (((double)x)/65536.0)

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
//extern int lock;

#define PRINT_VAR(x) {\
		Serial.print(F(#x ": "));\
		Serial.print(x, HEX);\
		Serial.print(F(" ("));\
		Serial.print(F2D(x));\
		Serial.println(F(")"));\
	}

#define PRINT_INLINE_VAR(x) {\
		Serial.print(F(#x ": "));\
		Serial.print(x, HEX);\
		Serial.print(F(" ("));\
		Serial.print(F2D(x));\
		Serial.print(F(") "));\
	}

uint8_t angle = 128;
fixed dir_x = fsine(angle);
fixed dir_y = fcosine(angle);
fixed pos_x = F_(4);
fixed pos_y = 0x71000;
//fixed velocity = 0;

//#define DEBUG_PRINT

int first_run = 1;

void app::run_frame(void) {
	//if (lock) return;

	// use camera horizontal fov 90 deg for now, means 60 deg fov vertical
	//fixed plane_x = F_(1.2); // FOV = 100.38
	//fixed plane_z = F_(0.68571428571428571429); // FOV = 68.88
	fixed plane_x = fsine(angle-64);
	fixed plane_y = fcosine(angle-64);

	uint8_t* buffer = NULL;
	// don't do this every frame!
	if (first_run){
	buffer = gb.display.getBuffer();
	//Serial.print(F("buffer: "));
	//Serial.println((uint16_t)buffer, HEX);
	set_display_buffer(buffer);
	//Serial.print(F("buffer: "));
	//Serial.println((uint16_t)buffer, HEX);
	first_run=0;
	}

	buffer = get_display_buffer();
	//Serial.print((uint16_t)buffer, HEX);
	//Serial.print(F(" "));
	//return;

#ifdef DEBUG_PRINT
	Serial.print(F("angle: "));
	Serial.println(angle);
	PRINT_VAR(pos_x);
	PRINT_VAR(pos_y);
	PRINT_VAR(dir_x);
	PRINT_VAR(dir_y);
	PRINT_VAR(plane_x);
	PRINT_VAR(plane_y);
	Serial.println(F("Done Dir"));
#endif

	const int8_t w = 84;
	const uint8_t h = 48; // should 126, but this is better for performance  //(w>>1)+w;
	const fixed camera_step_x = F_(1.0 / ((double)(w)));
	fixed ray_dir_step_x = f32_imult(plane_x, camera_step_x);
	fixed ray_dir_step_y = f32_imult(plane_y, camera_step_x);

	//fixed camera_x = 0;
	fixed ray_dir_x = dir_x + (-(plane_x>>1));
	fixed ray_dir_y = dir_y + (-(plane_y>>1));

	fixed map_x = pos_x & 0xffff0000;
	fixed map_y = pos_y & 0xffff0000;

	int8_t start_map_x = (map_x & 0x00ff0000)>>16;
	int8_t start_map_y = (map_y & 0x00ff0000)>>16;
/*
	Serial.print(angle);
	Serial.print(" ");
	PRINT_INLINE_VAR(dir_x);
	PRINT_INLINE_VAR(dir_y);
	PRINT_INLINE_VAR(plane_x);
	PRINT_INLINE_VAR(plane_y);
	PRINT_INLINE_VAR(pos_x);
	PRINT_INLINE_VAR(pos_y);
	Serial.print("    ");
	Serial.print("\r");
*/
#ifdef DEBUG_PRINT
	PRINT_VAR(camera_step_x);
	PRINT_VAR(ray_dir_step_x);
	PRINT_VAR(ray_dir_step_y);
	PRINT_VAR(map_x);
	PRINT_VAR(map_y);
	Serial.print(F("cur_map_x: "));
	Serial.println(start_map_x);
	Serial.print(F("cur_map_y: "));
	Serial.println(start_map_y);
	Serial.println(F("Done Camera"));
#endif

	#define shift_per_sq 6
	#define units_per_sq (1<<shift_per_sq)

	for (int8_t x = 0; x<w; x++)
	{
		fixed fract_dist_x = f32_idiv(f1, ray_dir_x);
		fixed fract_dist_y = f32_idiv(f1, ray_dir_y);

		int8_t step_x;
		int8_t step_y;

		fixed side_dist_x;
		fixed side_dist_y;

		fixed delta_dist_x = fract_dist_x;
		fixed delta_dist_y = fract_dist_y;

		bool hit = 0;
		bool side_hit;

		if (ray_dir_x < 0)
		{
			step_x = -1;
			delta_dist_x = -delta_dist_x;
			side_dist_x = f32_imult((pos_x - map_x), delta_dist_x);
		}
		else
		{
			step_x = 1;
			//if (x==40) PRINT_VAR((map_x+f1-pos_x));
			side_dist_x = f32_imult((map_x+f1-pos_x), delta_dist_x);
		}
		if (ray_dir_y < 0)
		{
			step_y = -1;
			delta_dist_y = -delta_dist_y;
			side_dist_y = f32_imult((pos_y - map_y), delta_dist_y);
		}
		else
		{
			step_y = 1;
			//if (x==40) PRINT_VAR((map_y+f1-pos_y));
			side_dist_y = f32_imult((map_y+f1-pos_y), delta_dist_y);
		}

#ifdef DEBUG_PRINT
		if (x<2){
			PRINT_VAR(fract_dist_x);
			PRINT_VAR(fract_dist_y);
			PRINT_VAR(delta_dist_x);
			PRINT_VAR(delta_dist_y);
			PRINT_VAR(side_dist_x);
			PRINT_VAR(side_dist_y);
		Serial.print(F("step_x: "));
		Serial.println(step_x);
		Serial.print(F("step_y: "));
		Serial.println(step_y);
		Serial.println(F("Done Line Prep"));
		}
#endif

		int8_t cur_map_x = start_map_x;
		int8_t cur_map_y = start_map_y;
		while (!hit){
			if (side_dist_x < side_dist_y)
			{
				side_dist_x += delta_dist_x;
				cur_map_x += step_x;
				side_hit = 0;
			}
			else
			{
				side_dist_y += delta_dist_y;
				cur_map_y += step_y;
				side_hit = 1;
			}
			hit = active_map[cur_map_y][cur_map_x];

#ifdef DEBUG_PRINT
			if (x<2){
			Serial.print(F("map["));
			Serial.print(cur_map_y);
			Serial.print(F("]["));
			Serial.print(cur_map_x);
			Serial.print(F("] = "));
			Serial.println(side_hit);
			}
#endif
		}

		fixed fcur_map_x = ((fixed)cur_map_x)<<16;
		fixed fcur_map_y = ((fixed)cur_map_y)<<16;

		fixed perp_wall_dist;

		if (side_hit == 0)
		{
			fixed lat_dist = fcur_map_x - pos_x + (((fixed)((1 - step_x)>>1))<<16);
			perp_wall_dist = f32_imult(lat_dist, fract_dist_x);
		}
		else
		{
			fixed lat_dist = fcur_map_y - pos_y + (((fixed)((1 - step_y)>>1))<<16);
			perp_wall_dist = f32_imult(lat_dist, fract_dist_y);
		}


		fixed real_wall_height = f32_idiv(f1, perp_wall_dist);
		// convert wall height 0-1 to 0-64. +1 is to aid rounding
		uint8_t line_height = ((real_wall_height>>9)+1)>>1;
		// reduce 64 to 3/4 to a max of 48
		line_height -= (((line_height>>1)+1)>>1);

		int16_t draw_texture_y_offset_steps = 0;
		int8_t draw_start = (-((line_height+1)>>1)) + 24;
		if (draw_start < 0) {
			draw_texture_y_offset_steps = 0 - draw_start;
			draw_start = 0;
		}
		int8_t draw_end = ((line_height+1)>>1) + 24;
		if (draw_end >= h) draw_end = h-1;

#ifdef DEBUG_PRINT
		if (x < 2){
			PRINT_VAR(fcur_map_x);
			PRINT_VAR(fcur_map_y);
			PRINT_VAR(perp_wall_dist);
		Serial.print(F("line_height: "));
		Serial.println(line_height);
		Serial.print(F("draw_start: "));
		Serial.println(draw_start);
		Serial.print(F("draw_end: "));
		Serial.println(draw_end);
		Serial.println(F("Done Draw"));
		}
#endif

		fixed wall_x;
		if (side_hit == 0){

			//wall_x = pos_y + f32_imult(perp_wall_dist, ray_dir_y);

			fixed step_1 = f32_imult(perp_wall_dist, ray_dir_y);
			wall_x = pos_y + step_1;

			//if (x < 1){
			//	PRINT_INLINE_VAR(step_1);
			//	PRINT_INLINE_VAR(wall_x);
			//	PRINT_INLINE_VAR(pos_y);
			//	PRINT_INLINE_VAR(perp_wall_dist);
			//	PRINT_INLINE_VAR(ray_dir_y);
			//	Serial.println(F(""));
			//}
		}
		else wall_x = pos_x + f32_imult(perp_wall_dist, ray_dir_x);

		wall_x = wall_x & 0xffff;
		//if (x < 1){
		//	PRINT_INLINE_VAR(wall_x);
		//}
		//fixed step_1 = ((fixed)wall_1_texture_1.w)<<16;
		//fixed step_2 = f32_imult(wall_x, step_1);
		//int8_t texture_x = step_2 >> 16;
		int8_t texture_x = (f32_imult(wall_x, ((fixed)wall_1_texture_1.w)<<16)>>16);
	/*	if (x < 1){
			PRINT_INLINE_VAR(step_1);
			PRINT_INLINE_VAR(step_2);
			PRINT_INLINE_VAR(wall_x);
			Serial.print(F("tx: "));
			Serial.println(texture_x);
		}
*/

/*
		if (x < 1){
			Serial.print(F("side: "));
			Serial.print(side_hit);
			Serial.print(F(" "));
			PRINT_INLINE_VAR(wall_x);
			PRINT_INLINE_VAR(pos_x);
			PRINT_INLINE_VAR(pos_y);
			PRINT_INLINE_VAR(perp_wall_dist);
			PRINT_INLINE_VAR(ray_dir_x);
			PRINT_INLINE_VAR(ray_dir_y);
			Serial.print(F("tx: "));
			Serial.println(texture_x);
		}
		*/

		int8_t ceil_draw_limit = draw_start;
		uint8_t *p = buffer + (x * 6);

		// --
		uint16_t lineh = ((real_wall_height>>1)+1)>>1; // 32->16 with 1.0 * 64
		lineh -= ((lineh>>1)+1)>>1; // divide by 3/4 for screen height
		//lineh = line_height;
		//lineh <<= 8;
		//set_display_buffer(buffer);
		uint16_t ret = draw_vertical(0, lineh, x);

		if (x == 2){
			uint16_t expected = ((uint16_t)draw_start) + (((uint16_t)draw_end)<<8);
			Serial.print(F("input: "));
			Serial.print((uint16_t)p, HEX);
			Serial.print(F(" expected: "));
			Serial.print(lineh);
			Serial.print(F(" actual: "));
			Serial.println(ret, HEX);
		}
		// --

		while (ceil_draw_limit >= 8) {
			//*p = 0xff;
			p++;
			ceil_draw_limit -= 8;
		}

		uint8_t texture = pgm_read_byte(&wall_1_texture_1.data[texture_x]);
		int16_t texture_step_y = (((int16_t)wall_1_texture_1.h)<<8) / ((int16_t)line_height+1);
		//int16_t texture_cur_y = 0;
		uint8_t scaled_texture;
		uint8_t texture_sample_mask = 0x80;
		uint8_t texture_sample = texture & texture_sample_mask;

		int8_t i = 8;
		uint8_t line_drawn = draw_end + (-draw_start) + 1;
		uint8_t floor_pattern_odd = x & 1;
		uint8_t floor_pattern = ((draw_end+1) & 1) == floor_pattern_odd ? 0x80 : 0x0;

		int16_t texture_cur_y = draw_texture_y_offset_steps * texture_step_y;
		uint8_t texture_step = (texture_cur_y & 0xff00)>>8;
		if (texture_step){

			texture_sample_mask >>= texture_step;
			texture_sample = texture & texture_sample_mask;
			texture_cur_y &= 0x00ff;
		}

		// double sampling - TEMP
		//texture_step_y = (texture_step_y + 1)>>1;

		//int16_t texture_step_y = (((int16_t)wall_1_texture_1.h)<<8) / ((int16_t)line_drawn);
/*
		if (x<2){
			Serial.print(F("line_drawn: "));
			Serial.print(line_drawn);
			Serial.print(F("d raw_end+1: "));
			Serial.print(draw_end+1);
			Serial.print(F(" floor_pattern: "));
			Serial.print(floor_pattern, HEX);
			Serial.print(F(" floor_pattern_odd: "));
			Serial.println(floor_pattern_odd);
		}
*/
		//p = buffer + (draw_start / 8) + (x * 6);
		//int8_t off = draw_start & 0x7;
		//i -= off;
		i -= ceil_draw_limit;
		int8_t off = 0;
		uint8_t floor_to_draw = h - draw_end;

		while (ceil_draw_limit){
			scaled_texture = (scaled_texture >> 1) + 0x80;
			ceil_draw_limit--;
		}

#ifdef DEBUG_PRINT
		if (x < 1){
			Serial.print(F("texture x: "));
			Serial.println(texture_x);
			Serial.print(F("texture pattern: "));
			Serial.println(texture);
			Serial.print(F("step_y: "));
			Serial.println(texture_step_y);
			Serial.print(F("offset: "));
			Serial.println(off);
			Serial.print(F("draw_start: "));
			Serial.println(draw_start);
			Serial.print(F("line_drawn: "));
			Serial.println(line_drawn);
			Serial.println(F("Done Draw"));
		}
#endif

		do {
			if (line_drawn - i < 0){
				off = i - line_drawn;
				i = line_drawn;
				floor_to_draw -= off;
			}
			line_drawn -= i;

			/* r20 = i, r21 = scaled texture, r22,r23 = texture cur y
			 * r18 = sample mask, r19 = texture sample, r24,r25 = texture step y
			 * r28 = texture
			 *
			 *	loop:
			 *		lsr r21
			 *		cpse r19,r1
			 *		ori r21,0x80
			 *		add r22,r24
			 *		adc r23,r25
			 *		breq skip
			 *	move_texture_sample:
			 *		lsl r18
			 *		dec r23
			 *		brne move_texture_sample
			 *		mov r19,r28
			 *		and r19,r18
			 *	skip:
			 * 		dec r20
			 * 		brne loop
			*/

			// 10 (9) cycles, 14(13) cycles
			while (i-- > 0){
				/*if (x<1){
				Serial.print(F("texture_cur_y: "));
				Serial.print(texture_cur_y);
				Serial.println(F(""));
				}*/
				// TEMP - commented out to use double sampling instead
				scaled_texture = (scaled_texture >> 1) + (texture_sample ? 0x80 : 0);

				//uint8_t sample_1 = (texture_sample ? 0x80 : 0);
				//uint8_t sample_1_y = texture_cur_y;

				texture_cur_y += texture_step_y;
				uint8_t texture_step = (texture_cur_y & 0xff00)>>8;
				if (texture_step){
					/*if (x<1){
					Serial.print(F("texture_step: "));
					Serial.print(texture_step);
					Serial.println(F(""));
					}*/
					texture_sample_mask >>= texture_step;
					texture_sample = texture & texture_sample_mask;
					texture_cur_y &= 0x00ff;
				}
/*
				// TEMP - double sampling
				uint8_t sample_2 = (texture_sample ? 0x80 : 0);
				uint8_t sample_2_y = texture_cur_y;

				if (sample_1 != sample_2){
					//sample_1_y = sample_1_y > 0x80 ? sample_1_y - 0x80 : sample_1_y;
					//sample_2_y = sample_2_y > 0x80 ? sample_2_y - 0x80 : sample_2_y;
					if (sample_2_y < sample_1_y)
						sample_1 = sample_2;
				}

				scaled_texture = (scaled_texture >> 1) + (sample_1 ? 0x80 : 0);

				texture_cur_y += texture_step_y;
				texture_step = (texture_cur_y & 0xff00)>>8;
				if (texture_step){
					texture_sample_mask >>= texture_step;
					texture_sample = texture & texture_sample_mask;
					texture_cur_y &= 0x00ff;
				}
*/
				// TEMP - double sampling done

				/*
				if (x <1){
					Serial.print(F(" tex: "));
					Serial.print(scaled_texture);
				}*/
			}
			//if (x <1)
			//	Serial.println(F(""));
			//if (off){
			//	scaled_texture = scaled_texture >> off;
			//	off = 0;
			//}
			while (off){
			/*	if (x<2){
					Serial.print(F("scaled_texture: "));
					Serial.print(scaled_texture, HEX);
					Serial.print(F(" floor_pattern: "));
					Serial.print(floor_pattern, HEX);
					Serial.print(F(" off: "));
					Serial.println(off);
				}*/
				scaled_texture = (scaled_texture >> 1) + floor_pattern;
				floor_pattern = 0x80 - floor_pattern;
				off--;
			}

			*p = scaled_texture;
			p++;
			i = 8;
		} while (line_drawn > 0);
/*
		if (x < 1){
			Serial.print(F("ftod: "));
			Serial.print(floor_to_draw);
			Serial.print(F("ctod: "));
			Serial.print(ceil_draw_limit);
			Serial.print(F("draw_start: "));
			Serial.print(draw_start);
			Serial.print(F("draw_end: "));
			Serial.println(draw_end);
		}
*/
		//floor_pattern = floor_pattern_odd ? 0x55 : 0xAA;
		floor_pattern = floor_pattern_odd ? 0xAA : 0x55;
		while (floor_to_draw >= 8){
			*p = floor_pattern;
			p++;
			floor_to_draw -= 8;
		}

		// Ready values for next loop iteration
		//camera_x += camera_step_x;
		ray_dir_x += ray_dir_step_x;
		ray_dir_y += ray_dir_step_y;

//		break;
	}

	if (gb.buttons.pressed(BTN_LEFT) || gb.buttons.repeat(BTN_LEFT,1))
		angle += 1;
	else if (gb.buttons.pressed(BTN_RIGHT) || gb.buttons.repeat(BTN_RIGHT,1))
		angle -= 1;

	dir_x = fsine(angle);
	dir_y = fcosine(angle);

	if (gb.buttons.pressed(BTN_UP) || gb.buttons.repeat(BTN_UP,1)){
		pos_x += dir_x >> 3;
		pos_y += dir_y >> 3;
	} else if (gb.buttons.pressed(BTN_DOWN) || gb.buttons.repeat(BTN_DOWN,1)) {
		pos_x -= dir_x >> 4;
		pos_y -= dir_y >> 4;
	}

	/*Serial.print("facing ");
	Serial.print(dir_x);
	Serial.print(", ");
	Serial.print(dir_y);
	Serial.println("");*/
//	stop_timing_frame();
	//Serial.println(F("==== Done ============================="));

	//Serial.println(gb.frameDurationMicros);

}

