# Tarefa: Roteiro de FreeRTOS #1 - EmbarcaTech 2025

Autor: **Danilo Oliveira Gonçalves**

Curso: Residência Tecnológica em Sistemas Embarcados

Instituição: EmbarcaTech - HBr

Brasília, Junho de 2025

---

# Controle Multitarefa com FreeRTOS no RP2040

## 🎯 Objetivo do Projeto

Este projeto demonstra o uso do sistema operacional de tempo real (RTOS) FreeRTOS no microcontrolador RP2040 (Raspberry Pi Pico). O objetivo é gerenciar múltiplas tarefas concorrentes: controle de um LED RGB, acionamento de um buzzer, monitoramento de botões para interação do usuário e atualização de um display OLED com o estado das tarefas. Este projeto serve como uma introdução prática aos conceitos fundamentais do FreeRTOS, como criação de tarefas, manipulação de handles de tarefas, suspensão e retomada de tarefas, e comunicação inter-tarefas implícita através da observação de estados.

## 🔧 Componentes Utilizados

- Placa com microcontrolador RP2040
- LED RGB 
- Buzzer passivo
- 2x Botões de pressão 
- Display OLED SSD1306 


## 📌 Pinagem do Dispositivo

| Constante         | Pino RP2040 (GP) | Função                                  | Conexão                |
|-------------------|--------------------|-----------------------------------------|------------------------|
| `LED_R_PIN`       | 13                 | Saída Digital                           | LED Vermelho (R)       |
| `LED_G_PIN`       | 11                 | Saída Digital                           | LED Verde (G)          |
| `LED_B_PIN`       | 12                 | Saída Digital                           | LED Azul (B)           |
| `BUZZER_A_PIN`    | 21                 | Saída PWM                               | Buzzer                 |
| `BUTTON_A_PIN`    | 5                  | Entrada Digital (com pull-up interno)   | Botão A (Controle LED) |
| `BUTTON_B_PIN`    | 6                  | Entrada Digital (com pull-up interno)   | Botão B (Controle Buzzer)|
| `I2C_SDA_PIN`     | 14                 | SDA (I2C1)                              | Display OLED SSD1306   |
| `I2C_SCL_PIN`     | 15                 | SCL (I2C1)                              | Display OLED SSD1306   |

## ⚙️ Como Compilar e Executar

1.  **Clone o Repositório:**
    ```bash
    git clone https://github.com/EmbarcaTech/tarefa-freertos-1-danlvr.git
    cd tarefa-freertos-1-danlvr
    ```

2.  **Configure o Pico SDK:**
    Certifique-se de que você tem o [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) configurado corretamente em seu ambiente de desenvolvimento. Defina a variável de ambiente `PICO_SDK_PATH`.

3.  **Integre o FreeRTOS:**
    Este projeto requer o FreeRTOS. O Pico SDK geralmente facilita a inclusão do FreeRTOS. Certifique-se de que seu `CMakeLists.txt` está configurado para encontrar e compilar o FreeRTOS.

    Se você precisar adicionar o FreeRTOS manualmente ao seu projeto SDK:
    *   **Obtenha o FreeRTOS Kernel:** Você pode adicioná-lo como um submódulo Git ou baixar os fontes.
        ```bash
        # Exemplo com submódulo (execute na raiz do seu projeto, não necessariamente este)
        # git submodule add https://github.com/FreeRTOS/FreeRTOS-Kernel.git extern/FreeRTOS-Kernel
        ```
    *   **Configure o `CMakeLists.txt`:**
        Você precisará informar ao CMake onde encontrar os fontes do FreeRTOS e o arquivo de configuração de portabilidade (`FreeRTOSConfig.h`) para o RP2040 (Cortex-M0+).
        O `pico_sdk_add_executable` e `target_link_libraries` no `CMakeLists.txt` principal do projeto devem ser ajustados para incluir o FreeRTOS.
        Consulte a documentação do Pico SDK para a forma recomendada de integrar o FreeRTOS se estiver configurando um novo projeto do zero. Para este projeto, espera-se que o `CMakeLists.txt` fornecido já lide com a inclusão do FreeRTOS.

4.  **Compile o Projeto:**
    Crie um diretório de build e compile:
    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```

5.  **Carregue o Firmware:**
    Copie o arquivo `.uf2` gerado (ex: `build/src/tarefa_freertos_1.uf2`) para o Raspberry Pi Pico quando ele estiver no modo bootloader (segurando o botão BOOTSEL ao conectar o USB).

## 📸 Demonstração do funcionamento

<video width="700" height="500" src="https://github.com/user-attachments/assets/b54d6f7d-a8b6-4cf7-8c22-c8835f952aad.mp4"></video>


## 📊 Resultados Esperados/Observados

-   **LED RGB:** Ao iniciar, o LED RGB começa a alternar entre as cores vermelho, verde e azul, com cada cor permanecendo acesa por 500ms.
-   **Buzzer:** O buzzer emite um beep curto (100ms) a cada segundo.
-   **Display OLED:**
    -   Inicialmente, exibe "Inicializando...".
    -   Após a inicialização das tarefas, exibe o status das tarefas do LED e do Buzzer (ex: "LED: Rodando", "Buzzer: Rodando").
    -   O status é atualizado a cada 250ms.
-   **Botão A (Controle LED):**
    -   Ao ser pressionado pela primeira vez, a tarefa do LED é suspensa. O LED para de mudar de cor, permanecendo no último estado. O display OLED reflete "LED: Suspensa".
    -   Ao ser pressionado novamente, a tarefa do LED é retomada. O LED volta a ciclar as cores. O display OLED reflete "LED: Rodando".
-   **Botão B (Controle Buzzer):**
    -   Ao ser pressionado pela primeira vez, a tarefa do Buzzer é suspensa. O buzzer para de emitir beeps. O display OLED reflete "Buzzer: Suspenso".
    -   Ao ser pressionado novamente, a tarefa do Buzzer é retomada. O buzzer volta a emitir beeps. O display OLED reflete "Buzzer: Rodando".
-   **Console Serial:** Mensagens de log são impressas no console serial (via USB) indicando o início do sistema, inicialização de hardware, criação de tarefas e quando as tarefas são suspensas ou retomadas.

## 🔍 Notas Técnicas

-   **FreeRTOS:** O projeto utiliza o FreeRTOS para gerenciar quatro tarefas principais:
    1.  `led_task`: Controla o ciclo de cores do LED RGB.
    2.  `buzzer_task`: Controla o acionamento periódico do buzzer usando PWM.
    3.  `button_task`: Monitora os dois botões. Detecta pressionamentos (borda de subida, considerando pull-ups internos) e suspende/resume as tarefas `led_task` e `buzzer_task` usando seus respectivos handles (`xLedTaskHandle`, `xBuzzerTaskHandle`).
    4.  `oled_task`: Atualiza o display OLED com o estado atual das tarefas do LED e do Buzzer, obtendo o estado através da função `eTaskGetState()`.
-   **Prioridades de Tarefa:** A `button_task` possui prioridade maior (2) que as demais tarefas (1) para garantir responsividade na leitura dos botões.
-   **PWM para Buzzer:** O buzzer é controlado utilizando um canal PWM do RP2040 para gerar a frequência e o ciclo de trabalho desejados.
-   **I2C para OLED:** A comunicação com o display OLED SSD1306 é feita através da interface I2C.
-   **Debouncing de Botão:** A `button_task` implementa uma forma simples de debouncing ao verificar o estado dos botões periodicamente (a cada 100ms) e agir na transição de "não pressionado" para "pressionado".
-   **Handles de Tarefa:** Os handles `xLedTaskHandle` e `xBuzzerTaskHandle` são essenciais para que a `button_task` e a `oled_task` possam controlar e consultar o estado das outras tarefas. A `oled_task` também usa `xOledTaskHandle` para si, embora não seja usado ativamente por outras tarefas neste exemplo.

## 📚 Bibliotecas Utilizadas

-   **Raspberry Pi Pico SDK (`pico-sdk`):** SDK oficial para desenvolvimento no RP2040. Inclui drivers de hardware (GPIO, PWM, I2C, etc.) e bibliotecas de utilidades.
-   **FreeRTOS Kernel:** Sistema operacional de tempo real para microcontroladores. ([Site Oficial](https://www.freertos.org/))
-   **[pico-ssd1306](https://github.com/daschr/pico-ssd1306):** Biblioteca para utilização de displays OLED SSD1306 com o Raspberry Pi Pico e o pico-sdk (ou uma biblioteca similar para SSD1306, como a que está sendo usada no projeto).

---

## 🤔 Reflexões

### O que acontece se todas as tarefas tiverem a mesma prioridade?
Se todas as tarefas tiverem a mesma prioridade, o escalonador do FreeRTOS (configurado por padrão para preemptivo com _time slicing_ ou _round-robin_ para tarefas de mesma prioridade) alternará entre elas, concedendo a cada tarefa uma fatia de tempo (quantum) da CPU. Quando uma tarefa esgota sua fatia de tempo ou bloqueia (por exemplo, chamando `vTaskDelay()`), o escalonador passa o controle para a próxima tarefa pronta de mesma prioridade na fila. Isso garante que todas as tarefas de mesma prioridade tenham a chance de executar, prevenindo que uma única tarefa monopolize a CPU indefinidamente, desde que elas cooperem cedendo o processador (ex: através de delays ou bloqueios em operações de E/S). No entanto, tarefas computacionalmente intensivas que não bloqueiam podem impactar a responsividade de outras tarefas de mesma prioridade.

### Qual tarefa consome mais tempo da CPU?
Determinar qual tarefa consome mais tempo de CPU sem ferramentas de profiling específicas pode ser uma estimativa. No entanto, podemos inferir:
-   A `led_task` e a `buzzer_task` passam a maior parte do tempo bloqueadas em `vTaskDelay()`. A `led_task` tem um delay de 500ms e a `buzzer_task` tem delays de 100ms e 900ms. O processamento ativo delas é mínimo (algumas chamadas `gpio_put` ou `pwm_set_gpio_level`).
-   A `button_task` executa a cada 100ms (`vTaskDelay(pdMS_TO_TICKS(100))`). Dentro de seu loop, ela lê dois GPIOs e realiza algumas comparações lógicas. O consumo é relativamente baixo, mas mais frequente que as tarefas de LED e buzzer quando estão em seus longos delays.
-   A `oled_task` executa a cada 250ms (`vTaskDelay(pdMS_TO_TICKS(250))`). Dentro de seu loop, ela chama `eTaskGetState()` duas vezes, `ssd1306_clear()`, `sprintf()` duas vezes, `ssd1306_draw_string()` duas vezes e `ssd1306_show()`. As operações de manipulação de strings e, especialmente, a comunicação I2C com o display (`ssd1306_show()`) podem ser comparativamente mais intensivas em termos de CPU e tempo de barramento.

**Conclusão:** A `oled_task` é a candidata mais provável a consumir mais tempo de CPU devido às operações de renderização e comunicação I2C, seguida pela `button_task` devido à sua frequência de execução, embora com operações mais simples. As tarefas `led_task` e `buzzer_task` provavelmente consomem menos tempo de CPU devido aos seus longos períodos de bloqueio. Para uma medição precisa, seria necessário usar as funcionalidades de estatísticas de tempo de execução do FreeRTOS (`configGENERATE_RUN_TIME_STATS`).

### Quais seriam os riscos de usar polling sem prioridades (ou em um sistema sem RTOS)?
Usar polling em um sistema sem um RTOS (superloop) ou em um RTOS onde todas as tarefas relevantes rodam com a mesma prioridade e usam polling intensivo apresenta vários riscos:

1.  **Desperdício de CPU:** Tarefas que usam polling contínuo para verificar uma condição (ex: `while(!condition_met) {}`) consomem ciclos de CPU desnecessariamente, mesmo quando não há trabalho útil a ser feito. Isso aumenta o consumo de energia e impede que outras partes do sistema (ou outras tarefas de mesma prioridade) executem eficientemente.
2.  **Baixa Responsividade:** Se uma tarefa em polling intensivo detém a CPU por muito tempo, outras tarefas ou verificações de eventos podem ser atrasadas. Por exemplo, se uma tarefa está em um loop de polling apertado e outra precisa responder a um evento crítico (como um botão pressionado), a resposta a esse evento pode ser significativamente retardada.
3.  **Dificuldade de Temporização e Previsibilidade:** Em um sistema baseado em polling sem prioridades claras ou mecanismos de preempção, é difícil garantir que tarefas críticas em termos de tempo sejam executadas dentro de seus prazos. O tempo de resposta a um evento torna-se dependente da soma dos tempos de execução de todos os polls no loop principal.
4.  **Complexidade de Gerenciamento:** À medida que o número de dispositivos ou condições a serem verificadas por polling aumenta, a lógica do loop principal pode se tornar complexa e difícil de manter. Garantir que todos os dispositivos sejam verificados com a frequência adequada sem impactar negativamente outros se torna um desafio.
5.  **Starvation (Inanição):** Em um cenário com múltiplas tarefas de polling de mesma prioridade, se uma delas tiver um loop de polling muito longo ou uma condição que raramente cede o processador, outras tarefas podem não receber tempo de CPU suficiente, levando à inanição.

O uso de interrupções e mecanismos de bloqueio de RTOS (como semáforos, filas, ou `vTaskDelay`) é geralmente preferível ao polling intensivo, pois permite que a CPU realize outras tarefas enquanto espera por eventos, melhorando a eficiência e a responsividade do sistema.

---

## 📜 Licença
GNU GPL-3.0.
