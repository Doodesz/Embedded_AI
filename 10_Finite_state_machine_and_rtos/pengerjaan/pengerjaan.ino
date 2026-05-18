/*
RIZQY JAUHARY ATSAANY
235150300111038
TKOM - Embedded Artificial Intelligence - B
*/

#include <Arduino.h>

constexpr int kSampleCount = 10;
constexpr TickType_t kSensorPeriod = pdMS_TO_TICKS(1000);
constexpr TickType_t kInferencePeriod = pdMS_TO_TICKS(10000);

int sensorValues[kSampleCount] = {0};
SemaphoreHandle_t dataMutex = NULL;
SemaphoreHandle_t serialMutex = NULL;
int writeIndex = 0;

void printLine(const char *label, float value) {
    if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
        Serial.print(label);
        Serial.println(value, 2);
        xSemaphoreGive(serialMutex);
    }
}

void printLine(const char *label, int value) {
    if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
        Serial.print(label);
        Serial.println(value);
        xSemaphoreGive(serialMutex);
    }
}

void sensorTask(void *pvParameters) {
    (void)pvParameters;

    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;) {
        int reading = random(50, 101);

        if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
            sensorValues[writeIndex] = reading;
            writeIndex = (writeIndex + 1) % kSampleCount;
            xSemaphoreGive(dataMutex);
        }

        printLine("Sensor reading: ", reading);
        vTaskDelayUntil(&lastWakeTime, kSensorPeriod);
    }
}

void inferenceTask(void *pvParameters) {
    (void)pvParameters;

    TickType_t lastWakeTime = xTaskGetTickCount();
    int localValues[kSampleCount] = {0};

    for (;;) {
        if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < kSampleCount; i++) {
                localValues[i] = sensorValues[i];
            }
            xSemaphoreGive(dataMutex);
        }

        int total = 0;
        for (int i = 0; i < kSampleCount; i++) {
            total += localValues[i];
        }

        float inferenceResult = total / kSampleCount;
        printLine("AI inference (average 10 data): ", inferenceResult);

        vTaskDelayUntil(&lastWakeTime, kInferencePeriod);
    }
}

void setup() {
    Serial.begin(115200);
    randomSeed(micros());

    dataMutex = xSemaphoreCreateMutex();
    serialMutex = xSemaphoreCreateMutex();

    if (dataMutex == NULL || serialMutex == NULL) {
        while (true) {
            delay(1000);
        }
    }

    xTaskCreateUniversal(sensorTask, "SensorTask", 4096, NULL, 2, NULL, 1);
    xTaskCreateUniversal(inferenceTask, "InferenceTask", 4096, NULL, 1, NULL, 0);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}

/*
NOTES:
- pdMS_TO_TICKS converts milliseconds to ticks, which is the time unit used by FreeRTOS for task scheduling. This allows you to specify task delays and periods in milliseconds while ensuring that the timing is accurate according to the FreeRTOS tick rate.
- xSemaphoreTake and xSemaphoreGive are used to manage access to shared resources (like sensorValues and Serial) between tasks
- vTaskDelayUntil is used to create periodic tasks that execute at regular intervals, ensuring that the sensor readings and AI inference happen at the specified periods.
- TickType_t is a data type used by FreeRTOS to represent time in ticks, which is the fundamental time unit for task scheduling and delays in FreeRTOS.
- portMAX_DELAY is a constant that indicates an infinite timeout when waiting for a semaphore, meaning the task will block indefinitely until the semaphore becomes available.
- pdTRUE is a constant that indicates a successful operation, such as successfully taking a semaphore. It is used to check the return value of functions like xSemaphoreTake to ensure that the operation was successful before proceeding with critical sections of code.
- xTaskGetTickCount is a function that returns the current tick count, which is used to track time in FreeRTOS. It is often used in conjunction with vTaskDelayUntil to create periodic tasks that execute at regular intervals.
- vTaskDelayUntil is a function that delays a task until a specified tick count is reached. It is used to create periodic tasks that execute at regular intervals, ensuring that the task runs at the correct timing based on the FreeRTOS tick count.
- xTaskCreateUniversal is a function that creates a new task in FreeRTOS. It allows you to specify the task function, name, stack size, parameters, priority, and core affinity (for dual-core systems). In this code, it is used to create the sensorTask and inferenceTask with specific priorities and core affinities.
- static_cast is a C++ operator used to perform explicit type conversions. In this code, it is used to convert the total of sensor values (an integer) to a float before calculating the average for the AI inference result. This ensures that the division operation produces a floating-point result rather than an integer result, which is important for accurate average calculation.
*/
