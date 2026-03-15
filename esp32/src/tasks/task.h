#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class Task
{
public:
    virtual ~Task() = default;

    virtual void Start() = 0;

protected:
    virtual void Run() = 0;

    static void TaskFunc(void* arg)
    {
        static_cast<Task*>(arg)->Run();
        vTaskDelete(nullptr);
    }
};
