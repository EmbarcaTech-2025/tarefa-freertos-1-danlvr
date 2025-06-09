#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#include "FreeRTOS.h"
#include "task.h"

#include "ssd1306.h"

// --- Definições de Pinos ---
#define LED_R_PIN 13 // Pino para o componente Vermelho do LED RGB
#define LED_G_PIN 11 // Pino para o componente Verde do LED RGB
#define LED_B_PIN 12 // Pino para o componente Azul do LED RGB

#define BUZZER_A_PIN 21      // Pino de controle A do Buzzer (PWM)
#define BUZZER_FREQUENCY 500 // Frequência do Buzzer em Hz

#define BUTTON_A_PIN 5 // Botão para controlar a tarefa do LED
#define BUTTON_B_PIN 6 // Botão para controlar a tarefa do Buzzer

// --- Configurações do Display OLED ---
#define I2C_SDA_PIN 14   // Pino SDA para comunicação I2C com o OLED
#define I2C_SCL_PIN 15   // Pino SCL para comunicação I2C com o OLED
#define I2C_ADDRESS 0x3C // Endereço I2C do display OLED SSD1306
#define I2C_FREQUENCY 400000 // Frequência da comunicação I2C em Hz (400kHz)
#define I2C_PORT i2c1        // Instância do barramento I2C a ser utilizada (i2c1)
#define OLED_WIDTH 128       // Largura do display OLED em pixels
#define OLED_HEIGHT 64       // Altura do display OLED em pixels

// Instância da estrutura de controle do display OLED
ssd1306_t display;

/**
 * @brief Inicializa o PWM para o controle do buzzer.
 * Configura o pino do buzzer para operar como saída PWM, define a frequência
 * e inicia o PWM com o nível de saída baixo.
 */
void buzzer_pwm_init(void)
{
    gpio_set_function(BUZZER_A_PIN, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(BUZZER_A_PIN);

    pwm_config config = pwm_get_default_config();
    // Calcula o divisor de clock para atingir a frequência desejada do buzzer.
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096));
    pwm_init(slice_num, &config, true); // Inicializa o slice PWM e o habilita

    pwm_set_gpio_level(BUZZER_A_PIN, 0); // Inicia o PWM com o buzzer desligado
}

// --- Handles das Tarefas do FreeRTOS ---
TaskHandle_t xLedTaskHandle = NULL;    // Handle para a tarefa do LED
TaskHandle_t xBuzzerTaskHandle = NULL; // Handle para a tarefa do Buzzer
TaskHandle_t xOledTaskHandle = NULL;   // Handle para a tarefa do OLED

/**
 * @brief Tarefa para controlar o LED RGB.
 * Alterna entre as cores Vermelho, Verde e Azul do LED RGB a cada 500ms.
 * @param pvParameters Parâmetros da tarefa (não utilizados).
 */
void led_task(void *pvParameters)
{
    const uint LED_PINS[] = {LED_R_PIN, LED_G_PIN, LED_B_PIN};
    const uint NUM_COLORS = sizeof(LED_PINS) / sizeof(LED_PINS[0]);
    uint current_color_index = 0;

    // Inicializa os pinos do LED como saída e os desliga
    for (uint i = 0; i < NUM_COLORS; ++i)
    {
        gpio_init(LED_PINS[i]);
        gpio_set_dir(LED_PINS[i], GPIO_OUT);
        gpio_put(LED_PINS[i], 0); // Garante que o LED comece desligado
    }

    while (true)
    {
        gpio_put(LED_PINS[current_color_index], 0); // Desliga a cor atual

        current_color_index = (current_color_index + 1) % NUM_COLORS; // Avança para a próxima cor

        gpio_put(LED_PINS[current_color_index], 1); // Liga a nova cor

        vTaskDelay(pdMS_TO_TICKS(500)); // Aguarda 500ms
    }
}

/**
 * @brief Tarefa para controlar o Buzzer.
 * Gera um beep curto de 100ms a cada segundo.
 * @param pvParameters Parâmetros da tarefa (não utilizados).
 */
void buzzer_task(void *pvParameters)
{
    while (true)
    {
        // Liga o buzzer com 50% do ciclo de trabalho (2048 de 4095 para 12-bit PWM)
        pwm_set_gpio_level(BUZZER_A_PIN, 2048);
        vTaskDelay(pdMS_TO_TICKS(100)); // Mantém ligado por 100ms
        pwm_set_gpio_level(BUZZER_A_PIN, 0);  // Desliga o buzzer
        vTaskDelay(pdMS_TO_TICKS(900)); // Aguarda 900ms, completando 1 segundo
    }
}

/**
 * @brief Tarefa para monitorar os botões e controlar as tarefas do LED e Buzzer.
 * Botão A: Suspende/resume a tarefa do LED.
 * Botão B: Suspende/resume a tarefa do Buzzer.
 * Utiliza detecção de borda de subida para os botões (transição de não pressionado para pressionado).
 * @param pvParameters Parâmetros da tarefa (não utilizados).
 */
void button_task(void *pvParameters)
{
    // Inicializa o pino do Botão A como entrada com pull-up interno
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    // Inicializa o pino do Botão B como entrada com pull-up interno
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);

    bool button_a_pressed_previously = false; // Estado anterior do Botão A
    bool button_b_pressed_previously = false; // Estado anterior do Botão B
    bool led_task_suspended = false;          // Flag para indicar se a tarefa do LED está suspensa
    bool buzzer_task_suspended = false;       // Flag para indicar se a tarefa do Buzzer está suspensa

    while (true)
    {
        // Leitura do Botão A (ativo em nível baixo devido ao pull-up)
        bool button_a_currently_pressed = !gpio_get(BUTTON_A_PIN);

        // Verifica se houve uma borda de subida (botão foi pressionado agora mas não antes)
        if (button_a_currently_pressed && !button_a_pressed_previously)
        {
            if (led_task_suspended)
            {
                vTaskResume(xLedTaskHandle); // Resume a tarefa do LED
                led_task_suspended = false;
                printf("Tarefa LED Retomada\n");
            }
            else
            {
                vTaskSuspend(xLedTaskHandle); // Suspende a tarefa do LED
                led_task_suspended = true;
                printf("Tarefa LED Suspensa\n");
            }
        }
        button_a_pressed_previously = button_a_currently_pressed; // Atualiza o estado anterior do Botão A

        // Leitura do Botão B (ativo em nível baixo devido ao pull-up)
        bool button_b_currently_pressed = !gpio_get(BUTTON_B_PIN);

        // Verifica se houve uma borda de subida (botão foi pressionado agora mas não antes)
        if (button_b_currently_pressed && !button_b_pressed_previously)
        {
            if (buzzer_task_suspended)
            {
                vTaskResume(xBuzzerTaskHandle); // Resume a tarefa do Buzzer
                buzzer_task_suspended = false;
                printf("Tarefa Buzzer Retomada\n");
            }
            else
            {
                vTaskSuspend(xBuzzerTaskHandle); // Suspende a tarefa do Buzzer
                buzzer_task_suspended = true;
                printf("Tarefa Buzzer Suspensa\n");
            }
        }
        button_b_pressed_previously = button_b_currently_pressed; // Atualiza o estado anterior do Botão B

        vTaskDelay(pdMS_TO_TICKS(100)); // Verifica os botões a cada 100ms para debouncing e responsividade
    }
}

/**
 * @brief Tarefa para exibir o estado das tarefas LED e Buzzer no display OLED.
 * Atualiza o display a cada 250ms com o status (Rodando/Suspensa) de cada tarefa.
 * @param pvParameters Parâmetros da tarefa (não utilizados).
 */
void oled_task(void *pvParameters) {
    char led_status_str[25];    // String para armazenar o status da tarefa LED
    char buzzer_status_str[25]; // String para armazenar o status da tarefa Buzzer
    eTaskState led_state;       // Enum para armazenar o estado da tarefa LED
    eTaskState buzzer_state;    // Enum para armazenar o estado da tarefa Buzzer

    // Aguarda até que os handles das tarefas LED e Buzzer sejam válidos.
    // Necessário caso esta tarefa inicie antes da criação completa das outras.
    while(xLedTaskHandle == NULL || xBuzzerTaskHandle == NULL) {
        vTaskDelay(pdMS_TO_TICKS(10)); // Pequeno delay para não sobrecarregar a CPU
    }

    printf("Tarefa OLED Iniciada\n");

    while (true) {
        // Obtém o estado da tarefa LED, verificando se o handle é válido
        if (xLedTaskHandle != NULL) {
            led_state = eTaskGetState(xLedTaskHandle);
        } else {
            led_state = eInvalid; // Define como estado inválido se o handle for nulo
        }

        // Obtém o estado da tarefa Buzzer, verificando se o handle é válido
        if (xBuzzerTaskHandle != NULL) {
            buzzer_state = eTaskGetState(xBuzzerTaskHandle);
        } else {
            buzzer_state = eInvalid; // Define como estado inválido se o handle for nulo
        }
        
        ssd1306_clear(&display); // Limpa o buffer do display antes de desenhar

        // Formata e exibe o status da tarefa LED
        if (led_state != eInvalid) {
            sprintf(led_status_str, "LED: %s",
                    (led_state == eSuspended) ? "Suspensa" : "Rodando ");
        } else {
            sprintf(led_status_str, "LED: Handle Nulo");
        }
        ssd1306_draw_string(&display, 0, 0, 1, led_status_str); // Desenha na linha 0

        // Formata e exibe o status da tarefa Buzzer
        if (buzzer_state != eInvalid) {
            sprintf(buzzer_status_str, "Buzzer: %s",
                    (buzzer_state == eSuspended) ? "Suspenso" : "Rodando");
        } else {
            sprintf(buzzer_status_str, "Buzzer: Handle Nulo");
        }
        ssd1306_draw_string(&display, 0, 10, 1, buzzer_status_str); // Desenha na linha 10 (abaixo da primeira)

        ssd1306_show(&display); // Atualiza o display físico com o conteúdo do buffer

        vTaskDelay(pdMS_TO_TICKS(250)); // Atualiza o display a cada 250ms
    }
}

/**
 * @brief Configura a comunicação I2C e inicializa o display OLED.
 * Define os pinos SDA e SCL, inicializa o barramento I2C e o controlador SSD1306.
 * Exibe uma mensagem de inicialização no display.
 */
void setup_i2c_oled() {
    i2c_init(I2C_PORT, I2C_FREQUENCY); // Inicializa o barramento I2C
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C); // Configura o pino SDA para I2C
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C); // Configura o pino SCL para I2C
    gpio_pull_up(I2C_SDA_PIN); // Habilita pull-up interno para SDA
    gpio_pull_up(I2C_SCL_PIN); // Habilita pull-up interno para SCL

    display.external_vcc = false; // Informa à biblioteca que o VCC é gerado internamente pelo display

    // Inicializa o display SSD1306
    if (!ssd1306_init(&display, OLED_WIDTH, OLED_HEIGHT, I2C_ADDRESS, I2C_PORT)) {
        printf("Falha ao inicializar SSD1306!\n");
        while(1); // Trava se a inicialização falhar
    }
    printf("OLED ok!\n");
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "Inicializando...");
    ssd1306_show(&display);
    sleep_ms(2000); // Aguarda 2 segundos para exibir a mensagem de inicialização
}

/**
 * @brief Função principal do programa.
 * Inicializa o sistema, periféricos (OLED, Buzzer), cria as tarefas do FreeRTOS
 * e inicia o escalonador de tarefas.
 * @return int Código de retorno (não deve ser alcançado, pois o escalonador FreeRTOS assume o controle).
 */
int main()
{
    stdio_init_all(); // Inicializa todas as E/S padrão (necessário para printf via USB/UART)
    sleep_ms(2000);   // Aguarda um tempo para a estabilização do sistema ou console serial
    printf("Sistema iniciando...\n");

    setup_i2c_oled();  // Configura I2C e inicializa o display OLED
    buzzer_pwm_init(); // Configura o PWM para o buzzer

    printf("Hardware inicializado.\n");

    // Criação das tarefas do FreeRTOS
    // xTaskCreate(função_tarefa, nome_tarefa, tamanho_pilha, parametros_tarefa, prioridade, &handle_tarefa)
    BaseType_t ledStatus = xTaskCreate(led_task, "LED_Task", 256, NULL, 1, &xLedTaskHandle);
    BaseType_t buzzerStatus = xTaskCreate(buzzer_task, "Buzzer_Task", 256, NULL, 1, &xBuzzerTaskHandle);
    BaseType_t buttonStatus = xTaskCreate(button_task, "Button_Task", 256, NULL, 2, NULL); // Prioridade maior para botões
    BaseType_t oledStatus = xTaskCreate(oled_task, "OLED_Task", 256, NULL, 1, &xOledTaskHandle);

    // Verifica se todas as tarefas foram criadas com sucesso
    if (ledStatus != pdPASS || buzzerStatus != pdPASS || buttonStatus != pdPASS || oledStatus != pdPASS) {
        printf("Erro ao criar uma ou mais tarefas!\n");
        // Exibe mensagem de erro no OLED se a inicialização falhar
        ssd1306_clear(&display);
        ssd1306_draw_string(&display, 0, 0, 1, "Erro Task!");
        ssd1306_show(&display);
        while(1); // Trava o sistema em caso de erro na criação de tarefas
    } else {
        printf("Tarefas criadas com sucesso.\n");
    }
    
    printf("Iniciando scheduler do FreeRTOS...\n");
    vTaskStartScheduler(); // Inicia o escalonador do FreeRTOS

    // O código abaixo desta linha nunca deve ser executado,
    // pois o controle é transferido para o escalonador do FreeRTOS.
    while (true)
    {
        // Loop infinito de segurança, não deve ser alcançado.
    }
    return 0; // Nunca alcançado
}