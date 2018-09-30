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
extern int16_t div16(int16_t,int16_t);
//extern int16_t imul16(int16_t,int16_t);
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
// ==========================================================================================
// spite rendering
typedef struct {
	uint8_t w, h;
	uint8_t data[32];
} texture8x8;

PROGMEM const texture8x8 wall_1_texture_1 = {
		16, 16,
		{
				0x00, 0x00,
				0x7e, 0x7e,
				0x85, 0x85,
				0x04, 0x04,
				0x04, 0x04,
				0x85, 0x85,
				0x7e, 0x7e,
				0x00, 0x00,
				0x00, 0x00,
				0x7e, 0x7e,
				0x85, 0x85,
				0x04, 0x04,
				0x04, 0x04,
				0x85, 0x85,
				0x7e, 0x7e,
				0x00, 0x00,
		}
};

static const int8_t w = 84;
static const uint8_t h = 48;

fixed zbuffer[w];

uint16_t frame_counter;

struct entity {
	uint8_t typeId;
	fixed_vec2d origin;
	int16_t hp;
	int8_t* initial_frame;
	uint8_t size_radius;
	void(*think)(entity*);
	void(*touch)(entity*);
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
fixed plane_x = fsine(angle-64);
fixed plane_y = fcosine(angle-64);
//fixed velocity = 0;

//#define DEBUG_PRINT

int first_run = 1;

static const int8_t max_ents = 10;
static entity entity_list[max_ents];
static int8_t active_entities = 0;

void sort_list(fixed* dists, int8_t len, int8_t* order) {
	for (int8_t i=1; i<len; i++){
		for (int8_t j=0; j<i; j++){
			if (dists[order[i]] < dists[order[j]]){
				int8_t cur_value = order[i];
				for (int8_t k=i;k>j;k--){
					order[k] = order[k-1];
				}
				order[j] = cur_value;
			}
		}
	}
}
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
void sprite_casting(void) {
	fixed ent_dist[active_entities];
	int8_t ent_order[active_entities];

	uint8_t* buffer = gb.display.getBuffer();

	for (int8_t i = 0; i<active_entities; i++){
		ent_order[i] = i;
		fixed distx = (pos_x - entity_list[i].origin.x);
		distx = f32_imult(distx, distx);
		fixed disty = (pos_y - entity_list[i].origin.y);
		disty = f32_imult(disty, disty);
		ent_dist[i] = distx + disty;
	}

	sort_list(ent_dist, active_entities, ent_order);

	for (int8_t i = 0; i<active_entities; i++){
		fixed x_off = entity_list[ent_order[i]].origin.x - pos_x;
		fixed y_off = entity_list[ent_order[i]].origin.y - pos_y;

		fixed inv_det = f32_idiv( f1, f32_imult(plane_x, dir_y) - f32_imult(plane_y, dir_x) );

		fixed transform_y = f32_imult( inv_det, f32_imult(-plane_y, x_off) + f32_imult(plane_x, y_off) );
		if (transform_y < 0) continue;
		fixed transform_x = f32_imult( inv_det, f32_imult(dir_y, x_off) - f32_imult(dir_x, y_off) );

		fixed inv_transform_y = f32_idiv( f1, transform_y );

		fixed sprite_screen_x_f = f32_imult( (((fixed)(w>>1))<<16), (f1 + f32_imult(transform_x, inv_transform_y)) );
		uint8_t sprite_x =  sprite_screen_x_f >> 16;

		fixed sprite_screen_y_f = f32_imult( (((fixed)h)<<16), inv_transform_y);
		uint8_t sprite_height = sprite_screen_y_f >> 16;
		uint8_t sprite_width = sprite_height;

		// set rate of change on y texture
		uint16_t tex_step_y = div16(0x0800, sprite_screen_y_f>>8);
		uint16_t tex_cur_y_start = 0;

		uint8_t start_draw_y = (-(sprite_height>>1)) + 24;
		if (start_draw_y < 0){
			tex_cur_y_start = ((int16_t)(-start_draw_y))*tex_step_y;
			start_draw_y = 0;
		}
		uint8_t end_draw_y = ((sprite_height-1)>>1) + 24;
		if (end_draw_y >= h) end_draw_y = h-1;

		// set rate of change on x texture
		uint16_t tex_step_x = div16(0x0800, sprite_screen_x_f>>8);
		uint16_t tex_cur_x = 0;

		int8_t start_draw_x = (-(sprite_width>>1)) + sprite_x;
		if (start_draw_x < 0){
			tex_cur_x = ((int16_t)(-start_draw_x))*tex_step_x;
			start_draw_x = 0;
		}
		int8_t end_draw_x = ((sprite_width-1)>>1) + sprite_x;
		if (end_draw_x >= w) end_draw_x = w-1;
/*
		if (i==0){
			Serial.print(start_draw_x);
			Serial.print(" ");
			Serial.print(end_draw_x);
			Serial.print(" ");
			Serial.print(start_draw_y);
			Serial.print(" ");
			Serial.print(end_draw_y);
			Serial.print("\r");
		}*/

		uint8_t cur_texture;

		for (uint8_t x = start_draw_x; x <= end_draw_x; x++){
			if (tex_cur_x & 0xff00){/*
				uint8_t texture_step = texture_cur_y >> 8;
				texture_sample_mask >>= texture_step;
				texture_sample = texture & texture_sample_mask;
				texture_cur_y &= 0x00ff;*/
			}

			if (transform_y < zbuffer[x]){
				uint8_t* pbuffer = &buffer[x*6];
				pbuffer += start_draw_y >> 3;
				uint16_t tex_cur_y = tex_cur_y_start;
				uint8_t i = start_draw_y & 0xff;
				uint8_t write_byte = 0xff;
				if (i == 0)
					i = 8;
				int8_t y_draw_count = sprite_height;
				while (y_draw_count){
			//	for (uint8_t y = start_draw_y; y <= end_draw_y; y++){
					if (y_draw_count - i < 0){
						i = y_draw_count;
					}
					y_draw_count -= i;
					for (; i > 0; i--){
						write_byte = (write_byte << 1) + 0x1;
					}
					*pbuffer = write_byte;
					i = 8;
				}
			}
			tex_cur_x += tex_step_x;
		}
	}
}

void app::run_frame(void) {
	//if (lock) return;

	// use camera horizontal fov 90 deg for now, means 60 deg fov vertical
	//fixed plane_x = F_(1.2); // FOV = 100.38
	//fixed plane_z = F_(0.68571428571428571429); // FOV = 68.88
	plane_x = fsine(angle-64);
	plane_y = fcosine(angle-64);

	uint8_t* buffer = NULL;
	buffer = gb.display.getBuffer();
	// don't do this every frame!
	if (first_run){
		active_entities = 10;
		entity_list[0].origin.x = 0x90000;
		entity_list[0].origin.y = 0x90000;
		entity_list[1].origin.x = 0x40000;
		entity_list[1].origin.y = 0x50000;
	//set_display_buffer(buffer);
	//first_run=0;
	}

	//buffer = get_display_buffer();

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
			side_dist_y = f32_imult((map_y+f1-pos_y), delta_dist_y);
		}

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

		zbuffer[x] = perp_wall_dist;

		fixed real_wall_height = f32_idiv(f1, perp_wall_dist);
		// convert wall height 0-1 to 0-64. +1 is to aid rounding
		uint16_t tmp = real_wall_height>>8;
		tmp >>= 2;
		uint8_t line_height = tmp;
		// reduce 64 to 3/4 to a max of 48
		line_height -= (((line_height>>1)+1)>>1);

		// ==========================================================================================
		// environment rendering

		int16_t draw_texture_y_offset_steps = 0;
		int8_t draw_start = (-(line_height>>1)) + 24;
		if (draw_start < 0) {
			draw_texture_y_offset_steps = 0 - draw_start;
			draw_start = 0;
		}
		int8_t draw_end = ((line_height-1)>>1) + 24;
		if (draw_end >= h) draw_end = h-1;

		fixed wall_x;
		if (side_hit == 0){
			fixed step_1 = f32_imult(perp_wall_dist, ray_dir_y);
			wall_x = pos_y + step_1;
		}
		else wall_x = pos_x + f32_imult(perp_wall_dist, ray_dir_x);
		wall_x = wall_x & 0xffff;
		int8_t texture_x = f32_imult(wall_x, 0x100000)>>16;
		texture_x <<= 1;

		uint8_t *p = buffer + (x * 6);

		int8_t ceil_draw_limit = draw_start;
		while (ceil_draw_limit >= 8) {
			*p = 0xff;
			p++;
			ceil_draw_limit -= 8;
		}
		uint8_t *wall_start_p = p;

		uint8_t texture = pgm_read_byte(&wall_1_texture_1.data[texture_x]);
		uint8_t scaled_texture;
		uint8_t texture_sample_mask = 0x80;
		uint8_t texture_sample = texture & texture_sample_mask;

		int8_t i = 8;
		uint8_t line_drawn = draw_end + (-draw_start) + 1;

		int16_t top_half_height = 0x18 - draw_start;
		int16_t texture_step_y_h = div16(0x0800, ((line_height)>>1));
		int16_t texture_step_y_l = div16(0x0800, ((line_height+1)>>1));
		int8_t top_draw_length = top_half_height;

		int8_t floor_to_draw = h - draw_start;
		uint8_t floor_pattern_odd = x & 1;
		uint8_t floor_pattern = floor_pattern_odd ? 0xAA : 0x55;
		while (floor_to_draw >= 0){
			*p = floor_pattern;
			p++;
			floor_to_draw -= 8;
		}

		int16_t texture_step_y = texture_step_y_h;
		int16_t texture_cur_y = draw_texture_y_offset_steps * texture_step_y;
		if (texture_cur_y & 0xff00){
			uint8_t texture_step = texture_cur_y >> 8;
			texture_sample_mask >>= texture_step;
			texture_sample = texture & texture_sample_mask;
			texture_cur_y &= 0x00ff;
		}

		i -= ceil_draw_limit;
		scaled_texture = 0xff;
		uint8_t floor_mask = 0xff;
		uint8_t push_text = 0;
		p = wall_start_p;
		do {
			if (line_drawn - i < 0){
				push_text = i - line_drawn;
				i = line_drawn;
				floor_mask <<= line_drawn;
			}
			line_drawn -= i;

			while (i-- > 0){
				scaled_texture = (scaled_texture >> 1) + (texture_sample ? 0x80 : 0);
				texture_cur_y += texture_step_y;
				top_draw_length--;
				if (top_draw_length==0){
					texture_step_y = texture_step_y_l;
					texture_cur_y=0;
					texture_sample_mask = 0x80;
					texture = pgm_read_byte(&wall_1_texture_1.data[texture_x+1]);
					texture_sample = texture & texture_sample_mask;
				}
				else if (texture_cur_y & 0xff00){
					uint8_t texture_step = texture_cur_y >> 8;
					texture_sample_mask >>= texture_step;
					texture_sample = texture & texture_sample_mask;
					texture_cur_y &= 0x00ff;
				}
			}

			if (push_text){
				scaled_texture >>= push_text;
				uint8_t text_mask = ~floor_mask;
				scaled_texture = (scaled_texture & text_mask) + (floor_pattern & floor_mask);
			}

			*p = scaled_texture;
			p++;
			i = 8;
		} while (line_drawn > 0);

		// Ready values for next loop iteration
		//camera_x += camera_step_x;
		ray_dir_x += ray_dir_step_x;
		ray_dir_y += ray_dir_step_y;
	}

	sprite_casting();

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

	Serial.println(gb.frameDurationMicros);
}

