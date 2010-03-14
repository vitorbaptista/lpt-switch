#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "parapin.h"

#define KEY_CODE 5

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

  // Status anterior dos pinos.
  int OLD_PIN_STATUS[26];

  // Evento
  XKeyEvent event;

  while (true) {
    for (i = 1; i <= 25; i++)
      OLD_PIN_STATUS[i] = pin_is_set(LP_PIN[i]);

    for (i = 10; i <= 15; i++) {
      if (i == 14)
	continue;
      
      for (j = 2; j <= 8; j++) {
	if (!pin_is_set(LP_PIN[j])) {
	  if (OLD_PIN_STATUS[j] && OLD_PIN_STATUS[i]) {
	    // Envia evento de KeyRelease
	    //enviarEvento(display, winRoot, KEY_CODE, false);
	    printf("Release: %i %i\n", i, j);
	  }  
	} else {
	  if (pin_is_set(LP_PIN[i]) && !OLD_PIN_STATUS[j]) {
	    // Envia evento de KeyPress
	    //enviarEvento(display, winRoot, KEY_CODE, true);
	    printf("Press: %i %i\n", i, j);
	  }
	}
      }
    }
  }

  // Done.
  XCloseDisplay(display);

  return 0;
}
