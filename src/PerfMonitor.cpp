/**
 * Copyright (c) Vincent Manoukian 2023.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE.md file in the root directory of this source tree.
 */

#include "PerfMonitor.h"

int32_t PerfMonitor::idle0Calls;
int32_t PerfMonitor::idle1Calls;
float PerfMonitor::cpu0;
float PerfMonitor::cpu1;

bool PerfMonitor::idle_task_0()
{
	idle0Calls += 1;
	return false;
}

bool PerfMonitor::idle_task_1()
{
	idle1Calls += 1;
	return false;
}

void PerfMonitor::perfmon_task(void *args)
{
	while (1)
	{
		int32_t idle0 = idle0Calls;
		int32_t idle1 = idle1Calls;
		idle0Calls = 0;
		idle1Calls = 0;

		cpu0 = 100.0-(100.0 * (idle0 / MaxIdleCalls));
		cpu1 = 100.0-(100.0 * (idle1 / MaxIdleCalls));

        log_i("CPU Usage : %.2f/%.2f",PerfMonitor::cpu0, PerfMonitor::cpu1);

		vTaskDelay(pdMS_TO_TICKS( 5000 ));
	}
}

bool PerfMonitor::start()
{
	ESP_ERROR_CHECK(esp_register_freertos_idle_hook_for_cpu(idle_task_0, 0));
	ESP_ERROR_CHECK(esp_register_freertos_idle_hook_for_cpu(idle_task_1, 1));
	xTaskCreate(perfmon_task, "perfmon", 3072, NULL, 1, &perfmon_task_handle);
	return true;
}

bool PerfMonitor::stop()
{
	esp_deregister_freertos_idle_hook_for_cpu(idle_task_0, 0);
	esp_deregister_freertos_idle_hook_for_cpu(idle_task_1, 1);
    if (perfmon_task_handle!=NULL) {
        vTaskDelete(perfmon_task_handle);
    }
	return true;
}
