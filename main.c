#include <unistd.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "parapin.h"

#define KEY_CODE 5

// Ainda não usamos todas as 60 possibilidades para facilitar o cálculo
// de qual botão está ativo, recebendo os valores do pino de status e
// de dados.
// A fórmula é: (status - 9) + [(dados - 2) * 5]
#define QNTD_BOTOES 40

#define NUM_BOTAO(status, dados) ((status - 9) + (dados - 2) * 5)

// Function to create a keyboard event
XKeyEvent createKeyEvent(Display *display, Window &win,
                           Window &winRoot, bool press,
                           int keycode, int modifiers)
{
   XKeyEvent event;

   event.display     = display;
   event.window      = win;
   event.root        = winRoot;
   event.subwindow   = None;
   event.time        = CurrentTime;
   event.x           = 1;
   event.y           = 1;
   event.x_root      = 1;
   event.y_root      = 1;
   event.same_screen = True;
   event.keycode     = XKeysymToKeycode(display, keycode);
   event.state       = modifiers;

   if(press)
      event.type = KeyPress;
   else
      event.type = KeyRelease;

   return event;
}

void enviarEvento(Display *display, Window &winRoot, int keyCode, bool isKeyDown) {
  // Variáveis para guardar a janela com foco
  Window winFocus;
  int revert;
  XGetInputFocus(display, &winFocus, &revert);

  // Send a fake key press event to the window.
  XKeyEvent event = createKeyEvent(display, winFocus, winRoot, true, keyCode, 0);
  XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
  
  // Send a fake key release event to the window.
  event = createKeyEvent(display, winFocus, winRoot, false, keyCode, 0);
  XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
}

int main() {
  // Contadores
  int i, j;

  // Status anteriores dos pinos
  int OLD_PIN_STATUS[26];
  for (i = 0; i < 26; i++)
    OLD_PIN_STATUS[i] = 0;

  // Pinos de status: 10, 11, 12, 13 e 15
  int *STATUS_PINS[5];
  STATUS_PINS[0] = &OLD_PIN_STATUS[10];
  STATUS_PINS[1] = &OLD_PIN_STATUS[11];
  STATUS_PINS[2] = &OLD_PIN_STATUS[12];
  STATUS_PINS[3] = &OLD_PIN_STATUS[13];
  STATUS_PINS[4] = &OLD_PIN_STATUS[15];

  // Pinos de dados: 2 até 9
  int *DATA_PINS[8];
  DATA_PINS[0] = &OLD_PIN_STATUS[2];
  DATA_PINS[1] = &OLD_PIN_STATUS[3];
  DATA_PINS[2] = &OLD_PIN_STATUS[4];
  DATA_PINS[3] = &OLD_PIN_STATUS[5];
  DATA_PINS[4] = &OLD_PIN_STATUS[6];
  DATA_PINS[5] = &OLD_PIN_STATUS[7];
  DATA_PINS[6] = &OLD_PIN_STATUS[8];
  DATA_PINS[7] = &OLD_PIN_STATUS[9];

  // Estado anterior dos botões
  bool ESTADO_BOTAO[QNTD_BOTOES];
  for (i = 0; i < QNTD_BOTOES; i++)
    ESTADO_BOTAO[i] = false;

  // Inicializa porta paralela
  if (pin_init_user(LPT1) < 0)
    return -1;

  pin_output_mode(LP_DATA_PINS);

  // Obtém o display do X11
  Display *display = XOpenDisplay(0);
  if (!display)
    return -1;

  // Pega a raiz do display atual
  Window winRoot = XDefaultRootWindow(display);

  // Evento
  XKeyEvent event;

    // Atualiza o estado anterior de todos os pinos
    for (i = 1; i <= 25; i++)
      OLD_PIN_STATUS[i] = pin_is_set(LP_PIN[i]);


  while (true) {
    // Seta todos os pinos de dados para 0
    // Assim conseguimos ver se os pinos de status estão
    // em curto ou não
    clear_pin(LP_DATA_PINS);

    // Loop entre os pinos de status
    for (i = 10; i <= 15; i++) {
      if (i == 14)
	continue;

      // Se ele está setado (não está em curto) nem agora 
      // nem anteriormente, significa que ele não está em 
      // curto nem acabou de ser soltado. Passa para a 
      // próxima iteração.
      if (pin_is_set(LP_PIN[i]) && OLD_PIN_STATUS[i])
        continue;

      // Atualiza o estado do pino de status atual
      OLD_PIN_STATUS[i] = pin_is_set(LP_PIN[i]);

      // Para cada pino de dados 
      for (j = 2; j <= 9; j++) {
        // Coloca todos em 1. Desta forma, o pino de status não
        // Ficará em curto. Depois disto, a ideia é colocar os
        // pinos de dados, um por um, em zero. Caso você coloque
        // o pino x em 0 e o pino de status fique em curto, significa
        // que o x e o de status estão ligados.
        set_pin(LP_DATA_PINS);

        clear_pin(LP_PIN[j]);

        if (!pin_is_set(LP_PIN[i])) {
          // Significa que o pino i está em curto com j

          // Se ele não estava em curto, então o jogador
          // acabou de apertá-lo.
          if (!ESTADO_BOTAO[NUM_BOTAO(i, j)]) {
            // Lança um evento KeyPress
            printf("KeyPress: %d e %d\n", i, j);
          }

          ESTADO_BOTAO[NUM_BOTAO(i, j)] = true;
        } else {
          // O pino i não está em curto com j

          // Se ele estava em curto, então o jogador
          // acabou de soltá-lo.
          if (ESTADO_BOTAO[NUM_BOTAO(i, j)]) {
            // Lança um evento KeyRelease
            printf("KeyRelease: %d e %d\n", i, j);
          }

          ESTADO_BOTAO[NUM_BOTAO(i, j)] = false;
        }
      }
    }

    usleep(200);
  }

  // Done.
  XCloseDisplay(display);

  return 0;
}
