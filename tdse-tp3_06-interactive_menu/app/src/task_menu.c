/*
 * Copyright (c) 2023 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @file   : task_menu.c
 * @date   : Set 26, 2023
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 * @version	v1.0.0
 */

/********************** inclusions *******************************************/
#include "main.h"
#include "logger.h"
#include "dwt.h"
#include "board.h"
#include "app.h"
#include "task_menu_attribute.h"
#include "task_menu_interface.h"
#include "display.h"

#define G_TASK_MEN_CNT_INI           0ul
#define G_TASK_MEN_TICK_CNT_INI      0ul
#define CUSTOM_DELAY_TICKS          37u

/* Variables auxiliares para el menú */
static uint8_t selectedMotor = 0;
static uint8_t selectedParam = 1; // 1: Power, 2: Speed, 3: Spin
static uint8_t valueTemp = 0;

/* Datos de tarea */
task_menu_dta_t task_menu_dta = {CUSTOM_DELAY_TICKS, ST_MEN_IDLE, EV_MEN_ENT_IDLE, false};

uint32_t g_task_menu_cnt;
volatile uint32_t g_task_menu_tick_cnt;

void task_menu_init(void *parameters)
{
	LOGGER_LOG("Task Menu (Interactive Menu) iniciado\r\n");
	g_task_menu_cnt = G_TASK_MEN_CNT_INI;
	g_task_menu_tick_cnt = G_TASK_MEN_TICK_CNT_INI;
	init_queue_event_task_menu();
	displayInit(DISPLAY_CONNECTION_GPIO_4BITS);
	displayCharPositionWrite(0, 0);
	displayStringWrite("TdSE Bienvenidos");
	displayCharPositionWrite(0, 1);
	displayStringWrite("Test Nro: ");
	cycle_counter_init();
	cycle_counter_reset();
}

void task_menu_update(void *parameters)
{
	char menu_str[8];
	g_task_menu_cnt++;

	__asm("CPSID i");
	if (G_TASK_MEN_TICK_CNT_INI < g_task_menu_tick_cnt)
		g_task_menu_tick_cnt--;
	__asm("CPSIE i");

	if (G_TASK_MEN_TICK_CNT_INI < g_task_menu_tick_cnt)
		return;

	g_task_menu_tick_cnt = CUSTOM_DELAY_TICKS;
	task_menu_dta_t *p = &task_menu_dta;

	if (p->tick > 0)
		p->tick--;
	else {
		snprintf(menu_str, sizeof(menu_str), "%lu", g_task_menu_cnt / 1000ul);
		displayCharPositionWrite(10, 1);
		displayStringWrite(menu_str);
		p->tick = CUSTOM_DELAY_TICKS;

		if (any_event_task_menu()) {
			p->flag = true;
			p->event = get_event_task_menu();
		}

		if (!p->flag) return;

		switch (p->state)
		{
			case ST_MEN_IDLE:
				if (p->event == EV_MEN_ENT_ACTIVE) {
					p->flag = false;
					p->state = ST_MEN_MENU1;
				}
				break;

			case ST_MEN_MENU1:
				if (p->event == EV_MEN_NEX_ACTIVE) {
					selectedMotor = (selectedMotor + 1) % 2;
				} else if (p->event == EV_MEN_ENT_ACTIVE) {
					p->state = ST_MEN_MENU2;
				} else if (p->event == EV_MEN_ESC_ACTIVE) {
					p->state = ST_MEN_IDLE;
				}
				p->flag = false;
				break;

			case ST_MEN_MENU2:
				if (p->event == EV_MEN_NEX_ACTIVE) {
					selectedParam = (selectedParam % 3) + 1;
				} else if (p->event == EV_MEN_ENT_ACTIVE) {
					switch (selectedParam) {
						case 1: p->state = ST_MEN_MENU3_POWER; break;
						case 2: p->state = ST_MEN_MENU3_SPEED; break;
						case 3: p->state = ST_MEN_MENU3_SPIN;  break;
					}
				} else if (p->event == EV_MEN_ESC_ACTIVE) {
					p->state = ST_MEN_MENU1;
				}
				p->flag = false;
				break;

			case ST_MEN_MENU3_POWER:
				if (p->event == EV_MEN_NEX_ACTIVE) {
					valueTemp = (valueTemp + 1) % 2;
				} else if (p->event == EV_MEN_ESC_ACTIVE) {
					// guardar valor si aplica
					p->state = ST_MEN_MENU2;
				}
				p->flag = false;
				break;

			case ST_MEN_MENU3_SPEED:
				if (p->event == EV_MEN_NEX_ACTIVE) {
					valueTemp = (valueTemp + 1) % 10;
				} else if (p->event == EV_MEN_ESC_ACTIVE) {
					p->state = ST_MEN_MENU2;
				}
				p->flag = false;
				break;

			case ST_MEN_MENU3_SPIN:
				if (p->event == EV_MEN_NEX_ACTIVE) {
					valueTemp = (valueTemp + 1) % 2;
				} else if (p->event == EV_MEN_ESC_ACTIVE) {
					p->state = ST_MEN_MENU2;
				}
				p->flag = false;
				break;
		}
	}
}

/********************** end of file ******************************************/
